#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <omp.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: ./3_v2 N num_threads\n";
        return 1;
    }

    int N = std::stoi(argv[1]);
    int threads = std::stoi(argv[2]);
    omp_set_num_threads(threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> x_new(N);
    std::vector<double> b(N, N + 1.0);

    double tau = 0.5 / N;
    double eps = 1e-6;
    int max_iter = 100000;
    int iter = 0;

    auto start = std::chrono::high_resolution_clock::now();

#pragma omp parallel
    {
        bool converged = false;
        while (!converged && iter < max_iter) {
            double sum_all_x = 0.0;
            double norm_res = 0.0;

#pragma omp for reduction(+:sum_all_x)
            for (int i = 0; i < N; ++i) {
                sum_all_x += x[i];
            }

#pragma omp for reduction(+:norm_res)
            for (int i = 0; i < N; ++i) {
                double Ax = sum_all_x + x[i];
                double res = Ax - b[i];
                norm_res += res * res;
                x_new[i] = x[i] - tau * res;
            }

#pragma omp single
            {
                norm_res = std::sqrt(norm_res);
                if (norm_res / (N + 1.0) < eps) {
                    converged = true;
                }
                ++iter;
            }

#pragma omp barrier

            if (converged) break;

#pragma omp for
            for (int i = 0; i < N; ++i) {
                x[i] = x_new[i];
            }

#pragma omp barrier
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(end - start).count();

    std::cout << "V2 (one parallel region) | N=" << N 
              << " | threads=" << threads 
              << " | time=" << time << " s | iterations=" << iter << std::endl;

    return 0;
}