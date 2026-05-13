// task2_client_server.cpp
#include <iostream>
#include <queue>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;
using namespace chrono;

// ============================================================================
// Шаблонный класс сервера задач (на основе std::packaged_task)
// Паттерн из example_add_package.cpp
// ============================================================================
template<typename T>
class TaskServer {
private:
    // Очередь хранит только packaged_task (выполнение)
    queue<pair<size_t, packaged_task<T()>>> task_queue;
    
    // Futures хранятся отдельно для получения результатов
    unordered_map<size_t, future<T>> futures;
    
    // Готовые результаты (после выполнения задачи)
    unordered_map<size_t, T> completed_results;
    
    mutex mut;
    condition_variable cond_var;
    thread server_thread;
    bool running = false;
    size_t next_task_id = 1;
    
    void server_loop() {
        while (true) {
            unique_lock<mutex> lock(mut);
            
            // Ждём задачу или сигнал остановки
            cond_var.wait(lock, [this] { 
                return !task_queue.empty() || !running; 
            });
            
            if (!running && task_queue.empty()) 
                break;
            
            if (!task_queue.empty()) {
                // Извлекаем задачу
                auto [id, task] = move(task_queue.front());
                task_queue.pop();
                
                // Освобождаем мьютекс ПЕРЕД выполнением задачи
                lock.unlock();
                
                // Выполняем задачу - это заполняет associated future
                task();
                
                // Результат теперь доступен через futures[id].get()
                // Но мы не храним future в server_loop, поэтому просто помечаем как готовое
                // Клиент сам получит результат через request_result
            }
        }
        cout << "[Server] stopped\n";
    }
    
public:
    void start() {
        running = true;
        server_thread = thread(&TaskServer::server_loop, this);
    }
    
    void stop() {
        {
            lock_guard<mutex> lock(mut);
            running = false;
        }
        cond_var.notify_all();
        if (server_thread.joinable()) 
            server_thread.join();
    }
    
    size_t add_task(function<T()> task_func) {
        // Создаём packaged_task
        packaged_task<T()> task(task_func);
        
        // Получаем future ДО перемещения task
        future<T> fut = task.get_future();
        
        size_t id = next_task_id++;
        
        {
            lock_guard<mutex> lock(mut);
            // Сохраняем future для последующего получения результата
            futures[id] = move(fut);
            // Добавляем задачу в очередь
            task_queue.push({id, move(task)});
        }
        
        // Уведомляем сервер о новой задаче
        cond_var.notify_one();
        return id;
    }
    
    T request_result(size_t id, bool blocking = true) {
        if (blocking) {
            // Ждём, пока задача не выполнится и future не станет ready
            while (true) {
                {
                    unique_lock<mutex> lock(mut);
                    
                    // Проверяем, есть ли уже готовый результат
                    auto it = completed_results.find(id);
                    if (it != completed_results.end()) {
                        T res = it->second;
                        completed_results.erase(it);
                        futures.erase(id);
                        return res;
                    }
                    
                    // Проверяем, готов ли future
                    auto fit = futures.find(id);
                    if (fit != futures.end() && fit->second.wait_for(chrono::milliseconds(0)) 
                        == future_status::ready) {
                        T res = fit->second.get(); // get() можно вызвать только один раз!
                        futures.erase(fit);
                        return res;
                    }
                }
                // Небольшая пауза чтобы не грузить CPU
                this_thread::sleep_for(milliseconds(10));
            }
        } else {
            // Неблокирующий режим
            lock_guard<mutex> lock(mut);
            
            auto it = completed_results.find(id);
            if (it != completed_results.end()) {
                T res = it->second;
                completed_results.erase(it);
                futures.erase(id);
                return res;
            }
            
            auto fit = futures.find(id);
            if (fit != futures.end() && fit->second.wait_for(chrono::milliseconds(0)) 
                == future_status::ready) {
                T res = fit->second.get();
                futures.erase(fit);
                return res;
            }
            
            throw runtime_error("Result not ready");
        }
    }
    
    ~TaskServer() { 
        if (running) stop(); 
    }
};

// ============================================================================
// Функции задач (все принимают один double для унификации)
// ============================================================================
double compute_sin(double x) {
    this_thread::sleep_for(milliseconds(5)); // имитация работы
    return sin(x);
}

