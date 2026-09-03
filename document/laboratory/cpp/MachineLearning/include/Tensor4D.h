#pragma once
#include "std17pch.h"

class Tensor4D {
 private:
    size_t batch;
    size_t channels;
    size_t height;
    size_t width;

    // Strides descriptors for zero-overhead multi-dimensional pointer navigation jumping
    size_t stride_b;
    size_t stride_c;
    size_t stride_h;

    // Core underlying high-performance contiguous layout memory block array
    std::vector<float> data;

 public:
    // Default constructor creating a blank scalar placeholder layout
    Tensor4D() : batch(0), channels(0), height(0), width(0), stride_b(0), stride_c(0), stride_h(0), data() {}

    // Main structural tensor allocator parameter constructor
    Tensor4D(size_t b, size_t c, size_t h, size_t w, float init_val = 0.0f)
        : batch(b), channels(c), height(h), width(w) {
        // Pre-compute strides array limits to unlock lightning fast O(1) index mappings
        this->stride_h = width;
        this->stride_c = height * width;
        this->stride_b = channels * height * width;

        // Allocate unified contiguous alignment block space in heap storage safely
        this->data.assign(batch * channels * height * width, init_val);
    }

    // High-performance Inline Element Accessor (Zero runtime call overhead via flattening mappings)
    inline float& at(size_t b, size_t c, size_t h, size_t w) {
        return data[b * stride_b + c * stride_c + h * stride_h + w];
    }

    inline const float& at(size_t b, size_t c, size_t h, size_t w) const {
        return data[b * stride_b + c * stride_c + h * stride_h + w];
    }

    // Accessors for architectural topology metadata tracking bounds
    size_t       get_batch() const { return batch; }
    size_t       get_channels() const { return channels; }
    size_t       get_height() const { return height; }
    size_t       get_width() const { return width; }
    float*       get_raw_data() { return data.data(); }
    const float* get_raw_data() const { return data.data(); }

    // =========================================================================
    // HIGH OPTIMIZED BLAS-STYLE GEMM MATRIX MULTIPLICATION
    // =========================================================================
    // This removes the need for calling .transpose() explicitly.
    // transA = true means treat matrix A as transposed.
    // transB = true means treat matrix B as transposed.
    // Performs: C = A * B over 2D slice matrices dimensions layout securely.
    static void matmul_gemm(const Tensor4D& A, bool transA, const Tensor4D& B, bool transB, Tensor4D& C) {
        // Extract logical dimensions based on activation hints matrix transformations flags
        size_t A_rows = transA ? A.get_width() : A.get_height();
        size_t A_cols = transA ? A.get_height() : A.get_width();
        size_t B_rows = transB ? B.get_width() : B.get_height();
        size_t B_cols = transB ? B.get_height() : B.get_width();

        if (A_cols != B_rows) {
            throw std::invalid_argument("GEMM COMPILER ERROR: Incompatible inner matrix dimensions.");
        }

        // Allocate or assert output container shape structures alignment profiles
        if (C.get_height() != A_rows || C.get_width() != B_cols || C.get_batch() != A.get_batch() ||
            C.get_channels() != A.get_channels()) {
            C = Tensor4D(A.get_batch(), A.get_channels(), A_rows, B_cols, 0.0f);
        }

        // Execute batch and multi-channel slicing tracking maps parallelly
        for (size_t b = 0; b < A.get_batch(); ++b) {
            for (size_t c = 0; c < A.get_channels(); ++c) {
// Hardware Parallel Loop unrolling block instructions injection hook
#pragma omp parallel for collapse(2) schedule(static)
                for (size_t i = 0; i < A_rows; ++i) {
                    for (size_t j = 0; j < B_cols; ++j) {
                        float accumulator = 0.0f;

                        for (size_t k = 0; k < A_cols; ++k) {
                            // Map memory stride coordinates on-the-fly bypassing dynamic RAM allocations
                            size_t A_h = transA ? k : i;
                            size_t A_w = transA ? i : k;
                            size_t B_h = transB ? j : k;
                            size_t B_w = transB ? k : j;

                            accumulator += A.at(b, c, A_h, A_w) * B.at(b, c, B_h, B_w);
                        }
                        C.at(b, c, i, j) = accumulator;
                    }
                }
            }
        }
    }
};