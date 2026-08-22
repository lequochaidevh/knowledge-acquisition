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
};
