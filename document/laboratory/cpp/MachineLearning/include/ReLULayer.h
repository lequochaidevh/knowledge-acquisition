#pragma once
#include "Layer.h"

class ReLULayer : public Layer {
 private:
    Tensor2D input_cache;  // Store input state for computing derivative
 public:
    // Explicitly define the default constructor to override any deleted state
    ReLULayer() : input_cache(0, 0) {}

    Tensor2D forward(const Tensor2D& input) override {
        this->input_cache = std::move(input.relu());
        return this->input_cache;
    }
    Tensor2D backward(const Tensor2D& incoming_gradient) override {
        return this->input_cache.relu_backward(incoming_gradient);
    }
};
