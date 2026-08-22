#pragma once

#include "Tensor2D.h"

class DenseLayer {
 public:
    Tensor2D weights;
    Tensor2D bias;

    // Cached variables for backpropagation
    Tensor2D input_cache;
    Tensor2D d_weights;
    Tensor2D d_bias;

    DenseLayer(size_t in_features, size_t out_features)
        : weights(in_features, out_features, 0.0f),
          bias(1, out_features, 0.0f),
          input_cache(0, 0),
          d_weights(in_features, out_features, 0.0f),
          d_bias(1, out_features, 0.0f) {
        std::random_device              rd;
        std::mt19937                    gen(rd());
        std::normal_distribution<float> d(0.0f, 0.1f);

        for (size_t i = 0; i < in_features; ++i) {
            for (size_t j = 0; j < out_features; ++j) {
                weights.at(i, j) = d(gen);
            }
        }
    }

    void zero_gradients() {
        // (Assuming d_weights and d_bias share the same dimensions as weights and bias)
        for (size_t i = 0; i < d_weights.get_rows(); ++i) {
            for (size_t j = 0; j < d_weights.get_cols(); ++j) {
                this->d_weights.at(i, j) = 0.0f;
            }
        }

        for (size_t j = 0; j < d_bias.get_cols(); ++j) {
            this->d_bias.at(0, j) = 0.0f;
        }
    }

    // Forward pass with input caching
    Tensor2D forward(const Tensor2D& input) {
        this->input_cache = input;  // Store input for backward pass

        Tensor2D output = input.matmul(weights);
        size_t   rows   = output.get_rows();
        size_t   cols   = output.get_cols();

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                output.at(i, j) += bias.at(0, j);
            }
        }
        return output;
    }

    // Backward pass computing gradients
    Tensor2D backward(const Tensor2D& incoming_gradient) {
        // (Instead of using '=', accumulate the matrix multiplication result
        // into d_weights)
        Tensor2D input_T           = this->input_cache.transpose();
        Tensor2D current_d_weights = input_T.matmul(incoming_gradient);

        // (Accumulate into existing d_weights)
        for (size_t i = 0; i < d_weights.get_rows(); ++i) {
            for (size_t j = 0; j < d_weights.get_cols(); ++j) {
                this->d_weights.at(i, j) += current_d_weights.at(i, j);
            }
        }

        // (Same for bias: Remove the zero-reset loop from here, just accumulate directly into d_bias)
        size_t rows = incoming_gradient.get_rows();
        size_t cols = incoming_gradient.get_cols();
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                this->d_bias.at(0, j) += incoming_gradient.at(i, j);
            }
        }

        // (Calculate gradient to pass back)
        Tensor2D weights_T = this->weights.transpose();
        Tensor2D d_input   = incoming_gradient.matmul(weights_T);

        return d_input;
    }
};
