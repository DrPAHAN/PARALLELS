#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <algorithm>

void parallel_init(std::vector<double>& A, std::vector<double>& B, int N, int num_threads) {
    std::vector<std::jthread> threads;
    int chunk = (N + num_threads - 1) / num_threads;

    auto init_chunk = [&](int start, int end, bool is_matrix) {
        for (int i = start; i < end; ++i) {
            if (is_matrix) {
                for (int j = 0; j < N; ++j) A[i * N + j] = static_cast<double>(rand()) / RAND_MAX;
            } else {
                B[i] = static_cast<double>(rand()) / RAND_MAX;
            }
        }
    };

    for (int t = 0; t < num_threads; ++t) {
        int start = t * chunk;
        int end = std::min(start + chunk, N);
        if (start >= N) break;
        threads.emplace_back(init_chunk, start, end, true);
    }
    for (int t = 0; t < num_threads; ++t) {
        int start = t * chunk;
        int end = std::min(start + chunk, N);
        if (start >= N) break;
        threads.emplace_back(init_chunk, start, end, false);
    }
}

void parallel_multiply(const std::vector<double>& A, const std::vector<double>& B, std::vector<double>& C, int N, int num_threads) {
    std::vector<std::jthread> threads;
    int chunk = (N + num_threads - 1) / num_threads;

    for (int t = 0; t < num_threads; ++t) {
        int start = t * chunk;
        int end = std::min(start + chunk, N);
        if (start >= N) break;
        threads.emplace_back([&]() {
            for (int i = start; i < end; ++i) {
                double sum = 0.0;
                for (int j = 0; j < N; ++j) sum += A[i * N + j] * B[j];
                C[i] = sum;
            }
        });
    }
}

int main() {
    std::ofstream csv("scalability_results.csv");
    csv << "Threads,Time_20000_ms,Time_40000_ms\n";

    std::vector<int> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};
    std::vector<int> sizes = {20000, 40000};

    for (int N : sizes) {
        std::cout << "Размер: " << N << "x" << N << "\n";
        for (int threads : thread_counts) {
            std::vector<double> A(N * N), B(N), C(N);
            
            auto start = std::chrono::high_resolution_clock::now();
            parallel_init(A, B, N, threads);
            parallel_multiply(A, B, C, N, threads);
            auto end = std::chrono::high_resolution_clock::now();

            double ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
            std::cout << "  Потоков: " << threads << " | Время: " << std::fixed << std::setprecision(2) << ms << " мс\n";

            if (N == 20000) csv << threads << "," << std::fixed << std::setprecision(2) << ms << ",";
            else csv << std::fixed << std::setprecision(2) << ms << "\n";
        }
    }
    csv.close();
    std::cout << "Данные сохранены в scalability_results.csv\n";
    return 0;
}