#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: ./simple_iter_v1 N num_threads [min_iter]\n";
        return 1;
    }

    long long N = std::stoll(argv[1]);
    int threads = std::stoi(argv[2]);
    long long min_iter = (argc > 3) ? std::stoll(argv[3]) : 10000LL;

    omp_set_num_threads(threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    double tau = 0.5 / N;
    double eps = 1e-5;
    long long max_iter = 300000;
    long long iter = 0;

    auto start = std::chrono::high_resolution_clock::now();

    while (iter < max_iter) {
        double sum_all = 0.0;
        double norm_res = 0.0;

#pragma omp parallel for reduction(+:sum_all)
        for (long long i = 0; i < N; ++i) {
            sum_all += x[i];
        }

#pragma omp parallel for reduction(+:norm_res)
        for (long long i = 0; i < N; ++i) {
            double Ax = sum_all + x[i];
            double res = Ax - (N + 1.0);
            norm_res += res * res;
            x_new[i] = x[i] - tau * res;
        }

        norm_res = std::sqrt(norm_res);
        ++iter;

        // Выходим, только если прошли минимум итераций И достигли нужной точности
        if (iter >= min_iter && norm_res < eps * (N + 1)) {
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    std::cout << "V1 (separate parallel) | N=" << N 
              << " | threads=" << threads 
              << " | time=" << time << " s | iterations=" << iter 
              << " | min_iter=" << min_iter << std::endl;

    return 0;
}