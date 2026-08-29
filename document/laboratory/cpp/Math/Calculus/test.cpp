// g++ test.cpp -std=gnu++17
#include "calculus.h"

int Finite_Difference_Method();
int house_pricing();
int main() {
    // Finite_Difference_Method();
    house_pricing();
    return 0;
}

int Finite_Difference_Method() {
    // Define a target mathematical function: f(x) = x^2 + 3x
    // Its exact analytical derivative is: f'(x) = 2x + 3
    auto my_function = [](double x) constexpr { return (x * x) + (3.0 * x); };

    // Let's compute the derivative at x = 5.0
    // Exact analytical answer: 2*(5.0) + 3 = 13.0
    constexpr double x_point = 5.0;
    constexpr double df      = derivative(my_function, x_point);

    std::cout << "Function: f(x) = x^2 + 3x\n";
    std::cout << "Compile-time Derivative at x = " << x_point << " is: " << df << "\n";

    return 0;
}

int house_pricing() {
    // Define a 2-variable mathematical function (House Pricing Model):
    // f(x1, x2) = 0.1 * x1^2 + 5.0 * x2
    // Analytical Gradient formula: [0.2 * x1, 5.0]
    auto house_pricing_model = [](const std::array<double, 2>& x) constexpr {
        double x1 = x[0];  // House size (m2)
        double x2 = x[1];  // Number of bedrooms
        return (0.1 * x1 * x1) + (5.0 * x2);
    };

    // Evaluate the gradient at point x = (100.0 m2, 3 bedrooms)
    // Analytical answer should be: [0.2 * 100, 5.0] -> [20.0, 5.0]
    constexpr std::array<double, 2> point{100.0, 3.0};

    // The entire multi-variable gradient is computed by the compiler!
    constexpr std::array<double, 2> grad = compute_gradient<2>(house_pricing_model, point);

    // Print out compile-time calculated results
    std::cout << "--- House Pricing Model Gradient at (100m2, 3 beds) ---\n";
    std::cout << "Partial derivative w.r.t Size (x1)     : " << grad[0] << "\n";
    std::cout << "Partial derivative w.r.t Bedrooms (x2) : " << grad[1] << "\n";

    return 0;
}