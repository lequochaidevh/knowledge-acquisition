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
        // 1. d_weights = input^T * incoming_gradient
        Tensor2D input_T = this->input_cache.transpose();
        this->d_weights  = input_T.matmul(incoming_gradient);

        // 2. d_bias = sum of incoming_gradient columns across rows
        size_t rows = incoming_gradient.get_rows();
        size_t cols = incoming_gradient.get_cols();

        // Reset d_bias to 0
        for (size_t j = 0; j < cols; ++j) {
            this->d_bias.at(0, j) = 0.0f;
        }

        // Accumulate bias gradients
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                this->d_bias.at(0, j) += incoming_gradient.at(i, j);
            }
        }

        // 3. Gradient to pass to the previous layer: incoming_gradient * weights^T
        Tensor2D weights_T = this->weights.transpose();
        Tensor2D d_input   = incoming_gradient.matmul(weights_T);

        return d_input;
    }
};
