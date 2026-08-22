#pragma once

#include "Tensor2D.h"

class MSELoss {
 public:
    // Calc averg Loss: 1/N * sum((pred - target)^2)
    float forward(const Tensor2D& pred, const Tensor2D& target) {
        if (pred.get_rows() != target.get_rows() || pred.get_cols() != target.get_cols()) {
            throw std::invalid_argument("Dimentions do not match in Loss forward.");
        }

        float  total_loss = 0.0f;
        size_t rows       = pred.get_rows();
        size_t cols       = pred.get_cols();
        size_t n          = rows * cols;

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                float diff = pred.at(i, j) - target.at(i, j);
                total_loss += diff * diff;
            }
        }
        return total_loss / n;
    }

    // Derivative of the MSE Loss: 2/N * (pred - target)
    // in order to back propagation
    Tensor2D backward(const Tensor2D& pred, const Tensor2D& target) {
        size_t rows = pred.get_rows();
        size_t cols = pred.get_cols();
        size_t n    = rows * cols;

        Tensor2D gradient(rows, cols, 0.0f);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                gradient.at(i, j) = (2.0f / n) * (pred.at(i, j) - target.at(i, j));
            }
        }
        return gradient;
    }
};

class BCELoss {
 public:
    // Forward pass: Calculate average Binary Cross-Entropy Loss
    float forward(const Tensor2D& pred, const Tensor2D& target) {
        size_t rows = pred.get_rows();
        size_t cols = pred.get_cols();

        float total_loss = 0.0f;
        float epsilon    = 1e-7f;  // Small constant value to avoid log(0) undefined stability crash

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                float p = pred.at(i, j);
                float t = target.at(i, j);

                // Clip values to guarantee stability boundary limits
                if (p < epsilon) p = epsilon;
                if (p > 1.0f - epsilon) p = 1.0f - epsilon;

                total_loss += -(t * std::log(p) + (1.0f - t) * std::log(1.0f - p));
            }
        }
        return total_loss / static_cast<float>(rows * cols);
    }

    // Backward pass: Compute gradient of loss with respect to predictions
    Tensor2D backward(const Tensor2D& pred, const Tensor2D& target) {
        size_t   rows = pred.get_rows();
        size_t   cols = pred.get_cols();
        Tensor2D loss_grad(rows, cols);

        float epsilon = 1e-7f;
        float n       = static_cast<float>(rows * cols);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                float p = pred.at(i, j);
                float t = target.at(i, j);

                if (p < epsilon) p = epsilon;
                if (p > 1.0f - epsilon) p = 1.0f - epsilon;

                // Mathematical derivative formula of BCE Loss function
                loss_grad.at(i, j) = ((p - t) / (p * (1.0f - p))) / n;
            }
        }
        return loss_grad;
    }
};