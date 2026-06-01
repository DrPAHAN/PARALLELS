import argparse  # Модуль для парсинга аргументов командной строки
import cv2  # Библиотека OpenCV для работы с видео (чтение, запись, кодирование)
import time  # Модуль для высокоточного замера времени выполнения
import os  # Модуль для работы с путями файловой системы и абсолютизации
import platform  # Модуль для определения текущей операционной системы
import queue  # Потокобезопасная очередь (оставлена на случай добавления RealTime-режима)
import threading  # Модуль для создания потоков (оставлен на случай RealTime-режима)
import multiprocessing as mp  # Модуль для работы с процессами (обходит GIL Python для CPU-задач)
from concurrent.futures import ProcessPoolExecutor  # Менеджер пула процессов для удобного параллелизма
from ultralytics import YOLO  # Импорт класса YOLOv8 для детекции поз и KeyPoints

class VideoResource:  # Класс, реализующий идиому RAII для управления видео-ресурсами
    def __init__(self, input_path=None, output_path=None):  # Конструктор: принимает пути к входному/выходному видео
        self.cap = None  # Инициализируем атрибут для объекта чтения видео
        self.writer = None  # Инициализируем атрибут для объекта записи видео
        self.active = True  # Флаг активности ресурсов (нужен для защиты от двойного освобождения)
        self.fps = 30.0  # Значение FPS по умолчанию на случай ошибки чтения метаданных
        self.resolution = (640, 480)  # Разрешение видео по умолчанию

        if input_path:  # Если передан путь к входному файлу
            abs_input = os.path.abspath(input_path)  # Преобразуем относительный путь в абсолютный (избегаем багов ОС)
            self.cap = cv2.VideoCapture(abs_input)  # Создаём объект OpenCV для чтения видео
            if not self.cap.isOpened():  # Проверяем, удалось ли открыть файл (существует ли он, поддерживается ли формат)
                raise RuntimeError(f"Не удалось открыть видео: {abs_input}")  # Выбрасываем исключение при ошибке
            
            fps_val = self.cap.get(cv2.CAP_PROP_FPS)  # Считываем частоту кадров из метаданных видеофайла
            self.fps = fps_val if fps_val > 0 else 30.0  # Если FPS=0 (баг контейнера), ставим безопасное значение 30.0
            self.resolution = (int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH)),  # Читаем ширину кадра
                               int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)))  # Читаем высоту кадра
            print(f"Видео: {self.resolution} @ {self.fps:.1f} FPS")  # Выводим реальные параметры для отладки

        if output_path:  # Если передан путь для сохранения результата
            abs_output = os.path.abspath(output_path)  # Преобразуем путь сохранения в абсолютный
            is_mac = platform.system() == "Darwin"  # Проверяем, запущен ли скрипт на macOS
            
            codecs_priority = [  # Список приоритетных кодеков для разных ОС (от лучшего к запасному)
                ('avc1', abs_output) if is_mac else ('mp4v', abs_output),  # macOS: H.264 нативный, остальные: mp4v
                ('H264', abs_output),  # Резервный кодек H.264
                ('mp4v', abs_output),  # Стандартный MPEG-4
                ('XVID', os.path.splitext(abs_output)[0] + ".avi"),  # Запасной вариант в формате AVI
                ('MJPG', os.path.splitext(abs_output)[0] + ".avi")  # Motion JPEG как последний резерв
            ]  # Завершаем список кодеков
            
            success = False  # Флаг успешного создания VideoWriter
            for fourcc_code, out_file in codecs_priority:  # Перебираем кодеки по приоритету
                fourcc = cv2.VideoWriter_fourcc(*fourcc_code)  # Создаём 4-байтный код кодека для OpenCV
                self.writer = cv2.VideoWriter(out_file, fourcc, self.fps, self.resolution)  # Пытаемся открыть файл для записи
                if self.writer.isOpened():  # Проверяем, успешно ли открылся поток записи
                    success = True  # Кодек подошёл, ставим флаг успеха
                    print(f"VideoWriter создан: '{fourcc_code}' -> {out_file}")  # Информируем пользователя
                    break  # Выходим из цикла, т.к. рабочий кодек найден
                self.writer.release()  # Закрываем неудачный поток, чтобы не занимать файл дескриптором
                
            if not success:  # Если ни один кодек не сработал
                raise RuntimeError("Не удалось создать VideoWriter. На macOS рекомендуется установить 'opencv-contrib-python' или использовать кодек 'avc1'.")  # Выбрасываем ошибку с подсказкой

    def __del__(self):  # Деструктор класса (вызывается сборщиком мусора Python при удалении объекта)
        self.release()  # Гарантированно освобождаем ресурсы при уничтожении экземпляра

    def release(self):  # Метод явного освобождения ресурсов видео
        if self.active:  # Проверяем, не были ли ресурсы уже освобождены ранее
            if self.cap:  # Если объект чтения существует
                self.cap.release()  # Закрываем дескриптор входного видео
            if self.writer:  # Если объект записи существует
                self.writer.release()  # Закрываем дескриптор выходного видео и сбрасываем буфер на диск
            self.active = False  # Обновляем флаг, чтобы метод не вызвался дважды
            print("[RAII] Ресурсы видео освобождены.")  # Лог успешного освобождения

_worker_model = None  # Глобальная переменная для хранения модели внутри дочернего процесса

def init_worker(model_path: str):  # Функция инициализации каждого воркера пула процессов
    global _worker_model  # Объявляем, что работаем с глобальной переменной
    _worker_model = YOLO(model_path)  # Загружаем модель один раз при старте процесса (экономит RAM и время)

