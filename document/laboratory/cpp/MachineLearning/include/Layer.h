#pragma once
#include "Tensor2D.h"

class Layer {
 public:
    virtual ~Layer() = default;

    // Core deep learning execution interface
    virtual Tensor2D forward(const Tensor2D& input)              = 0;
    virtual Tensor2D backward(const Tensor2D& incoming_gradient) = 0;

    // Optional hooks for layers that contain weights/biases
    virtual void                             zero_gradients() {}
    virtual std::map<std::string, Tensor2D*> get_parameters(const std::string& prefix) {
        return {};  // Non-parametric layers (ReLU, Sigmoid) return empty maps safely
    }

    // Add this virtual hook for parameter updates
    virtual void update_parameters(float learning_rate) {}
};