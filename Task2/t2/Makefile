CXX = g++                    # или clang++
CXXFLAGS = -O3 -fopenmp -std=c++17 -Wall -march=native

all: 2

2: 2.cpp
	$(CXX) $(CXXFLAGS) 2.cpp -o 2

clean:
	rm -f 2

.PHONY: all clean