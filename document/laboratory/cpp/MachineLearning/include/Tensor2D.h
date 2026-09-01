#pragma once

#include "std17pch.h"

class Tensor2D {
 private:
    size_t             rows;
    size_t             cols;
    std::vector<float> data;
    bool               is_transposed = false;

 public:
    Tensor2D(size_t r, size_t c, float initial_value = 0.0f)
        : rows(r), cols(c), data(r * c, initial_value), is_transposed(false) {}

    Tensor2D() : rows(0), cols(0), data(), is_transposed(false) {}

    // Move Semantics
    Tensor2D(Tensor2D&& other) noexcept
        : rows(other.rows), cols(other.cols), data(std::move(other.data)), is_transposed(other.is_transposed) {
        other.rows          = 0;
        other.cols          = 0;
        other.is_transposed = false;
    }

    Tensor2D& operator=(Tensor2D&& other) noexcept {
        if (this != &other) {
            rows                = other.rows;
            cols                = other.cols;
            data                = std::move(other.data);
            is_transposed       = other.is_transposed;
            other.rows          = 0;
            other.cols          = 0;
            other.is_transposed = false;
        }
        return *this;
    }

    // Copy Semantics
    Tensor2D(const Tensor2D& other) = default;
    Tensor2D& operator=(const Tensor2D& other) = default;

    float& at(size_t r, size_t c) {
        if (is_transposed) {
            if (c >= rows || r >= cols) throw std::out_of_range("Index out of range");
            return data[c * rows + r];
        } else {
            if (r >= rows || c >= cols) throw std::out_of_range("Index out of range");
            return data[r * cols + c];
        }
    }

    const float& at(size_t r, size_t c) const {
        if (is_transposed) {
            if (c >= rows || r >= cols) throw std::out_of_range("Index out of range");
            return data[c * rows + r];
        } else {
            if (r >= rows || c >= cols) throw std::out_of_range("Index out of range");
            return data[r * cols + c];
        }
    }

    inline float operator()(size_t r, size_t c) const {
        return is_transposed ? data[c * rows + r] : data[r * cols + c];
    }

    inline float& operator()(size_t r, size_t c) { return is_transposed ? data[c * rows + r] : data[r * cols + c]; }

    size_t get_rows() const { return rows; }
    size_t get_cols() const { return cols; }

