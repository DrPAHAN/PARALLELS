#include <iostream>
#include <omp.h>
#include <chrono>
#include <cmath>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: ./program N num_threads\n";
        return 1;
    }
    int N = std::stoi(argv[1]);
    int num_threads = std::stoi(argv[2]);
    omp_set_num_threads(num_threads);

    std::vector<double> x(N, 0.0);
    std::vector<double> new_x(N);
    std::vector<double> b(N, N + 1.0);

    double tau = 0.01;
    double eps = 1e-5;
    double norm_b = std::sqrt(N * std::pow(N + 1.0, 2));

    auto start = std::chrono::high_resolution_clock::now();
    int iter = 0;
    bool converged = false;
    while (!converged && iter < 100000) {  // Max итераций для безопасности
        double sum_x = 0.0;
#pragma omp parallel for reduction(+:sum_x)
        for (int i = 0; i < N; ++i) {
            sum_x += x[i];
        }

        double sum_sq = 0.0;
#pragma omp parallel for reduction(+:sum_sq)
        for (int i = 0; i < N; ++i) {
            double ax = sum_x + x[i];
            double res = ax - b[i];
            sum_sq += res * res;
        }
        double norm_res = std::sqrt(sum_sq);
        if (norm_res / norm_b <= eps) {
            converged = true;
            break;
        }

#pragma omp parallel for
        for (int i = 0; i < N; ++i) {
            double ax = sum_x + x[i];
            double res = ax - b[i];
            new_x[i] = x[i] - tau * res;
        }

#pragma omp parallel for
        for (int i = 0; i < N; ++i) {
            x[i] = new_x[i];
        }
        ++iter;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> dur = end - start;
    std::cout << "Time: " << dur.count() << " s, Iter: " << iter << "\n";

    return 0;
}