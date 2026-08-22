#pragma once

#include "std17pch.h"

class Tensor2D {
 private:
    size_t             rows;
    size_t             cols;
    std::vector<float> data;

 public:
    // Init r * c
    Tensor2D(size_t r, size_t c, float initial_value = 0.0f) : rows(r), cols(c), data(r * c, initial_value) {}

    // access data
    float& at(size_t r, size_t c) {
        if (r >= rows || c > cols) {
            throw std::out_of_range("Index out of range");
        }
        return data[r * cols + c];
    }

    const float& at(size_t r, size_t c) const {
        if (r >= rows || c >= cols) throw std::out_of_range("Index out of range");
        return data[r * cols + c];
    }

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

    Tensor2D matmul(const Tensor2D& other) const {
        if (this->cols != other.rows) {
            throw std::invalid_argument("Matrix dimensions do not match for multiplication.");
        }
        Tensor2D result(this->rows, other.cols, 0.0f);

        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < other.cols; ++j) {
                float sum = 0.0f;
                for (size_t k = 0; k < this->cols; ++k) {
                    sum += this->data[i * this->cols + k] * other.data[k * other.cols + j];
                }
                result.data[i * result.cols + j] = sum;
            }
        }

        return result;
    }

    Tensor2D relu() const {
        Tensor2D result(this->rows, this->cols, 0.0f);
        for (size_t i = 0; i < this->data.size(); ++i) {
            result.data[i] = (this->data[i] > 0.0f) ? this->data[i] : 0.0f;
        }
        return result;
    }

    // ReLU derivative: f'(x) = 1 if x > 0, otherwise 0
    // Receives the gradient matrix from the subsequent layer and filters it based on the current data
    Tensor2D relu_backward(const Tensor2D& incoming_gradient) const {
        if (this->rows != incoming_gradient.rows || this->cols != incoming_gradient.cols) {
            throw std::invalid_argument("Dimentions must match for relu backward");
        }

        Tensor2D gradient(this->rows, this->cols, 0.0f);
        for (size_t i = 0; i < this->data.size(); ++i) {
            gradient.data[i] = (this->data[i] > 0.0f) ? incoming_gradient.data[i] : 0.0f;
        }

        return gradient;
    }

    // Transpose the matrix: rows become columns and vice versa
    Tensor2D transpose() const {
        Tensor2D result(this->cols, this->rows, 0.0f);
        for (size_t i = 0; i < this->rows; ++i) {
            for (size_t j = 0; j < this->cols; ++j) {
                result.data[j * result.cols + i] = this->data[i * this->cols + j];
            }
        }
        return result;
    }

    // Forward pass: Apply sigmoid element-wise
    Tensor2D sigmoid() const {
        Tensor2D result(rows, cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                float x         = this->at(i, j);
                result.at(i, j) = 1.0f / (1.0f + std::exp(-x));
            }
        }
        return result;
    }

    // Backward pass: Compute gradient through sigmoid layer
    Tensor2D sigmoid_backward(const Tensor2D& incoming_gradient) const {
        Tensor2D d_input(rows, cols);
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                float x   = this->at(i, j);
                float sig = 1.0f / (1.0f + std::exp(-x));

                // local_gradient = sig * (1.0f - sig)
                d_input.at(i, j) = incoming_gradient.at(i, j) * sig * (1.0f - sig);
            }
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
};
