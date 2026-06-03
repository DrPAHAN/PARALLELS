# Лабораторная работа 6

Решается стационарное уравнение теплопроводности на сетке `N x N` методом Якоби с пятиточечным шаблоном. Границы заполнены линейной интерполяцией между углами `10, 20, 30, 20`, внутренняя область изначально равна нулю. Результирующая матрица сохраняется в текстовый файл.

## Сборка

```bash
mkdir build && cd build
cmake ..
make
```

## Проверка корректности (матрицы 10×10 и 13×13)
```
export ACC_DEVICE_TYPE=gpu
./task6 -n 10 --eps=1e-6 --max-iter=1000000 -o result_10.txt --print
./task6 -n 13 --eps=1e-6 --max-iter=1000000 -o result_13.txt --print
```

## Замер производительности (CPU и GPU)
```
# Одно ядро CPU
export ACC_DEVICE_TYPE=host
./task6 -n 512 --eps=1e-6 --max-iter=1000000 -o cpu_host_512.txt

# Многоядерный CPU (использует все доступные ядра)
export ACC_DEVICE_TYPE=multicore
./task6 -n 512 --eps=1e-6 --max-iter=1000000 -o cpu_multi_512.txt

# GPU (графический ускоритель)
export ACC_DEVICE_TYPE=gpu
./task6 -n 512 --eps=1e-6 --max-iter=1000000 -o gpu_512.txt
```
