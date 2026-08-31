/**
 * @file prob_and_statis.h
 * @brief High-performance compile-time prob_and_statis Library for ML pipelines.
 * Compatible with C++17 standards and above.
 */
#pragma once
#include "../Utilities/compile_time_math.h"
#include "../Matrix/matrix.h"

// Struct to hold calculated descriptive statistics results
struct DataStats {
    double mean;
    double variance;
    double standard_deviation;
};

// Compile-time statistic calculator optimized for flat datasets
template <size_t N>
[[nodiscard]] constexpr DataStats compute_statistics(const std::array<double, N>& dataset) {
    static_assert(N > 1, "Dataset must contain at least 2 elements to calculate variance!");

    // 1. Compute the Mean (Average)
    double sum = 0.0;
    for (double val : dataset) {
        sum += val;
    }
    double mean = sum / static_cast<double>(N);

    // 2. Compute the Variance (Average of squared differences from the Mean)
    double squared_diff_sum = 0.0;
    for (double val : dataset) {
        double diff = val - mean;
        squared_diff_sum += diff * diff;
    }
    // Using N - 1 for sample variance (Bessel's correction) to ensure unbiased estimation
    double variance = squared_diff_sum / static_cast<double>(N - 1);

    // 3. Compute Standard Deviation (Square root of variance)
    // Note: std::sqrt is fully constexpr supported in C++26; for C++17/20 compilers,
    // it will resolve optimally or can be evaluated via manual constexpr sqrt if forced.
    double std_dev = CompileTimeMath::sqrt(variance);

    return {mean, variance, std_dev};
}

// Main function to perform Column-wise Z-score Normalization
template <size_t Rows, size_t Cols>
[[nodiscard]] constexpr Matrix<Rows, Cols> z_score_normalize(const Matrix<Rows, Cols>& X) {
    static_assert(Rows > 1, "Must have more than 1 sample row to compute column statistics!");

    Matrix<Rows, Cols>       X_scaled{};
    std::array<double, Cols> means{};
    std::array<double, Cols> std_devs{};

    // Step 1: Compute Mean for each individual column
    for (size_t c = 0; c < Cols; ++c) {
        double col_sum = 0.0;
        for (size_t r = 0; r < Rows; ++r) {
            col_sum += X(r, c);
        }
        means[c] = col_sum / static_cast<double>(Rows);
    }

    // Step 2: Compute Standard Deviation for each individual column
    for (size_t c = 0; c < Cols; ++c) {
        double squared_diff_sum = 0.0;
        for (size_t r = 0; r < Rows; ++r) {
            double diff = X(r, c) - means[c];
            squared_diff_sum += diff * diff;
        }
        // Using sample variance (Rows - 1)
        double variance = squared_diff_sum / static_cast<double>(Rows - 1);
        std_devs[c]     = CompileTimeMath::sqrt(variance);

        // Safety checkpoint: if std_dev is near zero (constant feature), set it to 1 to avoid division by zero
        if (std_devs[c] < 1e-12) {
            std_devs[c] = 1.0;
        }
    }

    // Step 3: Apply the Z-score equation element-wise: (X - Mean) / StdDev
    for (size_t r = 0; r < Rows; ++r) {
        for (size_t c = 0; c < Cols; ++c) {
            X_scaled(r, c) = (X(r, c) - means[c]) / std_devs[c];
        }
    }

    return X_scaled;
}