# Makefile для macOS (Intel / Homebrew в /usr/local)

CXX = clang++

CXXFLAGS = -O3 \
           -Xclang -fopenmp \
           -std=c++17 \
           -Wall \
           -I/usr/local/opt/libomp/include \
           -L/usr/local/opt/libomp/lib \
           -lomp

all: 3_v1 3_v2

3_v1: 3_v1.cpp
	$(CXX) $(CXXFLAGS) 3_v1.cpp -o 3_v1

3_v2: 3_v2.cpp
	$(CXX) $(CXXFLAGS) 3_v2.cpp -o 3_v2

clean:
	rm -f 3_v1 3_v2

.PHONY: all clean