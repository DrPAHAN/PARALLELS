cmake_minimum_required(VERSION 3.16)
project(task6_openacc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS program_options)
add_executable(task6 task6.cpp)
target_link_libraries(task6 PRIVATE Boost::program_options)

target_compile_options(task6 PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:NVHPC>,$<CXX_COMPILER_ID:PGI>>:-acc;-Minfo=all>
    $<$<CXX_COMPILER_ID:GNU>:-O3;-Wall;-Wextra>
)
target_link_options(task6 PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:NVHPC>,$<CXX_COMPILER_ID:PGI>>:-acc>
)

set(OPENACC_TARGET "" CACHE STRING "OpenACC target: host, multicore, gpu")
set_property(CACHE OPENACC_TARGET PROPERTY STRINGS "" "host" "multicore" "gpu")
if((CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC" OR CMAKE_CXX_COMPILER_ID STREQUAL "PGI") AND NOT OPENACC_TARGET STREQUAL "")
    target_compile_options(task6 PRIVATE -acc=${OPENACC_TARGET})
    target_link_options(task6 PRIVATE -acc=${OPENACC_TARGET})
endif()