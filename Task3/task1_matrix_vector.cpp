#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <algorithm>

using namespace std;  
using namespace chrono;


void parallel_init(vector<double>& arr, double value, int num_threads) {
    size_t n = arr.size();
    if (n == 0) return;

    size_t chunk = (n + num_threads - 1) / num_threads;
    vector<thread> threads;
    threads.reserve(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = min(start + chunk, n);
        if (start >= n) break;
        threads.emplace_back([&arr, value, start, end]() { 
            for (size_t i = start; i < end; ++i)
                arr[i] = value;
        });
    }
    for (auto& th : threads)
        if (th.joinable()) th.join();
}

double multiply_row(const vector<double>& matrix, const vector<double>& vec, 
                   size_t row, size_t n) {
    double sum = 0;
    size_t start = row * n;
    for (size_t j = 0; j < n; ++j) 
        sum += matrix[start + j] * vec[j];
    return sum;
}

vector<double> mat_vec_mult(const vector<double>& matrix, const vector<double>& vec, 
                           size_t n, int num_threads) {
    vector<double> result(n, 0);
    if (n == 0) return result;
    
    size_t rows_per_thread = (n + num_threads - 1) / num_threads;
    vector<thread> threads;
    threads.reserve(num_threads);
    
    for (int t = 0; t < num_threads; ++t) {
        size_t row_start = t * rows_per_thread;
        size_t row_end = min(row_start + rows_per_thread, n);
        if (row_start >= n) break;
        
        threads.emplace_back([&matrix, &vec, &result, n, row_start, row_end]() {
            for (size_t row = row_start; row < row_end; ++row) {
                result[row] = multiply_row(matrix, vec, row, n);
            }
        });
    }
    for (auto& th : threads) 
        if (th.joinable()) th.join();
    
    return result;
}

double benchmark(int num_threads, size_t n) {
    vector<double> matrix;
    vector<double> vector_a;
    vector<double> result;
    
    try {
        matrix.resize(n * n);
        vector_a.resize(n);
        result.resize(n);
    } catch (const bad_alloc&) {
        cerr << "Недостаточно памяти для матрицы " << n << "x" << n << "\n";
        return -1.0;
    }
    
    parallel_init(matrix, 1.5, num_threads);
    parallel_init(vector_a, 2.0, num_threads);
    
    auto start = high_resolution_clock::now();
    result = mat_vec_mult(matrix, vector_a, n, num_threads);
    auto end = high_resolution_clock::now();
    
    return duration<double>(end - start).count();
}

int main() {
    cout << fixed << setprecision(4);
    cout << "Анализ масштабируемости: умножение матрицы на вектор\n\n";
    
    vector<int> thread_counts = {1, 2, 4, 7, 8, 16, 20, 40};
    vector<size_t> sizes = {20000, 40000};
    
    for (size_t n : sizes) {
        cout << "Размер матрицы: " << n << "x" << n << "\n";
        cout << string(70, '-') << "\n";
        cout << left << setw(10) << "Threads" 
             << setw(15) << "Time (s)" 
             << setw(15) << "Speedup" 
             << setw(15) << "Efficiency" << "\n";
        cout << string(70, '-') << "\n";
        
        double base_time = 0;
        
        for (int threads : thread_counts) {
            int max_threads = thread::hardware_concurrency();
            if (max_threads == 0) max_threads = 8;
            int actual_threads = min(threads, max(1, max_threads));
            
            double time = benchmark(actual_threads, n);
            if (time < 0) {
                cout << left << setw(10) << threads << "SKIPPED (OOM)\n";
                continue;
            }
            
            if (threads == 1) base_time = time;
            
            double speedup = (base_time > 0) ? base_time / time : 0;
            double efficiency = (threads > 0) ? (speedup / threads * 100) : 0;
            
            cout << left << setw(10) << threads 
                 << setw(15) << time 
                 << setw(15) << speedup 
                 << setw(14) << efficiency << "%\n";
        }
        cout << "\n";
    }
    return 0;
}