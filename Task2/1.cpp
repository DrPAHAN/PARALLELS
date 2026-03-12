#include <iostream>
#include <omp.h>
#include <chrono>
#include <cmath>

int main(int argc, char** argv) {
    #pragma omp parallel
    {
        #pragma omp single
        std::cout << "OpenMP работает! Потоков: " << omp_get_num_threads() << "\n";
    }
    
    if (argc < 3) {
        std::cout << "Usage: ./program N num_threads\n";
        return 1;
    }
    long long N = std::stoll(argv[1]);
    int num_threads = std::stoi(argv[2]);

    omp_set_num_threads(num_threads);

    double* A = new double[N * N];
    double* x = new double[N];
    double* y = new double[N];

    auto start_init = std::chrono::high_resolution_clock::now();

    // Параллельная инициализация A (например, A[i][j] = sin(i + j))
#pragma omp parallel for collapse(2)
    for (long long i = 0; i < N; ++i) {
        for (long long j = 0; j < N; ++j) {
            A[i * N + j] = std::sin(static_cast<double>(i + j));
        }
    }

    // Параллельная инициализация x
#pragma omp parallel for
    for (long long i = 0; i < N; ++i) {
        x[i] = std::cos(static_cast<double>(i));
    }

    auto end_init = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur_init = end_init - start_init;
    std::cout << "Init time: " << dur_init.count() << " s\n";

    auto start = std::chrono::high_resolution_clock::now();

    // Параллельное умножение A * x = y
#pragma omp parallel for
    for (long long i = 0; i < N; ++i) {
        double sum = 0.0;
        for (long long j = 0; j < N; ++j) {
            sum += A[i * N + j] * x[j];
        }
        y[i] = sum;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    std::cout << "Computation time: " << dur.count() << " s\n";

    delete[] A;
    delete[] x;
    delete[] y;

    return 0;
}