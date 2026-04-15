#include "metrics.h"
#include <iostream>
#include <iomanip>

void print_metrics(const std::string& algo_name, const Metrics& m) {
    std::cout << "\n=== Metrics: " << algo_name << " ===\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Runtime:       " << m.runtime_seconds << " seconds\n";
    std::cout << "  Iterations:    " << m.iterations << "\n";
    std::cout << "  Allreduce calls (logical messages): " << m.messages_sent << "\n";
    std::cout << "  Approx. bytes sent:                 " << m.bytes_sent << "\n";
    std::cout << "========================\n\n";
}