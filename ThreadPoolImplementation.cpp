#include <iostream>
#include <thread>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>

class ThreadPool {
public:
	ThreadPool(size_t Threads);
	~ThreadPool();

	template<class F, class... Args>
	void enqueue(F&& f, Args&&... args);

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
			condition.wait(lock, [this] {return stop || !tasks.empty(); });

			if (stop && tasks.empty()) return;

			task = std::move(tasks.front());
			tasks.pop();
		}
		task();
	}
}

template<class F, class... Args>
void ThreadPool::enqueue(F&& f, Args&&... args) {
	auto task = std::make_shared<std::function<void()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	{
		std::lock_guard<std::mutex> lock(queueMutex);
		tasks.emplace([task]() { (*task)(); });
	}
	condition.notify_one();
}


int main() {
	ThreadPool pool(4);

	for (int i = 0; i < 10; i++) {
		pool.enqueue([i] {
			std::cout << "Executing task " << i << " by thread " << std::this_thread::get_id() << "\n";
			});
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
	return 0;
}