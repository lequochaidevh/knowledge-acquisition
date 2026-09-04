#pragma once
#include "Tensor4D.h"

class MaxPool2DLayer {
 private:
    size_t pool_size;
    size_t stride;

 public:
    // Memory cache placeholders required during the backward backprop pass to route gradients
    Tensor4D input_cache;

    // Standard default configuration sets window size = 2, stride step = 2 (Halves the image dimensions)
    MaxPool2DLayer(size_t p_size = 2, size_t strd = 2) : pool_size(p_size), stride(strd), input_cache() {}

    // Core Execution: Forward pass filtering max features out of local bounding blocks
    Tensor4D forward(const Tensor4D& input) {
        this->input_cache = input;

        size_t B = input.get_batch();
        size_t C = input.get_channels();
        size_t H = input.get_height();
        size_t W = input.get_width();

        // Calculate exact mathematical pooled dimensions output profiles
        size_t out_H = ((H - pool_size) / stride) + 1;
        size_t out_W = ((W - pool_size) / stride) + 1;

        Tensor4D output(B, C, out_H, out_W, 0.0f);

// OpenMP Multi-threaded instruction unrolling across batch and independent feature channels
#pragma omp parallel for collapse(2) schedule(static)
        for (size_t b = 0; b < B; ++b) {
            for (size_t c = 0; c < C; ++c) {
                for (size_t oh = 0; oh < out_H; ++oh) {
                    for (size_t ow = 0; ow < out_W; ++ow) {
                        // Set tracking anchor to absolute lowest negative limit
                        float  max_value  = -INFINITY;
                        size_t in_h_start = oh * stride;
                        size_t in_w_start = ow * stride;

                        // Scan elements inside the local pool window bounds
                        for (size_t kh = 0; kh < pool_size; ++kh) {
                            for (size_t kw = 0; kw < pool_size; ++kw) {
                                size_t curr_h = in_h_start + kh;
                                size_t curr_w = in_w_start + kw;

                                if (curr_h < H && curr_w < W) {
                                    float current_pixel = input.at(b, c, curr_h, curr_w);
                                    if (current_pixel > max_value) {
                                        max_value = current_pixel;
                                    }
                                }
                            }
                        }
                        // Assign peak maximum feature value back into output array coordinates slots
                        output.at(b, c, oh, ow) = max_value;
                    }
                }
            }
        }
        return output;
    }
};