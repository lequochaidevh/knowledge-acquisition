/**
 * @file optimizer.h
 * @brief High-performance compile-time optimizer Library for ML pipelines.
 * Compatible with C++17 standards and above.
 */
#pragma once
#include "../Utilities/compile_time_math.h"
#include "../Matrix/matrix.h"

// Compile-time Binary Cross-Entropy Loss Calculator
template <size_t N>
[[nodiscard]] constexpr double compute_binary_cross_entropy(const std::array<double, N>& targets,
                                                            const std::array<double, N>& predictions) {
    double total_loss = 0.0;

    // A small epsilon value to prevent log(0) which causes undefined/NaN results
    constexpr double epsilon = 1e-15;

    for (size_t i = 0; i < N; ++i) {
        double y = targets[i];
        double p = predictions[i];

        // Clip predictions to stay strictly inside (epsilon, 1.0 - epsilon) boundary
        if (p < epsilon) p = epsilon;
        if (p > 1.0 - epsilon) p = 1.0 - epsilon;

        // Core Binary Cross-Entropy math step
        double log_likelihood = (y * CompileTimeMath::log(p)) + ((1.0 - y) * CompileTimeMath::log(1.0 - p));
        total_loss += log_likelihood;
    }

    return -total_loss / static_cast<double>(N);
}

// =========================

// Analytical Gradient calculation for a dummy non-convex "wavy" loss landscape
// Model loss: L = W^4 - 4*W^3 + 4*W (Contains multiple valleys / local minima)
// https://www.desmos.com
template <size_t Rows, size_t Cols>
[[nodiscard]] Matrix<Rows, Cols> compute_wavy_gradient(const Matrix<Rows, Cols>& W) {
    Matrix<Rows, Cols> grad{};
    for (size_t r = 0; r < Rows; ++r) {
        for (size_t c = 0; c < Cols; ++c) {
            double w = W(r, c);
            // Analytical derivative: dL/dW = 4*W^3 - 8*W + 2
            grad(r, c) = (4.0 * w * w * w) - (6.0 * w * w) + 4.0;
        }
    }
    return grad;
}

// Gradient Descent Optimizer with MOMENTUM Acceleration
template <size_t Rows, size_t Cols>
void train_with_momentum(Matrix<Rows, Cols>& W, double alpha = 0.01, double beta = 0.9, size_t epochs = 100) {
    // Initialize Velocity Matrix V with all zeros (The ball starts at rest)
    Matrix<Rows, Cols> V{};
    V.zero_fill();

    std::cout << "--- Starting Momentum Optimization Loop ---\n";
    for (size_t epoch = 1; epoch <= epochs; ++epoch) {
        // Compute current gradient landscape
        Matrix<Rows, Cols> grad = compute_wavy_gradient(W);

        for (size_t r = 0; r < Rows; ++r) {
            for (size_t c = 0; c < Cols; ++c) {
                // 1. Update Velocity: V = beta * V + (1 - beta) * Gradient
                V(r, c) = (beta * V(r, c)) + ((1.0 - beta) * grad(r, c));

                // 2. Update Weights: W = W - alpha * V
                W(r, c) = W(r, c) - (alpha * V(r, c));
            }
        }

        if (epoch % 20 == 0 || epoch == 1) {
            std::cout << "\n";
            std::cout << "Epoch " << epoch << "\n";
            std::cout << " --------- Weight ---------\n";
            W.print();
            std::cout << " --------- Velocity --------- \n";
            V.print();
        }
    }
}