double compute_sqrt(double x) {
    this_thread::sleep_for(milliseconds(5));
    return sqrt(abs(x));
}

double compute_pow(double x) {
    // Кодируем два аргумента: основание = |x|, степень = 2..5
    this_thread::sleep_for(milliseconds(5));
    double base = abs(x);
    double exp = 2.0 + fmod(abs(x) * 10, 3.0);
    return pow(base, exp);
}

// ============================================================================
// Клиент: добавляет задачи и сохраняет результаты в файл
// ============================================================================
void client_thread(function<double(double)> task_func, 
                   int n_tasks, 
                   const string& filename,
                   TaskServer<double>& server,
                   const string& task_name) {
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dist(-100.0, 100.0);
    
    ofstream out(filename);
    out << "# Task: " << task_name << "\n";
    out << "# Format: input -> result\n\n";
    out << fixed << setprecision(6);
    
    for (int i = 0; i < n_tasks; ++i) {
        double arg = dist(gen);
        
        // Создаём задачу с захватом аргумента
        auto task = [task_func, arg]() -> double {
            return task_func(arg);
        };
        
        // Добавляем задачу на сервер
        size_t id = server.add_task(task);
        
        // Ждём и получаем результат
        double result = server.request_result(id);
        
        // Записываем в файл
        out << task_name << "(" << arg << ") = " << result << "\n";
    }
    out.close();
    cout << "[Client] " << task_name << ": " << n_tasks << " tasks -> " << filename << "\n";
}

// ============================================================================
// Тест: проверка результатов из файлов
// ============================================================================
bool verify_line(const string& line) {
    size_t eq = line.find('=');
    if (eq == string::npos) return false;
    
    string rhs = line.substr(eq + 1);
    rhs.erase(0, rhs.find_first_not_of(" \t"));
    
    try {
        double expected = stod(rhs);
        
        size_t p1 = line.find('('), p2 = line.find(')');
        if (p1 == string::npos || p2 == string::npos) return true;
        
        string func = line.substr(0, p1);
        double arg = stod(line.substr(p1 + 1, p2 - p1 - 1));
        
        double verified = 0;
        if (func.find("sin") != string::npos) {
            verified = sin(arg);
        } else if (func.find("sqrt") != string::npos) {
            verified = sqrt(abs(arg));
        } else if (func.find("pow") != string::npos) {
            double base = abs(arg);
            double exp = 2.0 + fmod(abs(arg) * 10, 3.0);
            verified = pow(base, exp);
        } else {
            return true;
        }
        
        return abs(expected - verified) < 1e-4;
    } catch (...) {
        return false;
    }
}

bool test_results(const vector<string>& filenames) {
    cout << "\n=== Проверка результатов ===\n";
    bool all_passed = true;
    
    for (const string& fname : filenames) {
        ifstream in(fname);
        if (!in.is_open()) {
            cout << "[FAIL] Cannot open " << fname << "\n";
            all_passed = false;
            continue;
        }
        
        string line;
        int tested = 0, passed = 0;
        
        while (getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            tested++;
            if (verify_line(line)) passed++;
        }
        
        bool ok = (tested > 0 && passed == tested);
        cout << "[" << (ok ? "PASS" : "FAIL") << "] " << fname 
             << ": " << passed << "/" << tested << " verified\n";
        if (!ok) all_passed = false;
    }
    
    return all_passed;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    cout << "=== Клиент-сервер: асинхронные вычисления ===\n\n";
    
    const int N = 50; // 5 < N < 10000
    
    TaskServer<double> server;
    server.start();
    
    // Три клиента с разными задачами
    thread client1(client_thread, compute_sin, N, "results_sin.txt", ref(server), "sin");
    thread client2(client_thread, compute_sqrt, N, "results_sqrt.txt", ref(server), "sqrt");
    thread client3(client_thread, compute_pow, N, "results_pow.txt", ref(server), "pow");
    
    client1.join();
    client2.join();
    client3.join();
    
    server.stop();
    
    // Тест
    bool ok = test_results({"results_sin.txt", "results_sqrt.txt", "results_pow.txt"});
    
    cout << "\n=== Итог: " << (ok ? "Все тесты пройдены ✓" : "Есть ошибки ✗") << " ===\n";
    return ok ? 0 : 1;
}