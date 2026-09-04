// Inside FlattenLayer.h
#pragma once
#include "Tensor4D.h"
#include "Tensor2D.h"  // Ensure your legacy Tensor2D header is accessible here

class FlattenLayer {
 public:
    // Memory cache placeholders tracking structural shapes required during backprop
    size_t cached_batch;
    size_t cached_channels;
    size_t cached_height;
    size_t cached_width;

    FlattenLayer() : cached_batch(0), cached_channels(0), cached_height(0), cached_width(0) {}

    // Core Execution: Forward pass flattening a spatial 4D Tensor into a 2D Matrix row layout
    Tensor2D forward(const Tensor4D& input) {
        // Cache original dimensions topology to route backprop gradients accurately later
        this->cached_batch    = input.get_batch();
        this->cached_channels = input.get_channels();
        this->cached_height   = input.get_height();
        this->cached_width    = input.get_width();

        size_t B = input.get_batch();
        // Total flattened features length calculation = C * H * W
        size_t total_features = input.get_channels() * input.get_height() * input.get_width();

        // Instantiate a 2D Matrix container matching target bounds
        Tensor2D output(B, total_features);

        // Fetch raw memory buffer array pointers directly to unlock high speed data mapping
        const float* raw_input_ptr = input.get_raw_data();

// Assuming your Tensor2D implementation exposes a raw data pointer via get_raw_data()
// or allows direct array assignments. If not, use standard loop copying:
#pragma omp parallel for schedule(static)
        for (size_t i = 0; i < B * total_features; ++i) {
            // Map flat vector elements directly across memory boundaries
            // This replaces slow iterative multi-dimensional coordinate multi-loops tracking
            size_t row_idx = i / total_features;
            size_t col_idx = i % total_features;

            // Assuming your Tensor2D has a standard setting or .at(row, col) mutator method
            output.at(row_idx, col_idx) = raw_input_ptr[i];
        }

        return output;
    }

    // Backward pass: Reshapes 2D gradient matrices back into 4D structural tensor blocks
    Tensor4D backward(const Tensor2D& incoming_gradient) {
        // Instantiate a blank 4D Tensor matching cached dimension signatures
        Tensor4D d_input(cached_batch, cached_channels, cached_height, cached_width);

        float* raw_d_input_ptr = d_input.get_raw_data();
        size_t total_elements  = cached_batch * cached_channels * cached_height * cached_width;

#pragma omp parallel for schedule(static)
        for (size_t i = 0; i < total_elements; ++i) {
            size_t row_idx = i / (cached_channels * cached_height * cached_width);
            size_t col_idx = i % (cached_channels * cached_height * cached_width);

            // Stream elements back into the 4D topological channel coordinates mapping slots
            raw_d_input_ptr[i] = incoming_gradient.at(row_idx, col_idx);
        }

        return d_input;
    }
};
