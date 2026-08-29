// Inside AdamOptimizer.h
#pragma once
#include "Layer.h"
#include "DenseLayer.h"
#include "Sequential.h"
#include <vector>
#include <map>
#include <cmath>

class AdamOptimizer {
 private:
    float learning_rate;
    float beta1;
    float beta2;
    float epsilon;
    int   time_step;

 public:
    AdamOptimizer(float lr = 0.001f, float b1 = 0.9f, float b2 = 0.999f, float eps = 1e-8f)
        : learning_rate(lr), beta1(b1), beta2(b2), epsilon(eps), time_step(0) {}

    void step(Layer& layer) {
        DenseLayer* dense = dynamic_cast<DenseLayer*>(&layer);
        if (dense == nullptr) return;

        // Safely trigger local Adam parameters memory allocation
        dense->init_adam_states();

        Tensor2D&       W  = dense->weights;
        Tensor2D&       B  = dense->bias;
        const Tensor2D& dW = dense->d_weights;
        const Tensor2D& dB = dense->d_bias;

        // Directly pull historical state buffers stored safely inside the layer object
        Tensor2D& mw = dense->m_w;
        Tensor2D& vw = dense->v_w;
        Tensor2D& mb = dense->m_b;
        Tensor2D& vb = dense->v_b;

        // 1. Update weights parameters element-wise
        for (size_t i = 0; i < W.get_rows(); ++i) {
            for (size_t j = 0; j < W.get_cols(); ++j) {
                float dw = dW.at(i, j);

                mw.at(i, j) = beta1 * mw.at(i, j) + (1.0f - beta1) * dw;
                vw.at(i, j) = beta2 * vw.at(i, j) + (1.0f - beta2) * dw * dw;

                float m_hat = mw.at(i, j) / (1.0f - std::pow(beta1, time_step));
                float v_hat = vw.at(i, j) / (1.0f - std::pow(beta2, time_step));

                W.at(i, j) -= (learning_rate / (std::sqrt(v_hat) + epsilon)) * m_hat;
            }
        }

        // 2. Update bias parameters element-wise
        for (size_t j = 0; j < B.get_cols(); ++j) {
            float db = dB.at(0, j);

            mb.at(0, j) = beta1 * mb.at(0, j) + (1.0f - beta1) * db;
            vb.at(0, j) = beta2 * vb.at(0, j) + (1.0f - beta2) * db * db;

            float m_hat_b = mb.at(0, j) / (1.0f - std::pow(beta1, time_step));
            float v_hat_b = vb.at(0, j) / (1.0f - std::pow(beta2, time_step));

            B.at(0, j) -= (learning_rate / (std::sqrt(v_hat_b) + epsilon)) * m_hat_b;
        }
    }

    void step(Sequential& model) {
        if (this->time_step < 200) {
            this->time_step++;
        }
        for (Layer* layer : model.get_layers()) {
            this->step(*layer);
        }
    }
};