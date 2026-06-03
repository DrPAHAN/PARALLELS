# Лабораторная работа 6: теплопроводность, OpenACC

Решается стационарное уравнение теплопроводности на сетке `N x N` методом Якоби с пятиточечным шаблоном. Границы заполнены линейной интерполяцией между углами `10, 20, 30, 20`, внутренняя область изначально равна нулю. Результирующая матрица сохраняется в текстовый файл.

## Сборка

```bash
cd build
cmake ..
make
```

Для NVHPC/OpenACC:

```bash
cmake -S . -B build-gpu -DCMAKE_CXX_COMPILER=nvc++ -DOPENACC_TARGET=gpu -DCMAKE_BUILD_TYPE=Release
cmake --build build-gpu -j
```

`OPENACC_TARGET` может быть `host`, `multicore` или `gpu`. Для старых установок можно заменить `nvc++` на `pgc++`.

## Запуск

```bash
./build/task6 --size 512 --eps 1e-6 --max-iter 1000000 --output result_512.txt
./build/task6 -n 10 -e 1e-6 -i 1000000 -p -o result_10.txt
```

Параметры: `--size/-n`, `--eps/-e`, `--max-iter/-i`, `--output/-o`, `--print/-p`.