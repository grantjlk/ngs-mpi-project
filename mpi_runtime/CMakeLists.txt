cmake_minimum_required(VERSION 3.16)
project(ngs_mpi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(MPI REQUIRED)

include_directories(include ${MPI_CXX_INCLUDE_PATH})

add_executable(ngs_mpi
    src/main.cpp
    src/graph.cpp
    src/partition.cpp
    src/leader_election.cpp
    src/dijkstra.cpp
    src/metrics.cpp
)

target_link_libraries(ngs_mpi ${MPI_CXX_LIBRARIES})
