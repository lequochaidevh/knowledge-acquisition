#pragma once
#include "../Calculus/calculus.h"
#include "../Matrix/matrix.h"

// 2. The Gradient Descent Optimizer Engine
template <size_t N, typename Func>
[[nodiscard]] std::array<double, N> gradient_descent(Func loss_function, const std::array<double, N>& initial_guess,
                                                     double learning_rate = 0.1, size_t max_epochs = 100,
                                                     double tolerance = 1e-6) {
    std::array<double, N> current_position = initial_guess;

    for (size_t epoch = 0; epoch < max_epochs; ++epoch) {
        // Compute the gradient vector at the current position
        std::array<double, N> grad = compute_gradient<N>(loss_function, current_position);

        // Check for convergence: if the gradient is near zero, we reached the minimum
        double grad_norm = 0.0;
        for (double g : grad) grad_norm += g * g;
        if (std::sqrt(grad_norm) < tolerance) {
            std::cout << "[Converged] Stopped early at epoch " << epoch << "\n";
            break;
        }

        // Core Update Step: Move OPPOSITE to the gradient direction
        for (size_t i = 0; i < N; ++i) {
            current_position[i] = current_position[i] - (learning_rate * grad[i]);
        }
    }

    return current_position;
}

// 2. Matrix-based Gradient Calculator using Central Finite Difference
template <size_t Rows, size_t Cols, typename Func>
[[nodiscard]] constexpr Matrix<Rows, Cols> compute_matrix_gradient(Func loss_func, const Matrix<Rows, Cols>& W,
                                                                   double h = 1e-5) {
    Matrix<Rows, Cols> grad_matrix{};

    for (size_t r = 0; r < Rows; ++r) {
        for (size_t c = 0; c < Cols; ++c) {
            // Mutate ONLY the specific element W(r, c) while keeping all other weights constant
            Matrix<Rows, Cols> W_plus  = W;
            Matrix<Rows, Cols> W_minus = W;

            W_plus(r, c) += h;
            W_minus(r, c) -= h;

            // Compute partial derivative for element at row r, col c
            grad_matrix(r, c) = (loss_func(W_plus) - loss_func(W_minus)) / (2.0 * h);
        }
    }
    return grad_matrix;
}

// 3. Matrix Gradient Descent Engine
template <size_t Rows, size_t Cols, typename Func>
[[nodiscard]] Matrix<Rows, Cols> matrix_gradient_descent(Func loss_func, Matrix<Rows, Cols> initial_W,
                                                         double learning_rate = 0.01, size_t epochs = 500,
                                                         double tolerance = 1e-5) {
    Matrix<Rows, Cols> W = initial_W;

    for (size_t epoch = 0; epoch < epochs; ++epoch) {
        // Compute the gradient matrix
        Matrix<Rows, Cols> grad = compute_matrix_gradient<Rows, Cols>(loss_func, W);

        // Compute Frobenius norm of the gradient matrix to check convergence
        double norm = 0.0;
        for (size_t r = 0; r < Rows; ++r) {
            for (size_t c = 0; c < Cols; ++c) {
                norm += grad(r, c) * grad(r, c);
            }
        }
        if (std::sqrt(norm) < tolerance) {
            std::cout << "[Converged] Stopped early at epoch " << epoch << "\n";
            break;
        }

        // Weight Update Step: W = W - alpha * Gradient_Matrix
        for (size_t r = 0; r < Rows; ++r) {
            for (size_t c = 0; c < Cols; ++c) {
                W(r, c) = W(r, c) - (learning_rate * grad(r, c));
            }
        }
    }
    return W;
}

template <size_t M, size_t K>
[[nodiscard]] double compute_mse_loss(const Matrix<M, K>& X, const Matrix<M, 1>& Y, const Matrix<K, 1>& W) {
    // Prediction: Y_pred = X * W (Dimension: M x 1)
    Matrix<M, 1> Y_pred = multiply(X, W);

    double total_squared_error = 0.0;
    for (size_t i = 0; i < M; ++i) {
        double error = Y_pred(i, 0) - Y(i, 0);
        total_squared_error += error * error;
    }

    return total_squared_error / static_cast<double>(M);
}

// Finite Difference Gradient Calculator specifically tailored for Weight Matrix W
template <size_t M, size_t K, typename Func>
[[nodiscard]] Matrix<K, 1> compute_weight_gradient(Func loss_func, const Matrix<K, 1>& W, double h = 1e-5) {
    Matrix<K, 1> grad_W{};

    for (size_t r = 0; r < K; ++r) {
        Matrix<K, 1> W_plus  = W;
        Matrix<K, 1> W_minus = W;

        W_plus(r, 0) += h;
        W_minus(r, 0) -= h;

        grad_W(r, 0) = (loss_func(W_plus) - loss_func(W_minus)) / (2.0 * h);
    }
    return grad_W;
}
