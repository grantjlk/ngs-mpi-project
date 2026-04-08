#pragma once
#include <string>

struct Metrics {
    long messages_sent;
    long bytes_sent;
    int iterations;
    double runtime_seconds;
};

void print_metrics(const std::string& algo_name, const Metrics& m);