def process_frame(args):  # Функция обработки одного кадра в отдельном процессе
    idx, frame = args  # Распаковываем индекс кадра и сам кадр из переданного кортежа
    results = _worker_model(frame, verbose=False)  # Запускаем инференс YOLO на кадре (без вывода в консоль)
    return idx, results[0].plot()  # Возвращаем индекс и кадр с наложенными KeyPoints (plot() рисует позу)

def run_single(input_path, output_path, model_path):  # Функция однопоточной обработки видео
    vid = VideoResource(input_path=input_path, output_path=output_path)  # Создаём RAII-объект для видео
    model = YOLO(model_path)  # Загружаем модель в основной поток
    total_frames = int(vid.cap.get(cv2.CAP_PROP_FRAME_COUNT))  # Считаем общее число кадров в видео
    
    start = time.perf_counter()  # Запускаем высокоточный таймер
    try:  # Блок try гарантирует выполнение finally даже при внезапной ошибке инференса
        while True:  # Цикл чтения кадров до конца видео
            ret, frame = vid.cap.read()  # Читаем следующий кадр из файла
            if not ret: break  # Если кадры закончились (ret=False), выходим из цикла
            res = model(frame, verbose=False)  # Обрабатываем кадр через YOLO
            vid.writer.write(res[0].plot())  # Записываем кадр с наложенной позой в файл
    finally:  # Блок выполнится в любом случае (успех или ошибка)
        vid.release()  # Явно освобождаем ресурсы видео (гарантирует корректное сохранение файла)
    end = time.perf_counter()  # Останавливаем таймер
    
    return end - start, total_frames  # Возвращаем затраченное время и число кадров

def run_multi(input_path, output_path, model_path, num_workers=None):  # Функция многопроцессорной обработки видео
    if num_workers is None:  # Если число процессов не указано явно в аргументах
        num_workers = mp.cpu_count()  # Берём количество физических/логических ядер CPU
        
    vid = VideoResource(input_path=input_path, output_path=output_path)  # Инициализируем RAII-объект
    total_frames = int(vid.cap.get(cv2.CAP_PROP_FRAME_COUNT))  # Узнаём общее число кадров
    
    frames = []  # Список для хранения всех кадров в оперативной памяти
    while True:  # Цикл предзагрузки кадров
        ret, frame = vid.cap.read()  # Читаем кадр из потока
        if not ret: break  # Прекращаем чтение при достижении конца видео
        frames.append(frame)  # Сохраняем кадр в список для дальнейшей параллельной обработки
        
    start = time.perf_counter()  # Засекаем время начала параллельной обработки
    processed = [None] * total_frames  # Создаём массив-заглушку для результатов в исходном порядке
    
    try:  # Гарантируем закрытие видео в блоке finally
        with ProcessPoolExecutor(max_workers=num_workers,  # Создаём контекстный менеджер пула процессов
                                 initializer=init_worker,  # Указываем функцию запуска воркеров
                                 initargs=(model_path,)) as executor:  # Передаём путь к модели в каждый процесс
            futures = {executor.submit(process_frame, (i, frame)): i for i, frame in enumerate(frames)}  # Отправляем кадры в процессы параллельно
            for future in futures:  # Ждём завершения каждой асинхронной задачи
                idx, res_frame = future.result()  # Получаем результат обработки кадра (может прийти в случайном порядке)
                processed[idx] = res_frame  # Кладём обработанный кадр на правильное место по индексу
                
        for frame in processed:  # Последовательно записываем готовые кадры в файл
            vid.writer.write(frame)  # Пишем кадр в выходной видеопоток
    finally:  # Гарантированное выполнение независимо от ошибок
        vid.release()  # Освобождаем ресурсы и финализируем видеофайл
    end = time.perf_counter()  # Останавливаем таймер
    
    return end - start, total_frames  # Возвращаем метрики производительности

def main():  # Точка входа в программу
    parser = argparse.ArgumentParser(description="Лабораторная 5: Ускорение инференса YOLOv8s-pose")  # Создаём парсер аргументов
    parser.add_argument("--video", type=str, required=True, help="Путь к видео (640x480)")  # Обязательный аргумент: входной файл
    parser.add_argument("--mode", type=str, required=True, choices=["single", "multi"], help="single или multi")  # Выбор режима: 1 поток или пул процессов
    parser.add_argument("--output", type=str, required=True, help="Имя выходного файла")  # Обязательный аргумент: путь сохранения
    parser.add_argument("--workers", type=int, default=None, help="Число процессов")  # Опционально: количество воркеров
    parser.add_argument("--model", type=str, default="yolov8s-pose.pt", help="Модель YOLO")  # Опционально: путь к весам нейросети
    parser.add_argument("--realtime", action="store_true", help="RealTime с камеры (+3 балла)")  # Флаг запуска бонусного режима
    
    args = parser.parse_args()  # Парсим аргументы из командной строки
        
    if args.mode == "single":  # Если выбран однопоточный режим
        exec_time, total = run_single(args.video, args.output, args.model)  # Запускаем последовательную обработку
    else:  # Иначе выбран многопроцессорный режим
        exec_time, total = run_multi(args.video, args.output, args.model, args.workers)  # Запускаем параллельную обработку
        
    print(f"\nОбработано кадров: {total}")  # Выводим итоговое количество обработанных кадров
    print(f"Время выполнения: {exec_time:.4f} сек")  # Выводим затраченное время с точностью до 4 знаков
    print(f"Средняя скорость: {total/exec_time:.2f} FPS")  # Рассчитываем и выводим среднюю частоту кадров

if __name__ == "__main__":  # Стандартная проверка запуска скрипта напрямую (а не импортом в другой модуль)
    main()  # Вызываем главную функцию программы