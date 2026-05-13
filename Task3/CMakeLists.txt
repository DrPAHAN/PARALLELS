cmake_minimum_required(VERSION 3.10)
project(parallel_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Создаём исполняемые файлы для каждого .cpp
file(GLOB SOURCES "*.cpp")
foreach(SRC ${SOURCES})
    get_filename_component(EXE ${SRC} NAME_WE)
    add_executable(${EXE} ${SRC})
    target_link_libraries(${EXE} PRIVATE Threads::Threads)
    target_compile_options(${EXE} PRIVATE -O2 -pthread)
endforeach()