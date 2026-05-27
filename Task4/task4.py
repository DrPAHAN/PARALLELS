import argparse
import cv2
import logging
import numpy as np
import os
import queue
import sys
import threading
import time


LOG_DIR = 'log'
os.makedirs(LOG_DIR, exist_ok=True)

logger = logging.getLogger('ParallelismLab')
logger.setLevel(logging.DEBUG)

file_handler = logging.FileHandler(os.path.join(LOG_DIR, 'lab.log'), encoding='utf-8')
file_handler.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
logger.addHandler(file_handler)

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.INFO)
logger.addHandler(console_handler)


class Sensor:
    def get(self):
        raise NotImplementedError("Subclasses must implement method get()")

class SensorX(Sensor):
    '''Sensor X'''
    def __init__(self, delay: float):
        super().__init__()
        self._delay = delay
        self._data = 0
        self._queue = queue.Queue()
        self._stop_event = threading.Event()
        self._thread = None
        self._cached_value = 0
        
        try:
            self._thread = threading.Thread(target=self._worker, daemon=True)
            self._thread.start()
        except Exception as e:
            self.close()
            raise RuntimeError(f"Failed to start SensorX thread: {e}") from e

    def get(self) -> int:
        time.sleep(self._delay)
        self._delay += 1
        self._data += 1
        return self._data

    def _worker(self):
        while not self._stop_event.is_set():
            try:
                val = self.get()
                self._queue.put_nowait(val)
            except Exception as e:
                logger.error(f"SensorX error: {e}")
                time.sleep(0.1)

    def get_latest(self) -> int:
        latest = self._cached_value
        try:
            while True:
                latest = self._queue.get_nowait()
        except queue.Empty:
            pass
        self._cached_value = latest
        return latest

    def close(self):
        if hasattr(self, '_stop_event'):
            self._stop_event.set()
        if hasattr(self, '_thread') and self._thread and self._thread.is_alive():
            self._thread.join(timeout=1.0)

    def __del__(self):
        self.close()

class SensorCam(Sensor):
    def __init__(self, camera_id, resolution: tuple):
        super().__init__()
        self._camera_id = camera_id
        self._resolution = resolution
        
        self._cam = None
        self._queue = queue.Queue()
        self._stop_event = threading.Event()
        self._thread = None
        self._cached_frame = None
        
        backend = cv2.CAP_AVFOUNDATION if sys.platform == 'darwin' else cv2.CAP_V4L2
            
        try:
            self._cam = cv2.VideoCapture(camera_id, backend)
            if not self._cam.isOpened():
                logger.critical(f"Камера с индексом {camera_id} не открывается.")
                raise RuntimeError(f"Failed to open camera: {camera_id}")

            self._cam.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
            self._cam.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
            logger.info(f"Камера {camera_id} инициализирована с разрешением {resolution}")

            self._thread = threading.Thread(target=self._worker, daemon=True)
            self._thread.start()
        except Exception:
            self.close()
            raise

    def get(self):
        if not self._cam or not self._cam.isOpened():
            return None
        ret, frame = self._cam.read()
        if not ret:
            logger.warning("Не удалось прочитать кадр с камеры.")
            return None
        return frame

    def _worker(self):
        while not self._stop_event.is_set():
            frame = self.get()
            if frame is not None:
                self._queue.put_nowait(frame)
            else:
                logger.error("Камера перестала отдавать кадры. Поток остановлен.")
                self._stop_event.set()

    def get_latest(self):
        latest = self._cached_frame
        try:
            while True:
                latest = self._queue.get_nowait()
        except queue.Empty:
            pass
        self._cached_frame = latest
        return latest

    def close(self):
        if hasattr(self, '_stop_event'):
            self._stop_event.set()
        if hasattr(self, '_thread') and self._thread and self._thread.is_alive():
            self._thread.join(timeout=1.0)
        if hasattr(self, '_cam') and self._cam and self._cam.isOpened():
            self._cam.release()
            logger.info("Ресурс камеры корректно освобожден.")

    def __del__(self):
        self.close()

