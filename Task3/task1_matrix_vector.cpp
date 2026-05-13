#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <functional>

using namespace std;
using namespace chrono;

// Параллельная инициализация вектора
template<typename T>
void parallel_init_vector(vector<T>& vec, function<T(size_t)> generator, size_t num_threads) {
    size_t n = vec.size();
    size_t chunk = n / num_threads;
    
    vector<thread> threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = (t == num_threads - 1) ? n : start + chunk;
        threads.emplace_back([&vec, &generator, start, end]() {
            for (size_t i = start; i < end; ++i)
                vec[i] = generator(i);
        });
    }
    for (auto& th : threads) th.join();
}

// Параллельная инициализация матрицы (плоское представление)
template<typename T>
void parallel_init_matrix(vector<T>& mat, size_t rows, size_t cols, 
                         function<T(size_t, size_t)> generator, size_t num_threads) {
    size_t total = rows * cols;
    size_t chunk = total / num_threads;
    
    vector<thread> threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk;
        size_t end = (t == num_threads - 1) ? total : start + chunk;
        threads.emplace_back([&mat, rows, cols, &generator, start, end]() {
            for (size_t idx = start; idx < end; ++idx) {
                size_t i = idx / cols;
                size_t j = idx % cols;
                mat[idx] = generator(i, j);
            }
        });
    }
    for (auto& th : threads) th.join();
}

// Параллельное умножение матрицы на вектор: result = A * x
void mat_vec_mult_parallel(const vector<double>& A, const vector<double>& x, 
                          vector<double>& result, size_t rows, size_t cols, size_t num_threads) {
    result.resize(rows);
    size_t chunk = rows / num_threads;
    
    vector<thread> threads;
    for (size_t t = 0; t < num_threads; ++t) {
        size_t row_start = t * chunk;
        size_t row_end = (t == num_threads - 1) ? rows : row_start + chunk;
        
        threads.emplace_back([&A, &x, &result, cols, row_start, row_end]() {
            for (size_t i = row_start; i < row_end; ++i) {
                double sum = 0.0;
                for (size_t j = 0; j < cols; ++j)
                    sum += A[i * cols + j] * x[j];
                result[i] = sum;
            }
        });
    }
    for (auto& th : threads) th.join();
}

// Замер времени выполнения
double measure_time(function<void()> func) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    return duration<double>(end - start).count();
}

int main(int argc, char* argv[]) {
    // Параметры по умолчанию
    size_t N = 20000;  // Размер матрицы N x N
    vector<size_t> thread_counts = {1, 2, 4, 7, 8, 16}; // Можно добавить 20, 40
    
    // Парсинг аргументов (опционально)
    if (argc > 1) N = stoull(argv[1]);
    if (argc > 2) {
        thread_counts.clear();
        for (int i = 2; i < argc; ++i)
            thread_counts.push_back(stoul(argv[i]));
    }
    
    cout << fixed << setprecision(4);
    cout << "Matrix size: " << N << "x" << N << "\n";
    cout << "Threads\tTime(s)\tSpeedup\n";
    cout << "---------------------------\n";
    
    double base_time = 0;
    
    for (size_t num_threads : thread_counts) {
        if (num_threads > thread::hardware_concurrency()) {
            cout << num_threads << "\t(skipped, > available cores)\n";
            continue;
        }
        
        vector<double> A(N * N), x(N), result;
        
        // Замер времени
        double total_time = measure_time([&]() {
            // Параллельная инициализация
            parallel_init_matrix(A, N, N, 
                [](size_t i, size_t j) { return sin(i + j) * 0.01; }, 
                num_threads);
            parallel_init_vector(x, 
                [](size_t i) { return cos(i) * 0.01; }, 
                num_threads);
            
            // Параллельное умножение
            mat_vec_mult_parallel(A, x, result, N, N, num_threads);
        });
        
        // Вычисление ускорения
        if (base_time == 0) base_time = total_time;
        double speedup = base_time / total_time;
        
        cout << num_threads << "\t" << total_time << "\t" << speedup << "\n";
        
        // Проверка корректности (первый элемент)
        if (num_threads == 1) {
            cout << "Sample result[0]: " << result[0] << "\n\n";
        }
    }
    
    return 0;
}