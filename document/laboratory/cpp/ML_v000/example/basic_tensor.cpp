#include "../include/Tensor2D.h"

int main() {
    Tensor2D mat(2, 3, 1.5f);

    std::cout << "Init matrix:\n";
    mat.print();

    mat.at(0, 1) = 5.0f;

    std::cout << "Matrix affter changed:\n";
    mat.print();

    return 0;
}
