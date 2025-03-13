#include<iostream>
#include<thread>
#include<fstream>
#include<mutex>
#include<queue>
#include<condition_variable>
#include<atomic>
using namespace std;
class Logger {
private:
    ofstream logFile;
    queue<string> logQueue;
    mutex queueMutex;
    condition_variable condition;
    thread workerThread;
    atomic<bool> stopLogging;

    void processQueue() {
        while (true) {
            unique_lock<mutex> lock(queueMutex);
            condition.wait(lock, [this] { return !logQueue.empty() || stopLogging; });

            while (!logQueue.empty()) {
                logFile << logQueue.front() << endl;
                logQueue.pop();
            }

            logFile.flush();
            if (stopLogging && logQueue.empty()) break;
        }
    }

    void shutdown() {
        {
            lock_guard<mutex> lock(queueMutex);
            stopLogging = true;
        }
        
        condition.notify_one();
        workerThread.join();
        logFile.close();
    }

public:
    Logger(const string& filename) : stopLogging(false) {
        logFile.open(filename, ios::out | ios::app);
        if (!logFile.is_open()) {
            throw runtime_error("Failed to load the logFile");
        }
        workerThread = thread(&Logger::processQueue, this);
    }

    ~Logger() {
        shutdown();
    }
    void log(const string& message) {
        {
            lock_guard<mutex> lock(queueMutex);
            logQueue.push(message);
        }
        condition.notify_one();

    }

    
};

int main() {
    Logger logger("logs.txt");

    thread t1([&]() {
        for (int i = 0; i < 1000; i++) {
            logger.log("thread 1 - message");
        }
        });

    thread t2([&]() {
        for (int i = 0; i < 1000; i++) {
            logger.log("thread 2 - message");
        }
        });



    t1.join();
    t2.join();

    return 0;
}
