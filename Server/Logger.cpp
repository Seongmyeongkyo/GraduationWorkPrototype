#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace {
    std::ofstream g_log_file;
    std::mutex g_log_mutex;

    std::string Timestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
        localtime_s(&tm_buf, &t);
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
}

namespace Logger {
    void Init(unsigned short port) {
        std::error_code ec;
        std::filesystem::create_directories("Logs", ec);
        g_log_file.open("Logs/server_" + std::to_string(port) + ".txt", std::ios::app);
        Log("==== Server starting on port " + std::to_string(port) + " ====");
    }

    void Log(const std::string& message) {
        const std::string line = "[" + Timestamp() + "] " + message;
        std::lock_guard<std::mutex> lock(g_log_mutex); // multiple IOCP worker threads call this concurrently
        std::cout << line << std::endl;
        if (g_log_file.is_open()) {
            g_log_file << line << std::endl;
            g_log_file.flush(); // small/low-traffic dev server - favor durability over throughput
        }
    }
}
