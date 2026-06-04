#include <boost/program_options.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = boost::program_options;
using Matrix = std::vector<double>;

struct Args {
    int n = 128, max_iter = 1000000;
    double eps = 1e-6;
    std::string output = "result_matrix.txt";
    bool print = false;
};

inline int pos(int i, int j, int n) {
    return i * n + j;
}

Args parse(int argc, char** argv) {
    Args a;
    po::options_description d("Options");
    d.add_options()
        ("help,h", "show help")
        ("size,n", po::value<int>(&a.n)->default_value(a.n), "grid size N for NxN matrix")
        ("eps,e", po::value<double>(&a.eps)->default_value(a.eps), "accuracy")
        ("max-iter,i", po::value<int>(&a.max_iter)->default_value(a.max_iter), "iteration limit")
        ("output,o", po::value<std::string>(&a.output)->default_value(a.output), "matrix output file")
        ("print,p", po::bool_switch(&a.print), "print matrix to terminal");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, d), vm);
    po::notify(vm);
    if (vm.count("help")) {
        std::cout << d << '\n';
        std::exit(0);
    }
    if (a.n < 2 || a.eps <= 0 || a.max_iter < 1) {
        throw std::runtime_error("bad arguments");
    }
    return a;
}

void set_boundaries(Matrix& u, int n) {
    std::fill(u.begin(), u.end(), 0.0);
    const double tl = 10, tr = 20, br = 30, bl = 20;
    for (int k = 0; k < n; ++k) {
        double t = static_cast<double>(k) / (n - 1);
        u[pos(0, k, n)] = tl + (tr - tl) * t;
        u[pos(n - 1, k, n)] = bl + (br - bl) * t;
        u[pos(k, 0, n)] = tl + (bl - tl) * t;
        u[pos(k, n - 1, n)] = tr + (br - tr) * t;
    }
}

std::pair<int, double> solve(Matrix& current, Matrix& next, int n, double eps, int max_iter) {
    int total = n * n;
    double err = 0.0;
    int iter = 0;

#pragma acc data create(next[0:total]) copy(current[0:total])
    {
        for (iter = 1; iter <= max_iter; ++iter) {
            err = 0.0;

#pragma acc parallel loop collapse(2) reduction(max:err) independent
            for (int i = 1; i < n - 1; ++i) {
                for (int j = 1; j < n - 1; ++j) {
                    int k = pos(i, j, n);
                    double new_val = 0.25 * (
                        current[pos(i - 1, j, n)] +
                        current[pos(i + 1, j, n)] +
                        current[pos(i, j - 1, n)] +
                        current[pos(i, j + 1, n)]
                    );
                    err = std::max(err, std::abs(new_val - current[k]));
                    next[k] = new_val;
                }
            }

            std::swap(current, next);

            if (err <= eps) break;
        }
    }

    return {std::min(iter, max_iter), err};
}

void save(const Matrix& u, int n, const std::string& file) {
    std::ofstream out(file);
    out << std::setprecision(12);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out << std::setw(16) << u[pos(i, j, n)] << (j + 1 == n ? '\n' : ' ');
        }
    }
}

void print(const Matrix& u, int n) {
    std::cout << std::fixed << std::setprecision(6);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            std::cout << std::setw(11) << u[pos(i, j, n)];
        }
        std::cout << '\n';
    }
}

int main(int argc, char** argv) {
    try {
        Args args = parse(argc, argv);
        Matrix current(args.n * args.n);
        Matrix next(args.n * args.n);

        set_boundaries(current, args.n);
        set_boundaries(next, args.n);

        auto start = std::chrono::high_resolution_clock::now();
        auto [iters, err] = solve(current, next, args.n, args.eps, args.max_iter);
        double sec = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();

        save(current, args.n, args.output);
        std::cout << "size=" << args.n << "x" << args.n << "\n"
                  << "iterations=" << iters << "\n"
                  << "error=" << std::scientific << err << "\n"
                  << "time_sec=" << sec << "\n"
                  << "output=" << args.output << '\n';

        if (args.print) {
            print(current, args.n);
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
