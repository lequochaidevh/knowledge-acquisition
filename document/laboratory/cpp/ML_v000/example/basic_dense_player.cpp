#include "../include/DenseLayer.h"

int main() {
	Tensor2D input(2, 3);
	input.at(0, 0) = 1.0f;
	input.at(0, 1) = 2.0f;
	input.at(0, 2) = 3.0f;
	input.at(1, 0) = 0.5f;
	input.at(1, 1) = -1.0f;
	input.at(1, 2) = 1.5f;

	DenseLayer layer(3, 2);

	Tensor2D output = layer.forward(input);

	std::cout << "Result:\n";

	output.print();

	return 0;

}