    void print() const {
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                std::cout << this->at(i, j) << " ";
            }
            std::cout << "\n";
        }
    }

    // O(1)
    Tensor2D transpose() const {
        Tensor2D view = *this;
        std::swap(view.rows, view.cols);
        view.is_transposed = !this->is_transposed;
        return view;
    }

    Tensor2D matmul(const Tensor2D& other) const {
        if (this->cols != other.rows) {
            throw std::invalid_argument("Matrix dimensions do not match for multiplication.");
        }
        Tensor2D result(this->rows, other.cols, 0.0f);

        if (!this->is_transposed && !other.is_transposed) {
            // std::cout << "--- MULTI CASE 1 ---\n";
#pragma omp parallel for if (this->rows * other.cols > 64) schedule(static)
            for (size_t i = 0; i < this->rows; ++i) {
                size_t i_times_cols     = i * this->cols;
                size_t i_times_res_cols = i * result.cols;

                for (size_t k = 0; k < this->cols; ++k) {
                    float  a_val              = this->data[i_times_cols + k];
                    size_t k_times_other_cols = k * other.cols;

                    //  SIMD
                    for (size_t j = 0; j < other.cols; ++j) {
                        result.data[i_times_res_cols + j] += a_val * other.data[k_times_other_cols + j];
                    }
                }
            }
        } else if (!this->is_transposed && other.is_transposed) {
            // std::cout << "--- MULTI CASE 2 ---\n";
#pragma omp parallel for if (this->rows * other.cols > 64) schedule(static)
            for (size_t i = 0; i < this->rows; ++i) {
                size_t i_times_cols     = i * this->cols;
                size_t i_times_res_cols = i * result.cols;

                for (size_t j = 0; j < other.cols; ++j) {
                    float  sum                = 0.0f;
                    size_t j_times_other_rows = j * other.rows;

                    for (size_t k = 0; k < this->cols; ++k) {
                        sum += this->data[i_times_cols + k] * other.data[j_times_other_rows + k];
                    }
                    result.data[i_times_res_cols + j] = sum;
                }
            }
        } else {
            // std::cout << "--- MULTI CASE 3 ---\n";
            for (size_t i = 0; i < this->rows; ++i) {
                for (size_t k = 0; k < this->cols; ++k) {
                    float a_val = (*this)(i, k);
                    for (size_t j = 0; j < other.cols; ++j) {
                        result(i, j) += a_val * other(k, j);
                    }
                }
            }
        }

        return result;
    }

    Tensor2D relu() const {
        Tensor2D result(this->rows, this->cols, 0.0f);
        size_t   size = data.size();
        for (size_t i = 0; i < size; ++i) {
            result.data[i] = (data[i] > 0.0f) ? data[i] : 0.0f;
        }
        return result;
    }

    void relu_inplace() {
        size_t size = data.size();
        for (size_t i = 0; i < size; ++i) {
            if (data[i] < 0.0f) data[i] = 0.0f;
        }
    }

    Tensor2D relu_backward(const Tensor2D& incoming_gradient) const {
        if (this->rows != incoming_gradient.rows || this->cols != incoming_gradient.cols) {
            throw std::invalid_argument("Dimensions must match for relu backward");
        }

        Tensor2D gradient(this->rows, this->cols, 0.0f);
        size_t   size = data.size();
        for (size_t i = 0; i < size; ++i) {
            gradient.data[i] = (data[i] > 0.0f) ? incoming_gradient.data[i] : 0.0f;
        }

        return gradient;
    }

    // Sigmoid -> SIMD
    Tensor2D sigmoid() const {
        Tensor2D result(rows, cols);
        size_t   size = data.size();
        for (size_t i = 0; i < size; ++i) {
            result.data[i] = 1.0f / (1.0f + std::exp(-data[i]));
        }

        return result;
    }

    void sigmoid_inplace() {
        size_t size = data.size();
        for (size_t i = 0; i < size; ++i) {
            data[i] = 1.0f / (1.0f + std::exp(-data[i]));
        }
    }

    Tensor2D sigmoid_backward(const Tensor2D& incoming_gradient) const {
        if (this->rows != incoming_gradient.rows || this->cols != incoming_gradient.cols) {
            throw std::invalid_argument("Dimensions must match for sigmoid backward");
        }
        Tensor2D d_input(rows, cols);
        size_t   size = data.size();
        for (size_t i = 0; i < size; ++i) {
            float sig       = 1.0f / (1.0f + std::exp(-data[i]));
            d_input.data[i] = incoming_gradient.data[i] * sig * (1.0f - sig);
        }
        return d_input;
    }

    // Write raw memory buffer directly to a stream
    void write_binary(std::ostream& os) const {
        // Write metadata shapes first
        os.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        os.write(reinterpret_cast<const char*>(&cols), sizeof(cols));

        // Write continuous float data array directly
        // Assuming your data is stored in a contiguous std::vector<float> data
        os.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float));
    }

    // Read raw memory buffer directly from a stream
    void read_binary(std::istream& is) {
        size_t in_rows, in_cols;
        is.read(reinterpret_cast<char*>(&in_rows), sizeof(in_rows));
        is.read(reinterpret_cast<char*>(&in_cols), sizeof(in_cols));

        // Verify matrix dimension alignment before loading
        if (in_rows != this->rows || in_cols != this->cols) {
            throw std::runtime_error("Checkpoint dimension mismatch during load.");
        }

        // Read payload array directly into memory buffer
        is.read(reinterpret_cast<char*>(data.data()), data.size() * sizeof(float));
    }

    void fill_zero() { std::fill(data.begin(), data.end(), 0.0f); }
};
