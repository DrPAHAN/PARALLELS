#include <iostream>
#include <vector>
#include <chrono>
#include <omp.h>

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: ./3_v2 N threads iterations\n";
        return 1;
    }

    long long N = std::stoll(argv[1]);
    int threads = std::stoi(argv[2]);
    long long iterations = std::stoll(argv[3]);

    omp_set_num_threads(threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    double tau = 0.5 / N;

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel
    {
        for (long long it = 0; it < iterations; ++it) {
            double sum_all = 0.0;

#pragma omp for reduction(+:sum_all)
            for (long long i = 0; i < N; ++i) {
                sum_all += x[i];
            }

#pragma omp for
            for (long long i = 0; i < N; ++i) {
                double Ax = sum_all + x[i];
                double res = Ax - (N + 1.0);
                x_new[i] = x[i] - tau * res;
            }

#pragma omp for
            for (long long i = 0; i < N; ++i) {
                x[i] = x_new[i];
            }

#pragma omp barrier
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    std::cout << "V2 (one big parallel region) | N=" << N 
              << " | threads=" << threads 
              << " | iterations=" << iterations 
              << " | time=" << time << " s" << std::endl;

    return 0;
}