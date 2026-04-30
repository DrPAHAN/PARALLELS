#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: ./simple_iter_v1 N num_threads\n";
        return 1;
    }

    int N = std::stoi(argv[1]);
    int threads = std::stoi(argv[2]);
    omp_set_num_threads(threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    std::vector<double> b(N, N + 1.0);

    double tau = 0.5 / N;        // хороший параметр для сходимости
    double eps = 1e-6;
    int max_iter = 100000;
    int iter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (iter < max_iter) {
        // Вычисляем Ax
        double norm_res = 0.0;
        double sum_all_x = 0.0;

#pragma omp parallel for reduction(+:sum_all_x)
        for (int i = 0; i < N; ++i) {
            sum_all_x += x[i];
        }

#pragma omp parallel for reduction(+:norm_res)
        for (int i = 0; i < N; ++i) {
            double Ax = sum_all_x + x[i];        // потому что A[i][j] = 1 для j!=i, 2 для j==i
            double res = Ax - b[i];
            norm_res += res * res;
            x_new[i] = x[i] - tau * res;
        }

        norm_res = std::sqrt(norm_res);
        if (norm_res / (N + 1.0) < eps) break;

        // Копируем x_new -> x
#pragma omp parallel for
        for (int i = 0; i < N; ++i) {
            x[i] = x_new[i];
        }

        ++iter;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    std::cout << "V1 (separate parallel for) | N=" << N 
              << " | threads=" << threads 
              << " | time=" << time << " s | iterations=" << iter << std::endl;

    return 0;
}