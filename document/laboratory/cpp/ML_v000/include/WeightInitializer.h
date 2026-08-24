#pragma once

#include "Tensor2D.h"

class WeightInitializer {
 public:
    // Kaiming (He) Normal Initialization
    // - Compulsory standard for ReLU
    // and LeakyReLU layers
    static void kaiming_normal(Tensor2D& weight, size_t in_features) {
        // std::random_device rd;
        std::mt19937 gen(42);

        // Calculate the optimal standard deviation:
        // sqrt(2.0 / in_features)
        float                           std_dev = std::sqrt(2.0f / static_cast<float>(in_features));
        std::normal_distribution<float> d(0.0f, std_dev);

        for (size_t i = 0; i < weight.get_rows(); ++i) {
            for (size_t j = 0; j < weight.get_cols(); ++j) {
                weight.at(i, j) = d(gen);
            }
        }
    }

    // Xavier (Glorot) Normal Initialization - standard
    // optimization for Sigmoid/Tanh output nodes
    static void xavier_normal(Tensor2D& weight, size_t in_features, size_t out_features) {
        // std::random_device rd;
        std::mt19937 gen(42);

        // Calculate the optimal standard
        // deviation: sqrt(2.0f / (in_features + out_features))
        float                           std_dev = std::sqrt(2.0f / static_cast<float>(in_features + out_features));
        std::normal_distribution<float> d(0.0f, std_dev);

        for (size_t i = 0; i < weight.get_rows(); ++i) {
            for (size_t j = 0; j < weight.get_cols(); ++j) {
                weight.at(i, j) = d(gen);
            }
        }
    }
};
