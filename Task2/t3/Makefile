CXX = g++                  
CXXFLAGS = -O3 -fopenmp -std=c++14 -Wall -march=native

all: 3_v1 3_v2

3_v1: 3_v1.cpp
	$(CXX) $(CXXFLAGS) 3_v1.cpp -o 3_v1

3_v2: 3_v2.cpp
	$(CXX) $(CXXFLAGS) 3_v2.cpp -o 3_v2

clean:
	rm -f 3_v1 3_v2

.PHONY: all clean