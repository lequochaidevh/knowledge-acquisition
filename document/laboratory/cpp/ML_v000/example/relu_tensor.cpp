#include "../include/Tensor2D.h"

int main() {
	Tensor2D A(2, 2);
	A.at(0, 0) = -2.5f;
	A.at(0, 1) = 3.0f;
	A.at(1, 0) = -1.0f;
	A.at(1, 1) = -5.5f;
	
	std::cout << "Matrix A:\n";
	A.print();

	// Relu
	Tensor2D B = A.relu();
	
	std::cout << "ReluA = Matrix B:\n";
	B.print();

	return 0;
}
