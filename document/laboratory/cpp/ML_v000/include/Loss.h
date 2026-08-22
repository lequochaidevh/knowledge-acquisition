#pragma once

#include "Tensor2D.h"

class MSELoss {
public:
	// Calc averg Loss: 1/N * sum((pred - target)^2)
	float forward(const Tensor2D& pred, const Tensor2D& target) {
		if(pred.get_rows() != target.get_rows() || pred.get_cols() != target.get_cols()) {
			throw std::invalid_argument("Dimentions do not match in Loss forward.");
		}

		float total_loss = 0.0f;
		size_t rows = pred.get_rows();
		size_t cols = pred.get_cols();
		size_t n = rows * cols;

		for(size_t i=0; i<rows; ++i) {
			for(size_t j=0; j<cols; ++j) {
				float diff = pred.at(i, j) - target.at(i, j);
				total_loss += diff * diff;
			}
		}
		return total_loss / n;
	}

	// Derivative of the MSE Loss: 2/N * (pred - target) 
	// in order to back propagation
	Tensor2D backward(const Tensor2D& pred, const Tensor2D& target) {
		size_t rows = pred.get_rows();
		size_t cols = pred.get_cols();
		size_t n = rows * cols;

		Tensor2D gradient(rows, cols, 0.0f);
		for(size_t i=0; i<rows; ++i) {
			for(size_t j=0; j<cols; ++j) {
			       gradient.at(i, j) = (2.0f / n) * (pred.at(i, j) - target.at(i, j));
			}
	 	}
		return gradient;

	}		

};
