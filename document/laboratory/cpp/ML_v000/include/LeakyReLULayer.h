#pragma once
#include "Layer.h"

class LeakyReLULayer : public Layer {
 private:
    Tensor2D input_cache;  // Store input state for computing derivative
 public:
    // Explicitly define the default constructor to override any deleted state
    LeakyReLULayer() : input_cache(0, 0) {}

    Tensor2D forward(const Tensor2D& input) override {
        this->input_cache = input;
        Tensor2D result(input.get_rows(), input.get_cols());
        for (size_t i = 0; i < input.get_rows(); ++i) {
            for (size_t j = 0; j < input.get_cols(); ++j) {
                float x         = input.at(i, j);
                result.at(i, j) = (x > 0.0f) ? x : 0.01f * x;  // LeakyReLU math rule
            }
        }
        return result;
    }

    Tensor2D backward(const Tensor2D& incoming_gradient) override {
        Tensor2D d_input(input_cache.get_rows(), input_cache.get_cols());
        for (size_t i = 0; i < input_cache.get_rows(); ++i) {
            for (size_t j = 0; j < input_cache.get_cols(); ++j) {
                float x          = input_cache.at(i, j);
                float local_grad = (x > 0.0f) ? 1.0f : 0.01f;  // LeakyReLU derivative
                d_input.at(i, j) = incoming_gradient.at(i, j) * local_grad;
            }
        }
        return d_input;
    }
};
