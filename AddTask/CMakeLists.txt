cmake_minimum_required(VERSION 3.15)
project(ParallelLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_executable(task1_mat_vec task1_mat_vec.cpp)
target_link_libraries(task1_mat_vec PRIVATE Threads::Threads)

add_executable(task2_client_server task2_client_server.cpp)
target_link_libraries(task2_client_server PRIVATE Threads::Threads)

add_executable(test_results test_results.cpp)