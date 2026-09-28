#include "Logger.h"
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace {
    struct LogEntry {
        // 호출 시점에 찍어 두므로, 파일에 늦게 쓰여도 실제 발생 시각이 남는다.
        std::chrono::system_clock::time_point time;
        std::string message;
    };

    std::ofstream g_log_file;
    std::mutex g_queue_mutex;
    std::condition_variable g_queue_cv;
    std::vector<LogEntry> g_queue;
    bool g_stop = false;
    std::thread g_log_thread;
    std::once_flag g_shutdown_once;

    std::string FormatLine(const LogEntry& entry) {
        const std::time_t t = std::chrono::system_clock::to_time_t(entry.time);
        std::tm tm_buf{};
        localtime_s(&tm_buf, &t);
        std::ostringstream oss;
        oss << '[' << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "] " << entry.message;
        return oss.str();
    }

    void LogThreadMain() {
        std::vector<LogEntry> batch;
        for (;;) {
            bool stopping;
            {
                std::unique_lock<std::mutex> lock(g_queue_mutex);
                g_queue_cv.wait(lock, [] { return g_stop || !g_queue.empty(); });
                batch.swap(g_queue);
                stopping = g_stop;
            }
            for (const LogEntry& entry : batch) {
                const std::string line = FormatLine(entry);
                std::cout << line << '\n';
                if (g_log_file.is_open()) g_log_file << line << '\n';
            }
            std::cout.flush();
            if (g_log_file.is_open()) g_log_file.flush(); // 줄마다가 아니라 묶음마다 한 번
            batch.clear();
            // 종료 요청 후 가져간 묶음까지만 쓰고 끝낸다 (로그가 계속 들어와도 Shutdown이 무한 대기하지 않도록).
            if (stopping) return;
        }
    }
}

namespace Logger {
    void Init(unsigned short port) {
        std::error_code ec;
        std::filesystem::create_directories("Logs", ec);
        g_log_file.open("Logs/server_" + std::to_string(port) + ".txt", std::ios::app);
        g_log_thread = std::thread(LogThreadMain);
        Log("==== Server starting on port " + std::to_string(port) + " ====");
    }

    void Log(const std::string& message) {
        LogEntry entry{ std::chrono::system_clock::now(), message };
        {
            std::lock_guard<std::mutex> lock(g_queue_mutex);
            g_queue.push_back(std::move(entry));
        }
        g_queue_cv.notify_one();
    }

    void Shutdown() {
        std::call_once(g_shutdown_once, [] {
            {
                std::lock_guard<std::mutex> lock(g_queue_mutex);
                g_stop = true;
            }
            g_queue_cv.notify_one();
            if (g_log_thread.joinable()) g_log_thread.join();
        });
    }
}
