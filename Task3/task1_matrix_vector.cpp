#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace chrono;

// Параллельная инициализация вектора (универсальный callable)
template<typename T, typename Generator>
void parallel_init_vector(vector<T>& vec, Generator&& generator, size_t num_threads) {
    size_t n = vec.size();
    if (n == 0) return;
    size_t chunk = n / num_threads;
    
    vector<thread> threads;
    threads.reserve(num_threads);
    
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

// Параллельная инициализация матрицы (универсальный callable)
template<typename T, typename Generator>
void parallel_init_matrix(vector<T>& mat, size_t rows, size_t cols, 
                         Generator&& generator, size_t num_threads) {
    if (mat.size() != rows * cols) mat.resize(rows * cols);
    size_t total = rows * cols;
    if (total == 0) return;
    size_t chunk = total / num_threads;
    
    vector<thread> threads;
    threads.reserve(num_threads);
    
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
    result.resize(rows, 0.0);
    if (rows == 0) return;
    
    size_t chunk = rows / num_threads;
    if (chunk == 0) chunk = 1;
    
    vector<thread> threads;
    for (size_t t = 0; t < num_threads && t * chunk < rows; ++t) {
        size_t row_start = t * chunk;
        size_t row_end = min((t + 1) * chunk, rows);
        
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
    size_t N = 20000;
    vector<size_t> thread_counts = {1, 2, 4, 7, 8}; // 16, 20, 40 - при наличии ядер
    
    if (argc > 1) N = stoull(argv[1]);
    
    cout << fixed << setprecision(4);
    cout << "Matrix size: " << N << "x" << N << "\n";
    cout << "Available cores: " << thread::hardware_concurrency() << "\n";
    cout << "Threads\tTime(s)\tSpeedup\n";
    cout << "---------------------------\n";
    
    double base_time = 0;
    
    for (size_t num_threads : thread_counts) {
        size_t max_threads = thread::hardware_concurrency();
        if (max_threads > 0 && num_threads > max_threads) {
            cout << num_threads << "\t(skipped)\n";
            continue;
        }
        
        vector<double> A(N * N), x(N), result;
        
        double total_time = measure_time([&]() {
            // Инициализация с явным указанием типа <double>
            parallel_init_matrix<double>(A, N, N, 
                [](size_t i, size_t j) -> double { return sin(double(i + j)) * 0.01; }, 
                num_threads);
            parallel_init_vector<double>(x, 
                [](size_t i) -> double { return cos(double(i)) * 0.01; }, 
                num_threads);
            
            mat_vec_mult_parallel(A, x, result, N, N, num_threads);
        });