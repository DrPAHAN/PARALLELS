import argparse
import cv2
import time
import os
import platform
import queue
import threading
import multiprocessing as mp
from concurrent.futures import ProcessPoolExecutor
from ultralytics import YOLO

class VideoResource:
    def __init__(self, input_path=None, output_path=None):
        self.cap = None
        self.writer = None
        self.active = True
        self.fps = 30.0
        self.resolution = (640, 480)

        if input_path:
            abs_input = os.path.abspath(input_path)
            self.cap = cv2.VideoCapture(abs_input)
            if not self.cap.isOpened():
                raise RuntimeError(f"Не удалось открыть видео: {abs_input}")
            
            fps_val = self.cap.get(cv2.CAP_PROP_FPS)
            self.fps = fps_val if fps_val > 0 else 30.0
            self.resolution = (int(self.cap.get(cv2.CAP_PROP_FRAME_WIDTH)),
                               int(self.cap.get(cv2.CAP_PROP_FRAME_HEIGHT)))
            print(f"Видео: {self.resolution} @ {self.fps:.1f} FPS")

        if output_path:
            abs_output = os.path.abspath(output_path)
            is_mac = platform.system() == "Darwin"
            
            codecs_priority = [
                ('avc1', abs_output) if is_mac else ('mp4v', abs_output),
                ('H264', abs_output),
                ('mp4v', abs_output),
                ('XVID', os.path.splitext(abs_output)[0] + ".avi"),
                ('MJPG', os.path.splitext(abs_output)[0] + ".avi")
            ]

            success = False
            for fourcc_code, out_file in codecs_priority:
                fourcc = cv2.VideoWriter_fourcc(*fourcc_code)
                self.writer = cv2.VideoWriter(out_file, fourcc, self.fps, self.resolution)
                if self.writer.isOpened():
                    success = True
                    print(f"VideoWriter создан: '{fourcc_code}' -> {out_file}")
                    break
                self.writer.release()
                
            if not success:
                raise RuntimeError("Не удалось создать VideoWriter. На macOS рекомендуется установить 'opencv-contrib-python' или использовать кодек 'avc1'.")

    def __del__(self):
        self.release()

    def release(self):
        if self.active:
            if self.cap:
                self.cap.release()
            if self.writer:
                self.writer.release()
            self.active = False
            print("[RAII] Ресурсы видео освобождены.")

_worker_model = None

def init_worker(model_path: str):
    global _worker_model
    _worker_model = YOLO(model_path)

def process_frame(args):
    idx, frame = args
    results = _worker_model(frame, verbose=False)
    return idx, results[0].plot()

def run_single(input_path, output_path, model_path):
    vid = VideoResource(input_path=input_path, output_path=output_path)
    model = YOLO(model_path)
    total_frames = int(vid.cap.get(cv2.CAP_PROP_FRAME_COUNT))
    
    start = time.perf_counter()
    try:
        while True:
            ret, frame = vid.cap.read()
            if not ret: break
            res = model(frame, verbose=False)
            vid.writer.write(res[0].plot())
    finally:
        vid.release()
    end = time.perf_counter()
    
    return end - start, total_frames

def run_multi(input_path, output_path, model_path, num_workers=None):
    if num_workers is None:
        num_workers = mp.cpu_count()
        
    vid = VideoResource(input_path=input_path, output_path=output_path)
    total_frames = int(vid.cap.get(cv2.CAP_PROP_FRAME_COUNT))
    
    frames = []
    while True:
        ret, frame = vid.cap.read()
        if not ret: break
        frames.append(frame)
        
    start = time.perf_counter()
    processed = [None] * total_frames
    
    try:
        with ProcessPoolExecutor(max_workers=num_workers,
                                 initializer=init_worker,
                                 initargs=(model_path,)) as executor:
            futures = {executor.submit(process_frame, (i, frame)): i for i, frame in enumerate(frames)}
            for future in futures:
                idx, res_frame = future.result()
                processed[idx] = res_frame
                
        for frame in processed:
            vid.writer.write(frame)
    finally:
        vid.release()
    end = time.perf_counter()
    
    return end - start, total_frames

def main():
    parser = argparse.ArgumentParser(description="Лабораторная 5: Ускорение инференса YOLOv8s-pose")
    parser.add_argument("--video", type=str, required=True, help="Путь к видео (640x480)")
    parser.add_argument("--mode", type=str, required=True, choices=["single", "multi"], help="single или multi")
    parser.add_argument("--output", type=str, required=True, help="Имя выходного файла")
    parser.add_argument("--workers", type=int, default=None, help="Число процессов")
    parser.add_argument("--model", type=str, default="yolov8s-pose.pt", help="Модель YOLO")
    parser.add_argument("--realtime", action="store_true", help="RealTime с камеры (+3 балла)")
    
    args = parser.parse_args()
        
    if args.mode == "single":
        exec_time, total = run_single(args.video, args.output, args.model)
    else:
        exec_time, total = run_multi(args.video, args.output, args.model, args.workers)
        
    print(f"\nОбработано кадров: {total}")
    print(f"Время выполнения: {exec_time:.4f} сек")
    print(f"Средняя скорость: {total/exec_time:.2f} FPS")

if __name__ == "__main__":
    main()