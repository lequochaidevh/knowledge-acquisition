/**
 * @file matrix.h
 * @brief High-performance compile-time Matrix Library for ML pipelines.
 * Compatible with C++17 standards and above.
 */
#pragma once
#include "../../MachineLearning/include/std17pch.h"

// Forward declaration of Matrix to allow template usage
template <size_t Rows, size_t Cols = Rows>
struct Matrix {
    std::array<double, Rows * Cols> data{};

    constexpr Matrix() = default;

    // Constexpr constructor for compile-time initialization
    constexpr Matrix(std::array<double, Rows * Cols> init_data) : data(init_data) {}

    // C++17 friendly initializer list constructor
    constexpr Matrix(std::initializer_list<double> init_list) {
        std::size_t i = 0;
        for (double val : init_list) {
            if (i >= Rows * Cols) break;
            data[i++] = val;
        }
    }

    // Fast inline element accessors
    [[nodiscard]] constexpr double operator()(size_t r, size_t c) const { return data[r * Cols + c]; }

    [[nodiscard]] constexpr double& operator()(size_t r, size_t c) { return data[r * Cols + c]; }

    // Matrix Transpose: (Rows x Cols) -> (Cols x Rows)
    [[nodiscard]] constexpr Matrix<Cols, Rows> transpose() const {
        Matrix<Cols, Rows> result;
        for (std::size_t r = 0; r < Rows; ++r) {
            for (std::size_t c = 0; c < Cols; ++c) {
                result(c, r) = (*this)(r, c);
            }
        }
        return result;
    }

    // 2. Custom Functional Transform (applies a function to each element)
    // [[nodiscard]] instructs the compiler to warn you if you forget to catch the returned matrix
    template <typename Func>
    [[nodiscard]] constexpr Matrix<Rows, Cols> transform(Func f) const {
        Matrix<Rows, Cols> result;
        for (std::size_t i = 0; i < Rows * Cols; ++i) {
            result.data[i] = f(data[i]);
        }
        return result;
    }

    // Compiles in constexpr context, executes beautifully at runtime
    constexpr void print() const {
        for (size_t i = 0; i < Rows; ++i) {
            std::cout << "[ ";
            for (size_t j = 0; j < Cols; ++j) {
                // Accesses memory sequentially for maximum cache hits
                std::cout << (*this)(i, j) << (j == Cols - 1 ? "" : ", ");
            }
            std::cout << " ]\n";
        }
    }

    // Zero out all elements in the matrix (Essential for initializing velocity)
    constexpr void zero_fill() { data.fill(0.0); }
};

// --- DETERMINANT CORNER (Safe for both Square & Non-Square) ---

// Helper function to extract a minor matrix from a general matrix
template <size_t Rows, size_t Cols>
[[nodiscard]] constexpr Matrix<Rows - 1, Cols - 1> get_minor(const Matrix<Rows, Cols>& A, size_t exclude_row,
                                                             size_t exclude_col) {
    std::array<double, (Rows - 1) * (Cols - 1)> minor_data{};
    size_t                                      target_idx = 0;

    for (size_t r = 0; r < Rows; ++r) {
        if (r == exclude_row) continue;
        for (size_t c = 0; c < Cols; ++c) {
            if (c == exclude_col) continue;
            minor_data[target_idx++] = A(r, c);
        }
    }
    return Matrix<Rows - 1, Cols - 1>(minor_data);
}

// Unified Determinant function supporting compile-time validation
template <size_t Rows, size_t Cols>
[[nodiscard]] constexpr double determinant(const Matrix<Rows, Cols>& A) {
    // Compile-time safety checkpoint
    static_assert(Rows == Cols, "ERROR: Determinant can ONLY be calculated for SQUARE matrices!");

    if constexpr (Rows == 1) {
        return A(0, 0);
    } else if constexpr (Rows == 2) {
        return (A(0, 0) * A(1, 1)) - (A(0, 1) * A(1, 0));
    } else {
        double det  = 0.0;
        double sign = 1.0;

        for (size_t c = 0; c < Cols; ++c) {
            double minor_det = determinant(get_minor(A, 0, c));
            det += sign * A(0, c) * minor_det;
            sign = -sign;
        }
        return det;
    }
}

