import argparse
import cv2
import time
import threading
import queue
import logging
import os
import sys
import numpy as np

os.makedirs("log", exist_ok=True)
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.FileHandler("log/lab.log", encoding="utf-8"),
        logging.StreamHandler(sys.stdout)
    ]
)

class Sensor:
    def get(self):
        raise NotImplementedError("Subclasses must implement method get()")

class SensorX(Sensor):
    '''Sensor X'''
    def __init__(self, delay: float):
        self._delay = delay
        self._data = 0

    def get(self) -> int:
        time.sleep(self._delay)
        self._data += 1
        return self._data

class SensorCam(Sensor):
    def __init__(self, camera_id: str, resolution: tuple):
        self._camera_id = camera_id
        self._resolution = resolution
        self._cap = None
        self._init_camera()

    def _init_camera(self):
        try:
            cam_source = int(self._camera_id) if self._camera_id.isdigit() else self._camera_id
            self._cap = cv2.VideoCapture(cam_source)
            
            if not self._cap.isOpened():
                logging.error(f"Камера '{self._camera_id}' не найдена или недоступна.")
                sys.exit(1)
                
            self._cap.set(cv2.CAP_PROP_FRAME_WIDTH, self._resolution[0])
            self._cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self._resolution[1])
            logging.info(f"Камера {self._camera_id} инициализирована: {self._resolution}")
        except Exception as e:
            logging.error(f"Ошибка инициализации камеры: {e}")
            sys.exit(1)

    def get(self):
        try:
            ret, frame = self._cap.read()
            if not ret:
                logging.warning("Не удалось прочитать кадр. Камера могла отключиться или потерян сигнал.")
                return None
            return frame
        except Exception as e:
            logging.error(f"Ошибка чтения с камеры: {e}")
            return None

    def __del__(self):
        if self._cap is not None and self._cap.isOpened():
            self._cap.release()
            logging.info(f"Ресурс камеры '{self._camera_id}' освобожден (RAII)")

class WindowImage:
    def __init__(self, display_freq: float):
        self._display_freq = display_freq
        self._window_name = "Sensors Monitor"
        self._wait_ms = max(1, int(1000 / self._display_freq))
        self._init_window()

    def _init_window(self):
        try:
            cv2.namedWindow(self._window_name, cv2.WINDOW_AUTOSIZE)
            logging.info(f"Окно '{self._window_name}' создано. Частота обновления: {self._display_freq} Гц")
        except Exception as e:
            logging.error(f"Ошибка создания окна: {e}")
            sys.exit(1)

    def show(self, img: np.ndarray) -> int:
        try:
            if img is None or img.size == 0:
                logging.warning("Получено пустое изображение для отображения.")
                return -1
            cv2.imshow(self._window_name, img)
            key = cv2.waitKey(self._wait_ms) & 0xFF
            return key
        except Exception as e:
            logging.error(f"Ошибка отображения изображения: {e}")
            return -1

    def __del__(self):
        try:
            cv2.destroyWindow(self._window_name)
            logging.info("Ресурс окна отображения освобожден (RAII)")
        except Exception:
            pass

def compose_image(frame: np.ndarray, sensor_data: list) -> np.ndarray:
    """Формирует итоговое изображение с панелью датчиков."""
    if frame is None:
        frame = np.zeros((720, 1280, 3), dtype=np.uint8)

    h, w = frame.shape[:2]

    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 0.6
    color_text = (0, 0, 0)
    color_bg = (255, 255, 255)
    thickness = 2
    padding = 15
    margin = 20

    lines = []
    for i, val in enumerate(sensor_data[1:], start=1):
        lines.append(f"SensorX{i-1}: {val}")

    if not lines:
        return frame

    max_w = 0
    line_h = 0
    for line in lines:
        (tw, th), _ = cv2.getTextSize(line, font, font_scale, thickness)
        max_w = max(max_w, tw)
        line_h = max(line_h, th)

    box_w = max_w + padding * 2
    box_h = len(lines) * line_h + padding * 2 + (len(lines) - 1) * 10

    x2 = w - margin
    y2 = h - margin
    x1 = x2 - box_w
    y1 = y2 - box_h

    cv2.rectangle(frame, (x1, y1), (x2, y2), color_bg, -1)

    current_y = y1 + padding + line_h
    for line in lines:
        cv2.putText(frame, line, (x1 + padding, current_y), font, font_scale, color_text, thickness)
        current_y += line_h + 10

    return frame

def sensor_worker(sensor: Sensor, q: queue.Queue, stop_event: threading.Event):
    while not stop_event.is_set():
        try:
            data = sensor.get()
            try:
                q.put_nowait(data)
            except queue.Full:
                q.get_nowait()
                q.put_nowait(data)
        except Exception as e:
            logging.error(f"Критическая ошибка в потоке датчика: {e}")
            stop_event.set()
            break

def main():
    parser = argparse.ArgumentParser(description="Лабораторная: Теория параллелизма (Python Threading)")
    parser.add_argument("--camera", type=str, default="0", help="Имя/индекс камеры")
    parser.add_argument("--resolution", type=str, default="1280x720", help="Разрешение камеры (ШxВ)")
    parser.add_argument("--fps", type=float, default=10.0, help="Частота отображения картинки (Гц)")
    args = parser.parse_args()

    try:
        res = tuple(map(int, args.resolution.split('x')))
        if len(res) != 2: raise ValueError
    except ValueError:
        logging.error("Неверный формат разрешения. Ожидается ШxВ, например 1280x720")
        sys.exit(1)

    logging.info("Запуск системы параллельного опроса датчиков...")

    sensors = [
        SensorCam(args.camera, res),
        SensorX(0.01),  
        SensorX(0.1),   
        SensorX(1.0)   
    ]
    
    queues = [queue.Queue(maxsize=1) for _ in sensors]
    stop_event = threading.Event()
    threads = []

    for sensor, q in zip(sensors, queues):
        t = threading.Thread(target=sensor_worker, args=(sensor, q, stop_event), daemon=True)
        t.start()
        threads.append(t)

    window = WindowImage(args.fps)
    latest_data = [None] * len(sensors)

    try:
        while not stop_event.is_set():
            for i, q in enumerate(queues):
                try:
                    latest_data[i] = q.get_nowait()
                except queue.Empty:
                    pass

            display_frame = compose_image(latest_data[0], latest_data)
            key = window.show(display_frame)

            if key == ord('q'):
                logging.info("Нажата клавиша 'q'. Инициируется безопасное завершение...")
                stop_event.set()
                break
                
    except KeyboardInterrupt:
        logging.info("Прерывание пользователем (Ctrl+C). Завершение...")
    finally:
        stop_event.set()
        for t in threads:
            t.join(timeout=2.0)
        logging.info("Все потоки завершены. Ресурсы освобождены. Программа закрыта.")

if __name__ == "__main__":
    main()
