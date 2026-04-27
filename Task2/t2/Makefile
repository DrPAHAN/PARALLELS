CXX = g++                    # или clang++
CXXFLAGS = -O3 -fopenmp -std=c++17 -Wall -march=native

all: integrate

integrate: integrate.cpp
	$(CXX) $(CXXFLAGS) integrate.cpp -o integrate

clean:
	rm -f integrate

.PHONY: all clean