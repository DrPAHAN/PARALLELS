#include <iostream>
#include <omp.h>
#include <chrono>
#include <vector>
#include <cmath>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: N threads\n";
        return 1;
    }

    long long N = std::stoll(argv[1]);
    int threads = std::stoi(argv[2]);

    omp_set_num_threads(threads);

    // Выделяем память
    std::vector<double> A(N * N);   
    std::vector<double> x(N);
    std::vector<double> y(N);


    // Заполняем матрицу A
#pragma omp parallel for collapse(2)
    for (long long i = 0; i < N; ++i)
        for (long long j = 0; j < N; ++j)
            A[i*N + j] = std::sin(static_cast<double>(i + j));

    // Заполняем вектор x
#pragma omp parallel for
    for (long long i = 0; i < N; ++i)
        x[i] = std::cos(static_cast<double>(i));

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel for
    for (long long i = 0; i < N; ++i) {
        double sum = 0.0;
        for (long long j = 0; j < N; ++j) {
            sum += A[i*N + j] * x[j];
        }
        y[i] = sum;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    std::cout << "N = " << N << ", threads = " << threads 
              << ", Computation time = " << time << " s\n";

    return 0;
}