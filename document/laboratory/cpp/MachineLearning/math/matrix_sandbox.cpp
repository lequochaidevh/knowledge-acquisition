// g++ matrix_sandbox.cpp -std=gnu++17
#include "matrix.h"

int main() {
    // 3x3 Matrix instance initialized at compile time
    constexpr Matrix<3> A({1.0, 2.0, 3.0, 0.0, 1.0, 4.0, 5.0, 6.0, 0.0});

    // The entire calculation happens during compilation! Zero CPU cost at runtime.
    constexpr double det2 = determinant(A);

    std::cout << "Matrix Size: " << 3 << "x" << 3 << "\n";
    std::cout << "Compile-time Determinant: " << det2 << "\n";

    // 2. Transpose transformation (evaluated at compile time!)
    constexpr Matrix<3> A_T = A.transpose();

    // 3. Functional transformation (e.g., doubling every element)
    // C++17 allows lambdas inside constexpr contexts
    constexpr Matrix<3> A_doubled = A.transform([](double val) { return val * 2.0; });

    // Verify Transpose Output
    std::cout << "Top right element of A (row 0, col 2): " << A(0, 2) << "\n";        // 3.0
    std::cout << "Bottom left element of A_T (row 2, col 0): " << A_T(2, 0) << "\n";  // 3.0

    // Initialize Matrix B at compile time
    constexpr Matrix<3> B({2.0, 0.0, -2.0, 2.0, 0.0, -1.0, 1.0, 2.0, 1.0});

    // The multiplication is fully evaluated by the compiler!
    constexpr Matrix<3> C = multiply(A, B);

    // Print out the compile-time calculated result
    std::cout << "Result Matrix C (A * B):\n";
    C.print();

    // 1. Short-hand notation works perfectly! (Evaluates to Matrix<2, 2>)
    constexpr Matrix<2> SquareA{1.0, 2.0, 3.0, 4.0};
    constexpr Matrix<2> SquareB{2.0, 0.0, 1.0, 2.0};

    // Calls the specialized square overload cleanly
    constexpr Matrix<2> SquareC = multiply(SquareA, SquareB);
    constexpr double    detA    = determinant(SquareA);

    std::cout << "Square Matrix (via Matrix<2>) Det: " << detA << "\n";

    // 2. Rectangular notation still works seamlessly!
    constexpr Matrix<3, 2> RectA{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};

    constexpr Matrix<3, 2> RectResult = multiply(RectA, SquareB);
    std::cout << "Rect Matrix Result (0,0): " << RectResult(0, 0) << "\n";

    // Calculate the dominant eigenvalue and eigenvector at runtime
    auto resultEigenv = power_iteration(A);

    std::cout << "--- 3x3 Matrix Eigen-Result ---\n";
    std::cout << "Dominant Eigenvalue: " << resultEigenv.eigenvalue << "\n";
    std::cout << "Dominant Eigenvector: [ ";
    for (double val : resultEigenv.eigenvector) {
        std::cout << val << " ";
    }
    std::cout << "]\n";
    return 0;
}
