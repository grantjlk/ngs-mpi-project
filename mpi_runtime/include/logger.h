#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <string>

// Simple logger with levels and timestamps
// Usage: LOG_INFO("message"), LOG_WARN("message"), LOG_ERROR("message")

inline std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

#define LOG_INFO(msg)  std::cout << "[" << current_timestamp() << "][INFO]  " << msg << "\n"
#define LOG_WARN(msg)  std::cout << "[" << current_timestamp() << "][WARN]  " << msg << "\n"
#define LOG_ERROR(msg) std::cerr << "[" << current_timestamp() << "][ERROR] " << msg << "\n"