#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <iomanip>
using namespace std;

bool approx(double a, double b) { return fabs(a-b) < 1e-8; }

int test_file(const string& fname, double (*ref)(double)) {
    ifstream in(fname); if(!in) return -1;
    string line; int ok=0, total=0;
    while(getline(in,line)) {
        if(line.empty()||line[0]=='#') continue;
        double arg,res; if(sscanf(line.c_str(),"%lf\t%lf",&arg,&res)!=2) continue;
        total++; if(approx(res, ref(arg))) ok++;
    }
    cout << fname << ": " << ok << "/" << total << " OK\n";
    return ok==total ? 0 : 1;
}

int test_file_pow(const string& fname) {
    ifstream in(fname); if(!in) return -1;
    string line; int ok=0, total=0;
    while(getline(in,line)) {
        if(line.empty()||line[0]=='#') continue;
        double base,res; int exp;
        if(sscanf(line.c_str(),"%lf\t%d\t%lf",&base,&exp,&res)!=3) continue;
        total++; if(approx(res, pow(base,static_cast<double>(exp)))) ok++;
    }
    cout << fname << ": " << ok << "/" << total << " OK\n";
    return ok==total ? 0 : 1;
}

int main() {
    cout << fixed << setprecision(10);
    int r=0;
    r |= test_file("results_sin.txt", sin);
    r |= test_file("results_sqrt.txt", sqrt);
    r |= test_file_pow("results_pow.txt");
    cout << (r==0 ? "\nALL TESTS PASSED\n" : "\nSOME TESTS FAILED\n");
    return r;
}