class WindowImage:
    def __init__(self, fps: float, window_name: str = "Sensors Display"):
        self._fps = fps
        self._window_name = window_name
        self._wait_time = max(1, int(1000.0 / fps))
        self._created = False
        
        try:
            cv2.namedWindow(self._window_name, cv2.WINDOW_NORMAL)
            self._created = True
            logger.info(f"Окно '{self._window_name}' создано.")
        except Exception as e:
            logger.critical(f"Ошибка создания окна OpenCV: {e}")
            raise

    def show(self, img) -> bool:
        if img is None or not self._created:
            return False
        cv2.imshow(self._window_name, img)
        key = cv2.waitKey(self._wait_time) & 0xFF
        return key == ord('q')

    def close(self):
        if hasattr(self, '_created') and self._created:
            try:
                cv2.destroyWindow(self._window_name)
                logger.info("Окно корректно закрыто.")
            except Exception as e:
                logger.error(f"Ошибка закрытия окна: {e}")
            self._created = False

    def __del__(self):
        self.close()

def parse_args():
    parser = argparse.ArgumentParser(description="Лабораторная работа №4")
    parser.add_argument('--camera', type=str, default='0', help='Индекс камеры (на macOS используйте 0, 1...)')
    parser.add_argument('--resolution', type=str, default='1280x720', help='Желаемое разрешение камеры (WxH)')
    parser.add_argument('--fps', type=float, default=30.0, help='Частота отображения картинки (Hz)')
    return parser.parse_args()

def main():
    args = parse_args()

    try:
        w, h = map(int, args.resolution.split('x'))
        resolution = (w, h)
    except ValueError:
        logger.error("Неверный формат разрешения. Используйте например, 1280x720)")
        sys.exit(1)

    cam_id_raw = args.camera
    if sys.platform == 'darwin':
        if cam_id_raw.startswith('/dev/video'):
            cam_id = 0
        else:
            try:
                cam_id = int(cam_id_raw)
            except ValueError:
                cam_id = 0
    else:
        try:
            cam_id = int(cam_id_raw)
        except ValueError:
            cam_id = cam_id_raw

    logger.info(f"Запуск программы (OS: {sys.platform}, Camera ID: {cam_id})...")
    
    sensors = []
    window = None
    cam = None
    
    try:
        cam = SensorCam(cam_id, resolution)
        sensors = [SensorX(0.01), SensorX(0.1), SensorX(1)]
        window = WindowImage(args.fps)

        last_frame = None
        last_sensor_data = [0, 0, 0]

        while True:
            frame = cam.get_latest()
            if frame is not None:
                last_frame = frame.copy()
            elif last_frame is None:
                last_frame = np.zeros((h, w, 3), dtype=np.uint8)
                cv2.putText(last_frame, "Waiting for camera...", (w//4, h//2), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

            for i, s in enumerate(sensors):
                last_sensor_data[i] = s.get_latest()

            
            display_frame = last_frame.copy()
            
            font = cv2.FONT_HERSHEY_SIMPLEX
            font_scale = 0.8
            font_thickness = 2
            line_spacing = 30
            padding = 12
            margin = 20

            text_lines = [f"Sensor X{i}: {val}" for i, val in enumerate(last_sensor_data)]

            max_w = max(cv2.getTextSize(line, font, font_scale, font_thickness)[0][0] for line in text_lines)
            box_w = max_w + padding * 2
            box_h = len(text_lines) * line_spacing + padding * 2

            frame_h, frame_w = display_frame.shape[:2]
            x1 = frame_w - box_w - margin
            y1 = frame_h - box_h - margin

            cv2.rectangle(display_frame, (x1, y1), (x1 + box_w, y1 + box_h), (255, 255, 255), -1)

            for i, line in enumerate(text_lines):
                y_text = y1 + padding + (i + 1) * line_spacing
                cv2.putText(display_frame, line, (x1 + padding, y_text), font, font_scale, (0, 0, 0), font_thickness, cv2.LINE_AA)
                
            if window.show(display_frame):
                logger.info("Нажата клавиша 'q'. Завершение работы...")
                break
                
    except KeyboardInterrupt:
        logger.info("Программа прервана пользователем (Ctrl+C).")
    finally:
        for s in sensors:
            s.close()
        if cam:
            cam.close()
        if window:
            window.close()
        logger.info("Все ресурсы освобождены. Выход.")

if __name__ == "__main__":
    main()