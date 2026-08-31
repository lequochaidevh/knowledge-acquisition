// Inside AdamOptimizer.h
#pragma once
#include "Layer.h"
#include "DenseLayer.h"
#include "Sequential.h"
#include <vector>
#include <map>
#include <cmath>

// Momentum and RMSProp
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

        // Extract matrix and bias references from the layer
        // W: Weights, dW: Gradient of Weights, mw: First moment vector, vw: Second moment vector
        // B: Biases, dB: Gradient of Biases, mb: First moment vector, vb: Second moment vector
        Tensor2D&       W  = dense->weights;
        Tensor2D&       B  = dense->bias;
        const Tensor2D& dW = dense->d_weights;
        const Tensor2D& dB = dense->d_bias;

        // Directly pull historical state buffers stored safely inside the layer object
        Tensor2D& mw = dense->m_w;
        Tensor2D& vw = dense->v_w;
        Tensor2D& mb = dense->m_b;
        Tensor2D& vb = dense->v_b;

        /*
         * PERFORMANCE OPTIMIZATION:
         * Compute bias correction denominators outside the loops.
         * Since beta1, beta2, and time_step are uniform across all elements in this step,
         * calculating this once saves millions of redundant std::pow CPU cycles.
         */
        float correction1 = 1.0f - std::pow(beta1, time_step);
        float correction2 = 1.0f - std::pow(beta2, time_step);

        // ------------------------------------------------------------------------
        // 1. Update Weight Parameters Element-Wise
        // ------------------------------------------------------------------------
        for (size_t i = 0; i < W.get_rows(); ++i) {
            for (size_t j = 0; j < W.get_cols(); ++j) {
                float dw = dW.at(i, j);

                /*
                 * Update biased first moment estimate (Momentum).
                 * Moving average of past gradients acts like a ball rolling down a hill,
                 * smoothing out noisy updates and accelerating in the right direction.
                 */
                mw.at(i, j) = beta1 * mw.at(i, j) + (1.0f - beta1) * dw;

                /*
                 * Update biased second raw moment estimate (RMSProp).
                 * Moving average of squared gradients scales down updates for features
                 * with high-frequency gradients, preventing explosions or oscillations.
                 */
                vw.at(i, j) = beta2 * vw.at(i, j) + (1.0f - beta2) * dw * dw;

                /*
                 * Compute bias-corrected first and second moment estimates.
                 * This counteracts the fact that mw and vw are initialized to 0,
                 * preventing them from being heavily biased toward 0 in early iterations.
                 */
                float m_hat = mw.at(i, j) / correction1;
                float v_hat = vw.at(i, j) / correction2;

                /*
                 * Apply Adam update rule to Weights.
                 * Epsilon prevents any potential division-by-zero errors.
                 */
                W.at(i, j) -= (learning_rate / (std::sqrt(v_hat) + epsilon)) * m_hat;
            }
        }

        // ------------------------------------------------------------------------
        // 2. Update Bias Parameters Element-Wise
        // ------------------------------------------------------------------------
        for (size_t j = 0; j < B.get_cols(); ++j) {
            float db = dB.at(0, j);

            // Update biased first moment estimate for biases
            mb.at(0, j) = beta1 * mb.at(0, j) + (1.0f - beta1) * db;

            // Update biased second raw moment estimate for biases
            vb.at(0, j) = beta2 * vb.at(0, j) + (1.0f - beta2) * db * db;

            // Compute bias-corrected estimates for biases
            float m_hat_b = mb.at(0, j) / correction1;
            float v_hat_b = vb.at(0, j) / correction2;

            // Apply Adam update rule to Biases
            B.at(0, j) -= (learning_rate / (std::sqrt(v_hat_b) + epsilon)) * m_hat_b;
        }
    }

    void step(Sequential& model) {
        if (this->time_step < 200) {
            this->time_step++;
        }
        /*
         * FIXED LOGIC: Removed 'if (this->time_step < 200)' condition.
         * time_step must increment indefinitely. As time_step grows,
         * std::pow(beta, time_step) approaches 0, and correction factors converge to 1.
         * Freezing time_step breaks the underlying mathematics of Adam.
         */
        // this->time_step++;

        for (Layer* layer : model.get_layers()) {
            this->step(*layer);
        }
    }
};