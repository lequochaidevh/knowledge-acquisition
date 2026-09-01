#pragma once
#include "Layer.h"

class SigmoidLayer : public Layer {
 private:
    Tensor2D output_cache;  // Storing output state is mathematically cleaner for Sigmoid
 public:
    // Explicitly define the default constructor to override any deleted state
    SigmoidLayer() : output_cache(0, 0) {}

    Tensor2D forward(const Tensor2D& input) override {
        this->output_cache = std::move(input.sigmoid());
        return this->output_cache;
    }
    Tensor2D backward(const Tensor2D& incoming_gradient) override {
        // local_grad = out * (1.0f - out)
        return this->output_cache.sigmoid_backward(incoming_gradient);
    }
};