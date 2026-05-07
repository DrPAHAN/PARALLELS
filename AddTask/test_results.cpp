#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>
#include <iomanip>

bool verify_file(const std::string& filename, std::function<double(double)> expected_func) {
    std::ifstream in(filename);
    if (!in) { std::cerr << "Файл " << filename << " не найден.\n"; return false; }

    std::string line;
    std::getline(in, line); // skip header
    bool ok = true;
    int tests = 0, failed = 0;
    const double EPS = 1e-9;

    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string id_s, input_s, res_s;
        if (!std::getline(ss, id_s, ',') || !std::getline(ss, input_s, ',') || !std::getline(ss, res_s, ',')) continue;

        double input = std::stod(input_s);
        double res = std::stod(res_s);
        double exp = expected_func(input);

        if (std::abs(res - exp) > EPS) {
            failed++;
            std::cout << "  Ошибка в " << filename << " id=" << id_s << ": expected=" << exp << ", got=" << res << "\n";
        }
        tests++;
    }
    std::cout << filename << ": " << tests << " тестов, " << (tests - failed) << " пройдено, " << failed << " ошибок.\n";
    return failed == 0;
}

int main() {
    bool all_pass = true;
    all_pass &= verify_file("sin_results.csv", std::sin);
    all_pass &= verify_file("sqrt_results.csv", std::sqrt);
    all_pass &= verify_file("pow_results.csv", [](double x){ return std::pow(x, 2.5); });

    std::cout << (all_pass ? "\n✅ Все тесты пройдены успешно!" : "\n❌ Обнаружены ошибки в результатах.") << "\n";
    return all_pass ? 0 : 1;
}