#include <iostream>
#include <omp.h>
#include <chrono>
#include <cmath>

double f(double x) {
    return 4.0 / (1.0 + x * x);
}

double integrate_omp_atomic(long long nsteps, int num_threads) {
    omp_set_num_threads(num_threads);
    double step = 1.0 / nsteps;
    double sum = 0.0;

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel for
    for (long long i = 0; i < nsteps; ++i) {
        double x = (i + 0.5) * step;
        double val = f(x) * step;
#pragma omp atomic
        sum += val;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    std::cout << "Atomic time: " << dur.count() << " s\n";

    return sum;
}

double integrate_omp_local(long long nsteps, int num_threads) {
    omp_set_num_threads(num_threads);
    double step = 1.0 / nsteps;
    double sum = 0.0;

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
        sum += local_sum;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    std::cout << "Local time: " << dur.count() << " s\n";

    return sum;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: ./program num_threads\n";
        return 1;
    }
    int num_threads = std::stoi(argv[1]);
    long long nsteps = 40000000;

    double pi_atomic = integrate_omp_atomic(nsteps, num_threads);
    std::cout << "Pi (atomic): " << pi_atomic << "\n";

    double pi_local = integrate_omp_local(nsteps, num_threads);
    std::cout << "Pi (local): " << pi_local << "\n";

    return 0;
}