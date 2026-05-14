cmake_minimum_required(VERSION 3.16)
project(ParallelLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Флаги для разных компиляторов
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    add_compile_options(-Wall -Wextra -pthread)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -pthread)
endif()

add_executable(task1 task1_matrix_vector.cpp)
add_executable(task2 task2_client_server.cpp)

# Линковка потоков
find_package(Threads REQUIRED)
target_link_libraries(task1 Threads::Threads)
target_link_libraries(task2 Threads::Threads)