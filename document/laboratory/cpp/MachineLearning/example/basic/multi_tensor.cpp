#include "../include/Tensor2D.h"

int main() {
	Tensor2D A(2, 3);
	A.at(0, 0) = 1;
	A.at(0, 1) = 2;
	A.at(0, 2) = 3;
	A.at(1, 0) = 4;
	A.at(1, 1) = 5;
	A.at(1, 2) = 6;

	Tensor2D B(3, 2);
	B.at(0, 0) = 7;
	B.at(0, 1) = 8;
	B.at(1, 0) = 9;
	B.at(1, 1) = 10;
	B.at(2, 0) = 11;
	B.at(2, 1) = 12;

	std::cout << "\nMatrix A:\n";
	A.print();
	std::cout << "\nMatrix B:\n";
	B.print();

	try {
		Tensor2D C = A.matmul(B);
		std::cout << "\nResult: C = A * B\n";

		C.print();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
	}

	return 0;
}



