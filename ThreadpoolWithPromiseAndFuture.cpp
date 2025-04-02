#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>
#include <future>

class ThreadPool {
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

private:
    std::mutex queueMutex;
    std::condition_variable condition;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::atomic<bool> stop;

    void worker();
};

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; i++) {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    for (std::thread& work : workers) {
        if (work.joinable()) {
            work.join();
        }
    }
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...) 
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

int main() {
    ThreadPool pool(4);
    
    std::vector<std::future<int>> results;
    
    for (int i = 0; i < 10; i++) {
        results.emplace_back(pool.enqueue([i] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "Executing task " << i << " by thread " << std::this_thread::get_id() << "\n";
            return i * i; // Return the square of the number
        }));
    }
    
    for (auto& res : results) {
        std::cout << "Result: " << res.get() << "\n"; // Retrieve and print the result
    }
    
    return 0;
}
