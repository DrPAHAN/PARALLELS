#define _USE_MATH_DEFINES
#include <iostream>
#include <cmath>

const int N = 10000000;
const double PI = M_PI;

//прерка на тип foat/double при компиляции
//OFF - doube, ON - float
#ifdef USE_FLOAT
    typedef float real;
#else
    typedef double real;
#endif

int main() {
    real* array = new real[N]; 
    real sum = 0.0;

    for (int i = 0; i < N; i++) {
        array[i] = sin(2 * PI * i / N);
    }

    for (int i = 0; i < N; i++) {
        sum += array[i];
    }

    std::cout << "Sum: " << sum << std::endl;

    delete[] array;
    return 0;
}