/**
 * @file matrix.h
 * @brief High-performance compile-time Matrix Library for ML pipelines.
 * Compatible with C++17 standards and above.
 */
#pragma once
#include "../../MachineLearning/include/std17pch.h"

// Higher-order constexpr derivative calculator
// Computes f'(x) using central difference method: [f(x + h) - f(x - h)] / (2 * h)
template <typename Func>
[[nodiscard]] constexpr double derivative(Func f, double x, double h = 1e-5) {
    return (f(x + h) - f(x - h)) / (2.0 * h);
}

// - Instead of a straight \(d\),
// calculus uses a curly \(\partial \), read as "del" or "partial d".
// Compile-time Gradient calculator for a function with N input variables
// Uses Central Finite Difference method element-wise
template <size_t N, typename Func>
[[nodiscard]] constexpr std::array<double, N> compute_gradient(Func f, const std::array<double, N>& x,
                                                               double h = 1e-5) {
    std::array<double, N> gradient{};

    for (size_t i = 0; i < N; ++i) {
        // Create mutated points to calculate partial derivative along dimension i
        std::array<double, N> x_plus  = x;
        std::array<double, N> x_minus = x;

        // Apply a tiny change ONLY to the i-th variable (Holding others constant)
        x_plus[i] += h;
        x_minus[i] -= h;

        // Central difference formula: [f(x_plus) - f(x_minus)] / (2 * h)
        gradient[i] = (f(x_plus) - f(x_minus)) / (2.0 * h);
    }

    return gradient;
}