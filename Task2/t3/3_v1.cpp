#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: ./simple_iter_v1 N threads num_iterations\n";
        return 1;
    }

    long long N = std::stoll(argv[1]);
    int threads = std::stoi(argv[2]);
    long long num_iter = std::stoll(argv[3]);

    omp_set_num_threads(threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    double tau = 0.5 / N;

    auto t_start = std::chrono::high_resolution_clock::now();

    for (long long it = 0; it < num_iter; ++it) {
        double sum_all = 0.0;

        // 1. Суммируем все элементы x
#pragma omp parallel for reduction(+:sum_all)
        for (long long i = 0; i < N; ++i) {
            sum_all += x[i];
        }

        // 2. Вычисляем новое приближение
#pragma omp parallel for
        for (long long i = 0; i < N; ++i) {
            double Ax = sum_all + x[i];                 // A*x (диагональ 2, остальные 1)
            double res = Ax - (N + 1.0);
            x_new[i] = x[i] - tau * res;
        }

        // 3. Копируем новое приближение обратно
#pragma omp parallel for
        for (long long i = 0; i < N; ++i) {
            x[i] = x_new[i];
        }
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(t_end - t_start).count();

    std::cout << "V1 (separate parallel for) | N=" << N 
              << " | threads=" << threads 
              << " | iterations=" << num_iter 
              << " | time=" << time << " s" << std::endl;

    return 0;
}