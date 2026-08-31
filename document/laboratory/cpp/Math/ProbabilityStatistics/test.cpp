// g++ test.cpp -std=c++17 && ./a.out && rm a.out
#include "prob_and_statis.h"

int basic_estimate() {
    // A mock dataset representing house sizes (scaled or normalized) or errors
    constexpr std::array<double, 5> sample_data{1.2, 2.5, 3.8, 4.1, 5.4};

    // The statistics are calculated entirely by the compiler! Zero execution cost.
    constexpr DataStats stats = compute_statistics<5>(sample_data);

    std::cout << "--- Compile-Time Dataset Analysis ---\n";
    std::cout << "Calculated Mean (Average)       : " << stats.mean << "\n";
    std::cout << "Sample Variance                 : " << stats.variance << "\n";
    std::cout << "Standard Deviation (Sigma)      : " << stats.standard_deviation << "\n";

    return 0;
}

int main() {
    // Input Matrix X (4 houses, 2 features):
    // Column 0: Real estate size (ranges from 50 to 200 m2) -> High range numbers
    // Column 1: Number of bedrooms (ranges from 1 to 4 beds) -> Low range numbers
    constexpr Matrix<4, 2> X{50.0, 1.0, 100.0, 2.0, 150.0, 3.0, 200.0, 4.0};

    std::cout << "--- Original Matrix X ---\n";
    X.print();

    // The normalization loops are completely evaluated at compile time by the compiler!
    constexpr Matrix<4, 2> X_normalized = z_score_normalize(X);

    std::cout << "\n--- Z-score Normalized Matrix X ---\n";
    X_normalized.print();

    return 0;
}