cmake_minimum_required(VERSION 3.10)

add_definitions(-D_USE_MATH_DEFINES)

project(SinusSum)

option(USE_FLOAT "Use float instead of double" OFF)

if(USE_FLOAT)
    add_definitions(-DUSE_FLOAT)
endif()

add_executable(sinus_sum main.cpp)