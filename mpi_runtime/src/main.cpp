#include <mpi.h>
#include <iostream>
#include <string>
#include "graph.h"
#include "partition.h"
#include "metrics.h"

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // parse CLI arguments
    std::string graph_path, part_path, algo;
    int source = 0;
    int rounds = 200;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--graph" && i + 1 < argc)       graph_path = argv[++i];
        else if (arg == "--part" && i + 1 < argc)   part_path  = argv[++i];
        else if (arg == "--algo" && i + 1 < argc)   algo       = argv[++i];
        else if (arg == "--source" && i + 1 < argc) source     = std::stoi(argv[++i]);
        else if (arg == "--rounds" && i + 1 < argc) rounds     = std::stoi(argv[++i]);
    }

    if (graph_path.empty() || part_path.empty() || algo.empty()) {
        if (rank == 0) {
            std::cerr << "Usage: ngs_mpi --graph <path> --part <path> "
                      << "--algo <leader|dijkstra> [--source 0] [--rounds 200]\n";
        }
        MPI_Finalize();
        return 1;
    }

    // load graph and partition on all ranks
    Graph g;
    Partition p;
    try {
        g = load_graph(graph_path);
        p = load_partition(part_path);
    } catch (const std::exception& ex) {
        std::cerr << "[rank " << rank << "] Error loading files: "
                  << ex.what() << "\n";
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        std::cout << "Running algorithm: " << algo
                  << " with " << size << " ranks\n";
    }

    // algorithms will be called here once written

    MPI_Finalize();
    return 0;
}