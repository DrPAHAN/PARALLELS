#include <iostream>
#include <omp.h>
#include <chrono>
#include <iomanip>

double f(double x) {
    return 4.0 / (1.0 + x * x);
}

// Вариант 1: с использованием atomic
double integrate_omp_atomic(long long nsteps, int num_threads) {
    omp_set_num_threads(num_threads);
    double step = 1.0 / nsteps;
    double sum = 0.0;

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel for
    for (long long i = 0; i < nsteps; ++i) {
        double x = (i + 0.5) * step;
#pragma omp atomic
        sum += f(x) * step;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Atomic version, threads = " << num_threads 
              << ", time = " << elapsed.count() << " s, pi ≈ " 
              << std::fixed << std::setprecision(10) << sum << std::endl;

    return elapsed.count();
}

// Вариант 2: с локальной переменной (рекомендуется — быстрее)
double integrate_omp_local(long long nsteps, int num_threads) {
    omp_set_num_threads(num_threads);
    double step = 1.0 / nsteps;
    double global_sum = 0.0;

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel
    {
        double local_sum = 0.0;

#pragma omp for
        for (long long i = 0; i < nsteps; ++i) {
            double x = (i + 0.5) * step;
            local_sum += f(x) * step;
        }

#pragma omp critical
        global_sum += local_sum;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    std::cout << "Local version, threads = " << num_threads 
              << ", time = " << elapsed.count() << " s, pi ≈ " 
              << std::fixed << std::setprecision(10) << global_sum << std::endl;

    return elapsed.count();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./2 <num_threads>\n";
        return 1;
    }

    int threads = std::stoi(argv[1]);
    long long nsteps = 40'000'000LL;

    std::cout << "=== Численное интегрирование (nsteps = " << nsteps 
              << ", threads = " << threads << ") ===\n\n";

    // Замеряем обе версии
    std::cout << "1. Версия с #pragma omp atomic:\n";
    double time_atomic = integrate_omp_atomic(nsteps, threads);

    std::cout << "\n2. Версия с локальной переменной (рекомендуется):\n";
    double time_local = integrate_omp_local(nsteps, threads);

    // Выводим сравнение
    std::cout << "\n=== Сравнение ===\n";
    std::cout << "Atomic time : " << time_atomic << " s\n";
    std::cout << "Local  time : " << time_local  << " s\n";
    if (time_local > 0)
        std::cout << "Local быстрее atomic в " 
                  << (time_atomic / time_local) << " раз\n";

    return 0;
}