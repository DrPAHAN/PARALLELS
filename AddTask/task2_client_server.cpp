#include <iostream>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>
#include <cmath>
#include <random>
#include <vector>
#include <string>
#include <iomanip>

template<typename T>
class Server {
public:
    Server() : next_id_(0) {}
    ~Server() { stop(); }

    void start() {
        stop_flag_ = false;
        server_thread_ = std::jthread([this](std::stop_token stoken) { worker_thread(stoken); });
    }

    void stop() {
        stop_flag_ = true;
        cv_queue_.notify_one();
        if (server_thread_.joinable()) server_thread_.join();
    }

    size_t add_task(std::function<T()> task) {
        size_t id;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            id = next_id_++;
            task_queue_.emplace(id, std::move(task));
        }
        cv_queue_.notify_one();
        return id;
    }

    T request_result(size_t id) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_result_.wait(lock, [this, id]() { return results_.find(id) != results_.end(); });
        T res = results_[id];
        results_.erase(id);
        return res;
    }

private:
    void worker_thread(std::stop_token stoken) {
        while (true) {
            size_t id;
            std::function<T()> task;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_queue_.wait(lock, [this, &stoken]() { return !task_queue_.empty() || stoken.stop_requested(); });
                if (stoken.stop_requested() && task_queue_.empty()) break;
                id = task_queue_.front().first;
                task = std::move(task_queue_.front().second);
                task_queue_.pop();
            }
            T res = task();
            {
                std::lock_guard<std::mutex> lock(mtx_);
                results_[id] = res;
            }
            cv_result_.notify_one();
        }
    }

    std::queue<std::pair<size_t, std::function<T()>>> task_queue_;
    std::unordered_map<size_t, T> results_; // O(1) вставка, поиск, удаление
    std::mutex mtx_;
    std::condition_variable cv_queue_, cv_result_;
    std::jthread server_thread_;
    size_t next_id_;
    std::atomic<bool> stop_flag_{false};
};

void client_thread(const std::string& name, Server<double>& srv, int N, std::function<double(double)> func, double min_val, double max_val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min_val, max_val);
    std::vector<size_t> ids(N);

    for (int i = 0; i < N; ++i) {
        double arg = dis(gen);
        ids[i] = srv.add_task([=]() { return func(arg); });
    }

    std::ofstream out(name + "_results.csv");
    out << "id,input,result\n";
    for (int i = 0; i < N; ++i) {
        double res = srv.request_result(ids[i]);
        out << i << "," << std::fixed << std::setprecision(6) << dis(gen) << "," << res << "\n";
    }
    std::cout << "Клиент [" << name << "] завершил обработку " << N << " задач.\n";
}

int main() {
    Server<double> srv;
    srv.start();

    const int N = 500; // 5 < N < 10000
    std::thread c1(client_thread, "sin", std::ref(srv), N, std::sin, 0.0, 10.0);
    std::thread c2(client_thread, "sqrt", std::ref(srv), N, std::sqrt, 0.0, 100.0);
    std::thread c3(client_thread, "pow", std::ref(srv), N, [](double x) { return std::pow(x, 2.5); }, 1.0, 10.0);

    c1.join(); c2.join(); c3.join();
    srv.stop();
    std::cout << "Все задачи выполнены. Результаты сохранены в *_results.csv\n";
    return 0;
}