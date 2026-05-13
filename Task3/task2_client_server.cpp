#include <iostream>
#include <queue>
#include <future>
#include <thread>
#include <chrono>
#include <cmath>
#include <random>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <fstream>
#include <functional>
#include <atomic>

using namespace std;
using namespace chrono;

// ============================================================================
// Шаблонный класс сервера задач
// ============================================================================
template<typename T>
class TaskServer {
private:
    // Тип задачи: функция без аргументов, возвращающая T
    using Task = packaged_task<T()>;
    
    queue<pair<size_t, Task>> task_queue;           // Очередь задач с ID
    unordered_map<size_t, future<T>> pending;        // Ожидающие выполнения
    unordered_map<size_t, T> results;                // Готовые результаты
    
    mutex mut;
    condition_variable cond_var;
    atomic<bool> running{false};
    atomic<size_t> next_id{1};
    
    thread server_thread;
    
    // Основной цикл сервера
    void run() {
        while (running || !task_queue.empty()) {
            unique_lock lock(mut);
            
            cond_var.wait(lock, [this] { 
                return !task_queue.empty() || !running; 
            });
            
            if (!running && task_queue.empty()) break;
            
            if (!task_queue.empty()) {
                auto [id, task] = move(task_queue.front());
                task_queue.pop();
                lock.unlock();
                
                try {
                    T result = task();  // Выполнение задачи
                    lock.lock();
                    results[id] = move(result);
                    lock.unlock();
                } catch (...) {
                    // Обработка ошибок (опционально)
                }
            }
        }
    }
    
public:
    // 1) Запустить сервер
    void start() {
        if (running) return;
        running = true;
        server_thread = thread(&TaskServer::run, this);
    }
    
    // 2) Остановить сервер
    void stop() {
        running = false;
        cond_var.notify_all();
        if (server_thread.joinable())
            server_thread.join();
    }
    
    // 3) Добавить задачу, вернуть ID
    size_t add_task(function<T()> func) {
        size_t id = next_id++;
        Task task(func);
        future<T> fut = task.get_future();
        
        {
            lock_guard lock(mut);
            task_queue.emplace(id, move(task));
            pending[id] = move(fut);
        }
        cond_var.notify_one();
        return id;
    }
    
    // 4) Получить результат (блокирующий)
    T request_result(size_t id) {
        unique_lock lock(mut);
        
        // Если результат уже готов
        if (results.count(id)) {
            T res = move(results[id]);
            results.erase(id);
            pending.erase(id);
            return res;
        }
        
        // Ждём выполнения
        if (pending.count(id)) {
            future<T>& fut = pending[id];
            lock.unlock();
            
            T res = fut.get();  // get() вызывается только один раз!
            
            lock.lock();
            pending.erase(id);
            return res;
        }
        
        throw runtime_error("Task ID not found: " + to_string(id));
    }
    
    ~TaskServer() { stop(); }
};

// ============================================================================
// Типы задач для клиентов
// ============================================================================
struct TaskSin { double arg; };
struct TaskSqrt { double arg; };
struct TaskPow { double base, exp; };

// ============================================================================
// Клиенты
// ============================================================================
void client_sin(TaskServer<double>& server, size_t N, const string& filename) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(-M_PI, M_PI);
    
    vector<pair<size_t, double>> tasks;
    
    for (size_t i = 0; i < N; ++i) {
        double arg = dis(gen);
        size_t id = server.add_task([arg]() { return sin(arg); });
        tasks.emplace_back(id, arg);
    }
    
    // Сохранение результатов
    ofstream out(filename);
    out << "# arg\tresult\n";
    for (auto [id, arg] : tasks) {
        double res = server.request_result(id);
        out << arg << "\t" << res << "\n";
    }
    cout << "Client SIN: saved " << N << " results to " << filename << "\n";
}

void client_sqrt(TaskServer<double>& server, size_t N, const string& filename) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 10000.0);
    
    vector<pair<size_t, double>> tasks;
    
    for (size_t i = 0; i < N; ++i) {
        double arg = dis(gen);
        size_t id = server.add_task([arg]() { return sqrt(arg); });
        tasks.emplace_back(id, arg);
    }
    
    ofstream out(filename);
    out << "# arg\tresult\n";
    for (auto [id, arg] : tasks) {
        double res = server.request_result(id);
        out << arg << "\t" << res << "\n";
    }
    cout << "Client SQRT: saved " << N << " results to " << filename << "\n";
}

void client_pow(TaskServer<double>& server, size_t N, const string& filename) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis_base(0.0, 10.0);
    uniform_int_distribution<> dis_exp(0, 10);
    
    vector<tuple<size_t, double, int>> tasks;
    
    for (size_t i = 0; i < N; ++i) {
        double base = dis_base(gen);
        int exp = dis_exp(gen);
        size_t id = server.add_task([base, exp]() { return pow(base, exp); });
        tasks.emplace_back(id, base, exp);
    }
    
    ofstream out(filename);
    out << "# base\texp\tresult\n";
    for (auto [id, base, exp] : tasks) {
        double res = server.request_result(id);
        out << base << "\t" << exp << "\t" << res << "\n";
    }
    cout << "Client POW: saved " << N << " results to " << filename << "\n";
}

// ============================================================================
// Main
// ============================================================================
int main() {
    const size_t N = 100;  // Количество задач на клиента (5 < N < 10000)
    
    cout << "Starting TaskServer...\n";
    TaskServer<double> server;
    server.start();
    
    // Запуск трёх клиентов в отдельных потоках
    thread t1(client_sin, ref(server), N, "results_sin.txt");
    thread t2(client_sqrt, ref(server), N, "results_sqrt.txt");
    thread t3(client_pow, ref(server), N, "results_pow.txt");
    
    t1.join(); t2.join(); t3.join();
    
    server.stop();
    cout << "All done!\n";
    
    return 0;
}