// --- MULTIPLICATION CORNER (Supports M x K * K x N) ---
template <size_t M, size_t K, size_t N>
[[nodiscard]] constexpr Matrix<M, N> multiply(const Matrix<M, K>& A, const Matrix<K, N>& B) {
    Matrix<M, N> result{};

    for (size_t i = 0; i < M; ++i) {
        for (size_t k = 0; k < K; ++k) {
            double temp = A(i, k);
            for (size_t j = 0; j < N; ++j) {
                result(i, j) += temp * B(k, j);
            }
        }
    }
    return result;
}

// Dedicated overload for multiplying shortened square matrices Matrix<N>
template <size_t N>
[[nodiscard]] constexpr Matrix<N, N> multiply(const Matrix<N, N>& A, const Matrix<N, N>& B) {
    return multiply<N, N, N>(A, B);
}

// Structure to hold the runtime results of NxN Eigen-decomposition
template <size_t N>
struct DominantEigen {
    double                eigenvalue;
    std::array<double, N> eigenvector;
};

// Add this helper function above your power_iteration function
[[nodiscard]] constexpr double constexpr_sqrt(double x) {
    if (x < 0.0) return 0.0;  // Simplistic error handling for negative values
    if (x == 0.0 || x == 1.0) return x;

    double curr = x;
    double prev = 0.0;

    // Newton's method loop (guaranteed to converge quickly for sqrt)
    // We cannot use std::abs here either, so use explicit conditional checks
    while ((curr - prev > 1e-15) || (prev - curr > 1e-15)) {
        prev = curr;
        curr = 0.5 * (curr + (x / curr));
    }
    return curr;
}

// Power Iteration Algorithm for general N x N Square Matrices
template <size_t N>
[[nodiscard]] constexpr DominantEigen<N> power_iteration(const Matrix<N, N>& A, size_t max_iterations = 1000,
                                                         double tolerance = 1e-7) {
    static_assert(N > 0, "Matrix size must be greater than 0");

    // Step 1: Initialize a random/default guess vector (all 1.0s)
    std::array<double, N> b_k{1.0};

    double eigenvalue = 0.0;

    for (size_t iter = 0; iter < max_iterations; ++iter) {
        // Step 2: Calculate the product matrix A * b_k
        std::array<double, N> b_k1{};
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                b_k1[i] += A(i, j) * b_k[j];
            }
        }

        // Step 3: Compute the norm (length) of the new vector
        double norm = 0.0;
        for (size_t i = 0; i < N; ++i) {
            norm += b_k1[i] * b_k1[i];
        }
        // norm = std::sqrt(norm);

        norm = constexpr_sqrt(norm);

        // Avoid division by zero if matrix is singular
        if (norm < 1e-12) break;

        // Step 4: Normalize the vector to prevent numerical overflow
        for (size_t i = 0; i < N; ++i) {
            b_k1[i] /= norm;
        }

        // Step 5: Rayleigh Quotient to estimate the eigenvalue: (b_k^T * A * b_k) / (b_k^T * b_k)
        double next_eigenvalue = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double ax = 0.0;
            for (size_t j = 0; j < N; ++j) {
                ax += A(i, j) * b_k1[j];
            }
            next_eigenvalue += b_k1[i] * ax;
        }

        // Step 6: Check for convergence
        // if (std::abs(next_eigenvalue - eigenvalue) < tolerance) {
        //    eigenvalue = next_eigenvalue;
        //    b_k        = b_k1;
        //    break;
        //}

        // Replace std::abs with a standard C++17 ternary operator
        double diff     = next_eigenvalue - eigenvalue;
        double abs_diff = (diff < 0.0) ? -diff : diff;

        // Step 6: Check for convergence
        if (abs_diff < tolerance) {
            eigenvalue = next_eigenvalue;
            b_k        = b_k1;
            break;
        }

        eigenvalue = next_eigenvalue;
        b_k        = b_k1;
    }

    return {eigenvalue, b_k};
}
