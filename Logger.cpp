#include <iostream>
#include <fstream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <sstream>
#include <chrono>

class Logger {
public:
    enum class LogLevel { INFO, WARN, ERROR };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void log(LogLevel level, const std::string& message) {
        std::ostringstream logEntry;
        logEntry << "[" << getTimestamp() << "] " << logLevelToString(level) << ": " << message << "\n";

        {
            std::lock_guard<std::mutex> lock(mutex_);
            logQueue_.push(logEntry.str());
        }
        condVar_.notify_one();  // Wake up logging thread
    }

    void stop() {
        isRunning_ = false;
        condVar_.notify_one();
        if (logThread_.joinable()) {
            logThread_.join();
        }
    }

private:
    std::queue<std::string> logQueue_;
    std::mutex mutex_;
    std::condition_variable condVar_;
    std::atomic<bool> isRunning_;
    std::thread logThread_;
    std::ofstream logFile_;

    Logger() : isRunning_(true), logFile_("log.txt", std::ios::app) {
        logThread_ = std::thread(&Logger::processLogs, this);
    }

    ~Logger() {
        stop();
    }

    void processLogs() {
        while (isRunning_ || !logQueue_.empty()) {
            std::unique_lock<std::mutex> lock(mutex_);
            condVar_.wait(lock, [this]() { return !logQueue_.empty() || !isRunning_; });

            while (!logQueue_.empty()) {
                logFile_ << logQueue_.front();
                logQueue_.pop();
            }
            logFile_.flush();
        }
    }

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::tm tmStruct;
        localtime_r(&timeT, &tmStruct); // Thread-safe version of localtime

        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmStruct);
        return std::string(buffer);
    }

    std::string logLevelToString(LogLevel level) {
        switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

void workerThread(int id) {
    for (int i = 0; i < 5; ++i) {
        Logger::getInstance().log(Logger::LogLevel::INFO, "Thread " + std::to_string(id) + " logging message " + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    const int NUM_THREADS = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(workerThread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    Logger::getInstance().stop();
    return 0;
}
