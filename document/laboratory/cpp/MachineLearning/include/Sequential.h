#pragma once
#include "Layer.h"
#include "ModelCheckpoint.h"
#include <vector>

class Sequential {
 private:
    std::vector<Layer*> layers;

 public:
    ~Sequential() {
        for (Layer* layer : layers) {
            delete layer;  // Prevent memory leaks
        }
    }

    const std::vector<Layer*>& get_layers() const { return this->layers; }

    // Append a new layer to the execution stack
    void add(Layer* layer) { layers.push_back(layer); }

    // Forward pass: loop through layers from index 0 to N
    Tensor2D forward(const Tensor2D& input) {
        // Copy but will optimize if need
        Tensor2D current_activation = input;
        for (Layer* layer : layers) {
            current_activation = std::move(layer->forward(current_activation));
        }
        return current_activation;
    }

    // Backward pass: reverse loop through layers from index N to 0
    Tensor2D backward(const Tensor2D& loss_gradient) {
        // Copy but will optimize if need
        Tensor2D current_gradient = loss_gradient;
        for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
            current_gradient = std::move((*it)->backward(current_gradient));
        }
        return current_gradient;
    }

    // Reset all parameters gradients inside layers
    void zero_grad() {
        for (Layer* layer : layers) {
            layer->zero_gradients();
        }
    }

    // Collect all parameters from internal layers and save to file cleanly
    void save(const std::string& filepath) {
        ModelCheckpoint checkpoint;
        for (size_t i = 0; i < layers.size(); ++i) {
            std::string prefix       = "layer_" + std::to_string(i);
            auto        layer_params = layers[i]->get_parameters(prefix);
            for (auto& [name, tensor_ptr] : layer_params) {
                checkpoint.register_parameter(name, *tensor_ptr);
            }
        }
        checkpoint.save(filepath);
    }

    // Load parameters into internal matching layer keys from file cleanly
    void load(const std::string& filepath) {
        ModelCheckpoint checkpoint;
        for (size_t i = 0; i < layers.size(); ++i) {
            std::string prefix       = "layer_" + std::to_string(i);
            auto        layer_params = layers[i]->get_parameters(prefix);
            for (auto& [name, tensor_ptr] : layer_params) {
                checkpoint.register_parameter(name, *tensor_ptr);
            }
        }
        checkpoint.load(filepath);
    }
};