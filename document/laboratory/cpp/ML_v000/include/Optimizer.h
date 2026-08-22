#pragma once
#include "DenseLayer.h"

class SGDOptimizer {
 private:
    float learning_rate;

 public:
    SGDOptimizer(float lr = 0.01f) : learning_rate(lr) {}

    void step(DenseLayer& layer) {
        // 1. Update weight: W = W - lr * dW
        for (size_t i = 0; i < layer.weights.get_rows(); ++i) {
            for (size_t j = 0; j < layer.weights.get_cols(); ++j) {
                layer.weights.at(i, j) -= learning_rate * layer.d_weights.at(i, j);
            }
        }

        // 2. Update bias B = B - lr * dB
        for (size_t j = 0; j < layer.bias.get_cols(); ++j) {
            layer.bias.at(0, j) -= learning_rate * layer.d_bias.at(0, j);
        }
    }
};
