#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <iomanip>

using namespace std;

const double EPS = 1e-9;

bool approx_equal(double a, double b) {
    return fabs(a - b) < EPS;
}

bool test_file_sin(const string& filename) {
    ifstream in(filename);
    if (!in) { cerr << "Cannot open " << filename << "\n"; return false; }
    
    string line;
    int passed = 0, total = 0;
    
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        double arg, expected;
        if (sscanf(line.c_str(), "%lf\t%lf", &arg, &expected) != 2) continue;
        
        double computed = sin(arg);
        total++;
        if (approx_equal(expected, computed)) passed++;
        else cerr << "FAIL: sin(" << arg << ") = " << expected 
                  << ", expected " << computed << "\n";
    }
    
    cout << filename << ": " << passed << "/" << total << " passed\n";
    return passed == total;
}

bool test_file_sqrt(const string& filename) {
    ifstream in(filename);
    if (!in) { cerr << "Cannot open " << filename << "\n"; return false; }
    
    string line;
    int passed = 0, total = 0;
    
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        double arg, expected;
        if (sscanf(line.c_str(), "%lf\t%lf", &arg, &expected) != 2) continue;
        
        double computed = sqrt(arg);
        total++;
        if (approx_equal(expected, computed)) passed++;
        else cerr << "FAIL: sqrt(" << arg << ") = " << expected 
                  << ", expected " << computed << "\n";
    }
    
    cout << filename << ": " << passed << "/" << total << " passed\n";
    return passed == total;
}

bool test_file_pow(const string& filename) {
    ifstream in(filename);
    if (!in) { cerr << "Cannot open " << filename << "\n"; return false; }
    
    string line;
    int passed = 0, total = 0;
    
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        double base, expected;
        int exp;
        if (sscanf(line.c_str(), "%lf\t%d\t%lf", &base, &exp, &expected) != 3) continue;
        
        double computed = pow(base, exp);
        total++;
        if (approx_equal(expected, computed)) passed++;
        else cerr << "FAIL: pow(" << base << "," << exp << ") = " << expected 
                  << ", expected " << computed << "\n";
    }
    
    cout << filename << ": " << passed << "/" << total << " passed\n";
    return passed == total;
}

int main() {
    cout << fixed << setprecision(10);
    cout << "Running tests...\n\n";
    
    bool ok = true;
    ok &= test_file_sin("results_sin.txt");
    ok &= test_file_sqrt("results_sqrt.txt");
    ok &= test_file_pow("results_pow.txt");
    
    cout << "\n" << (ok ? "ALL TESTS PASSED ✓" : "SOME TESTS FAILED ✗") << "\n";
    return ok ? 0 : 1;
}