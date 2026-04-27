CXX = clang++
CXXFLAGS = -Xclang -fopenmp \
           -I$(shell brew --prefix libomp)/include \
           -L$(shell brew --prefix libomp)/lib \
           -lomp \
           -std=c++17 \
           -Wall

all: 1

1: 1.cpp
	$(CXX) $(CXXFLAGS) 1.cpp -o 1

clean:
	rm -f 1