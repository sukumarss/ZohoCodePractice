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
		workers.emplace_back(worker, this);
	}
}

ThreadPool::~ThreadPool() {
	stop = true;
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
	auto task = std::make_shared(std::function<void()>)(
		std::bind(std::forward<F>(f), forward<Args>...(args));
		);

	{
		std::lock_guard(std::mutex) lock(queueMutex);
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
		stop();
	}

	void processLog() {
		while (isRunning || logQueue.empty()) {
			std::unique_lock<std::mutex> lock(logMutex);
			condition.wait(lock, [this]() {return isRunning || !logQueue.empty(); });

			while (!logQueue.empty()) {
				logFile_ << logQueue.front();
				logQueue.pop();
			}
			logFile_.flush();

		}
	}
	std::string getTimestamp() {
		auto now = std::chrono::system_clock::now();
		auto timeT = std::chrono::system_clock::to_time_t(now);
		std::tm tmStruct{};

		// Handle platform differences
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
		condition.notify_all();
		if (logger.joinable()) {
			logger.join();
		}
	}

};

void logging(Logger::LogLevel level,std::string log) {
	
}

