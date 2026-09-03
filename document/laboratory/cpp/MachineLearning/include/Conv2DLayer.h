#pragma once
#include "Tensor4D.h"
#include <vector>
#include <cmath>
#include <random>

class Conv2DLayer {
 private:
    size_t in_channels;
    size_t out_channels;
    size_t kernel_size;
    size_t stride;
    size_t padding;

 public:
    Tensor4D weights;  // Shape layout: [out_channels, in_channels, kernel_size, kernel_size]
    Tensor4D bias;     // Shape layout: [1, out_channels, 1, 1]
    Tensor4D d_weights;
    Tensor4D d_bias;
    Tensor4D input_cache;

    Conv2DLayer(size_t in_c, size_t out_c, size_t k_size, size_t strd = 1, size_t pad = 0)
        : in_channels(in_c), out_channels(out_c), kernel_size(k_size), stride(strd), padding(pad) {
        // Allocate weights tensor shape bounds
        this->weights = Tensor4D(out_channels, in_channels, kernel_size, kernel_size);
        this->bias    = Tensor4D(1, out_channels, 1, 1);

        this->d_weights = Tensor4D(out_channels, in_channels, kernel_size, kernel_size);
        this->d_bias    = Tensor4D(1, out_channels, 1, 1);

        // Professional Kaiming (He) Initialization for Convolutional weights tensor fields
        std::random_device rd;
        std::mt19937       gen(42);  // Lock seed to 42 for absolute stability mapping consistency
        float              bound = std::sqrt(2.0f / static_cast<float>(in_channels * kernel_size * kernel_size));
        std::normal_distribution<float> dist(0.0f, bound);

        for (size_t b = 0; b < out_channels; ++b) {
            for (size_t c = 0; c < in_channels; ++c) {
                for (size_t h = 0; h < kernel_size; ++h) {
                    for (size_t w = 0; w < kernel_size; ++w) {
                        this->weights.at(b, c, h, w) = dist(gen);
                    }
                }
            }
        }

        // Initialize bias channels elements safely to zero
        for (size_t c = 0; c < out_channels; ++c) {
            this->bias.at(0, c, 0, 0) = 0.0f;
        }
    }

    // Core Execution: Highly stable Forward pass calculation implementing Padding and Stride limits
    Tensor4D forward(const Tensor4D& input) {
        this->input_cache = input;

        size_t B = input.get_batch();
        size_t H = input.get_height();
        size_t W = input.get_width();

        // Calculate exact mathematical dimensions output profile boundaries
        size_t out_H = ((H + 2 * padding - kernel_size) / stride) + 1;
        size_t out_W = ((W + 2 * padding - kernel_size) / stride) + 1;

        Tensor4D output(B, out_channels, out_H, out_W, 0.0f);

// Multi-threaded sliding dot product processing via OpenMP parallel loop unrolling
#pragma omp parallel for collapse(2) schedule(static)
        for (size_t b = 0; b < B; ++b) {
            for (size_t oc = 0; oc < out_channels; ++oc) {
                for (size_t oh = 0; oh < out_H; ++oh) {
                    for (size_t ow = 0; ow < out_W; ++ow) {
                        float sum = 0.0f;
                        // Calculate input center anchor mapping coordinates values
                        size_t in_h_start = oh * stride - padding;
                        size_t in_w_start = ow * stride - padding;

                        for (size_t ic = 0; ic < in_channels; ++ic) {
                            for (size_t kh = 0; kh < kernel_size; ++kh) {
                                for (size_t kw = 0; kw < kernel_size; ++kw) {
                                    long long curr_h = static_cast<long long>(in_h_start + kh);
                                    long long curr_w = static_cast<long long>(in_w_start + kw);

                                    // Handle Padding boundary guard protections conditions safely
                                    if (curr_h >= 0 && curr_h < static_cast<long long>(H) && curr_w >= 0 &&
                                        curr_w < static_cast<long long>(W)) {
                                        sum +=
                                            input.at(b, ic, static_cast<size_t>(curr_h), static_cast<size_t>(curr_w)) *
                                            this->weights.at(oc, ic, kh, kw);
                                    }
                                }
                            }
                        }
                        // Add Channel-specific Bias descriptor element and assign to output matrix tensor slot
                        output.at(b, oc, oh, ow) = sum + this->bias.at(0, oc, 0, 0);
                    }
                }
            }
        }
        return output;
    }

    void zero_gradients() {
        // Safe element-wise structural gradient tensor flush routines execution mapping
        for (size_t b = 0; b < out_channels; ++b) {
            for (size_t c = 0; c < in_channels; ++c) {
                for (size_t h = 0; h < kernel_size; ++h) {
                    for (size_t w = 0; w < kernel_size; ++w) {
                        this->d_weights.at(b, c, h, w) = 0.0f;
                    }
                }
            }
            this->d_bias.at(0, b, 0, 0) = 0.0f;
        }
    }
};