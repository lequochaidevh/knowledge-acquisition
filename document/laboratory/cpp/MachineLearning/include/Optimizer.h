#pragma once
#include "DenseLayer.h"

class SGDOptimizer {
 private:
    float learning_rate;

 public:
    SGDOptimizer(float lr = 0.01f) : learning_rate(lr) {}

    void step(Layer& layer) {
        layer.update_parameters(learning_rate);  // Polymorphic call execution
    }

    void step(Sequential& model) {
        for (Layer* layer : model.get_layers()) {
            layer->update_parameters(learning_rate);
        }
    }
};
