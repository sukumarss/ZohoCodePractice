#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <fstream>
#include <sstream>
#include <memory>
#include <ctime>



class ThreadPool {
public:
	ThreadPool(size_t);
	~ThreadPool();

	template<class F, class... Args>
	void enqueue(F&& f,Args&&... args);

private:
	std::mutex queueMutex;
	std::queue<std::function<void()>> tasks;
	std::vector<std::thread> workers;
	std::condition_variable condition;
	std::atomic<bool> stop;

	void worker();
};

ThreadPool::ThreadPool(size_t numThreads) {
	for (size_t i = 0; i < numThreads; i++) {
		workers.emplace_back([this] {this->worker(); });
	}
}

ThreadPool::~ThreadPool() {
	stop.store(true);
	condition.notify_all();
	for (std::thread& worker : workers) {
		if (worker.joinable()) worker.join();
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

template<class F,class... Args>
void ThreadPool::enqueue(F&& f, Args&&... args) {
	auto task = std::make_shared<std::function<void()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

	{
		std::lock_guard<std::mutex> lock(queueMutex);
		tasks.emplace([task]() {(*task)(); });

	}
	condition.notify_one();
}


class Logger {
public:
	enum class LogLevel { INFO, WARN, ERR};

	static Logger& getInstance() {
		static Logger logger;
		return logger;
	}

	void log(LogLevel level, const std::string logMessage) {
		std::ostringstream logEntry;
		logEntry << "[" << getTimestamp() << "]" << logLevelToString(level) << logMessage << "\n";

		{
			std::lock_guard<std::mutex> lock(logMutex);
			logQueue.push(logEntry.str());

		}
		condition.notify_one();

	}


private:
	std::mutex logMutex;
	std::queue<std::string> logQueue;
	std::condition_variable condition;
	std::atomic<bool> isRunning;
	std::thread logger;
	std::ofstream logFile_;

	Logger() : isRunning(true), logFile_("log.txt", std::ios::app) {
		logger = std::thread(&Logger::processLog, this);
	}

	~Logger() {
		isRunning = false;
		stop();
	}


	void processLog() {
		while (true) {
			std::unique_lock<std::mutex> lock(logMutex);
			condition.wait(lock, [this]() { return !isRunning || !logQueue.empty(); });

			while (!logQueue.empty()) {
				if (logFile_.is_open()) {
					logFile_ << logQueue.front();
					logQueue.pop();
				}
				else {
					std::cerr << "Log file is not open!" << std::endl;
					return;
				}
			}
			logFile_.flush();

			if (!isRunning && logQueue.empty()) return;
		}
	}
	std::string getTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		std::tm tmStruct{};

		#ifdef _WIN32
			localtime_s(&tmStruct, &timeT); // Windows safe version
		#else
			localtime_r(&timeT, &tmStruct); // Linux/macOS thread-safe version
		#endif

		char buffer[20];
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmStruct);
		return std::string(buffer);
	}


	std::string logLevelToString(LogLevel level) {
		switch (level) {
		case LogLevel::INFO: return "INFO";
		case LogLevel::WARN: return "WARN";
		case LogLevel::ERR: return "ERROR";
		default: return "UNKNOWN";
		}
	}

	void stop() {
		{
			std::lock_guard<std::mutex> lock(logMutex);
			isRunning = false;
		}
		condition.notify_all();
		if (logger.joinable()) {
			logger.join();
		}
	}

};

void logging(std::thread::id id, Logger::LogLevel level, const std::string& log) {
	Logger::getInstance().log(level, " Thread " + std::to_string(reinterpret_cast<std::uintptr_t>(&id)) + " " + log);
	std::this_thread::sleep_for(std::chrono::microseconds(100));
}


void add(int x, int y) {
	std::cout << "Adding operation: "<<x+y << std::endl;
	logging(std::this_thread::get_id(), Logger::LogLevel::INFO, "Adding");

}

void sub(int x, int y) {
	std::cout << "Subtracting operation" << x + y << std::endl;
	logging(std::this_thread::get_id(), Logger::LogLevel::INFO, "Adding");
}

void multi(int x, int y) {
	std::cout << "Multiplication operation" << x + y << std::endl;

}


int main() {
	ThreadPool pool(3);

	for (int i = 0; i < 30; i++) {
		if (i % 3 == 0) {
			pool.enqueue(multi,1,2);
		}
		else if (i % 2 == 0) {
			pool.enqueue(sub,1,2);
		}
		else {
			pool.enqueue(add,1,2);
		}
	}

	return 0;
}

