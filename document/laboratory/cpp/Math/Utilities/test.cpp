// g++ test.cpp -std=c++17 && ./a.out && rm a.out
#include "compile_time_math.h"

int main() {
    // -------------------------------------------------------------------------
    // COMPILE-TIME CHECKS (Silent if successful)
    // -------------------------------------------------------------------------
    constexpr CompileTimeMath::Fraction f1(1, 2);
    constexpr CompileTimeMath::Fraction f2(1, 3);
    constexpr auto                      res_add = f1.add(f2);
    static_assert(res_add.num == 5 && res_add.den == 6);

    constexpr CompileTimeMath::Point2D<double> p1{0.0, 0.0};
    constexpr CompileTimeMath::Point2D<double> p2{3.0, 4.0};
    static_assert(p1.distance_to(p2) == 5.0);

    constexpr CompileTimeMath::LinearFunction<double> line1{2.0, 5.0};
    constexpr CompileTimeMath::LinearFunction<double> line2{2.0, -1.0};
    constexpr CompileTimeMath::LinearFunction<double> line3{-0.5, 3.0};
    static_assert(line1.evaluate(2.0) == 9.0);
    static_assert(line1.is_parallel_to(line2));
    static_assert(line1.is_perpendicular_to(line3));

    constexpr CompileTimeMath::QuadraticFunction<double> quad{1.0};
    constexpr auto                                       eq_roots = quad.solve_with(-5.0, 6.0);
    static_assert(eq_roots.has_real_roots && eq_roots.root_count == 2);
    static_assert(eq_roots.x1 == 2.0 && eq_roots.x2 == 3.0);

    // -------------------------------------------------------------------------
    // RUNTIME OUTPUTS (This will show on your terminal screen)
    // -------------------------------------------------------------------------
    std::cout << "=== VN MATH LIBRARY RUNTIME OUTPUT ===\n\n";

    // Grade 6: Fraction Output
    std::cout << "[Grade 6] Fraction Addition: 1/2 + 1/3 = " << res_add.num << "/" << res_add.den << "\n";

    // Grade 7: Coordinate Distance Output
    std::cout << "[Grade 7] Distance from (0,0) to (3,4) = " << p1.distance_to(p2) << "\n";

    // Grade 8: The 7 Remarkable Identities
    using AlgId = CompileTimeMath::Identities<long long>;

    // Setup typical evaluation values for algebra tests
    constexpr long long a = 5;
    constexpr long long b = 3;

    // Validate identity structures cleanly at compile-time
    static_assert(AlgId::verify_square_of_sum(a, b));    // (5+3)^2 == 25 + 30 + 9
    static_assert(AlgId::verify_square_of_diff(a, b));   // (5-3)^2 == 25 - 30 + 9
    static_assert(AlgId::verify_diff_of_squares(a, b));  // 25 - 9  == (5-3)*(5+3)
    static_assert(AlgId::verify_cube_of_sum(a, b));      // (5+3)^3 == 125 + 225 + 135 + 27
    static_assert(AlgId::verify_cube_of_diff(a, b));     // (5-3)^3 == 125 - 225 + 135 - 27
    static_assert(AlgId::verify_sum_of_cubes(a, b));     // 125 + 27 == (5+3)*(25 - 15 + 9)
    static_assert(AlgId::verify_diff_of_cubes(a, b));    // 125 - 27 == (5-3)*(25 + 15 + 9)

    // Runtime Confirmation Output
    std::cout << "All 7 identities successfully validated at compile-time!\n";
    std::cout << "Example (a=5, b=3):\n";
    std::cout << "[Grade 8]1. Square of Sum expanded form: " << AlgId::square_of_sum_expanded(a, b) << "\n";
    std::cout << "[Grade 8]3. Difference of Squares value: " << AlgId::diff_of_squares_expanded(a, b) << "\n";
    std::cout << "[Grade 8]6. Sum of Cubes factored check: " << std::boolalpha << AlgId::verify_sum_of_cubes(a, b)
              << "\n";
    std::cout << "\n";

    // Grade 9: Linear Evaluation Output
    std::cout << "[Grade 9] Line y = 2x + 5 evaluated at x=2 -> y = " << line1.evaluate(2.0) << "\n";
    std::cout << "[Grade 9] Is line1 parallel to line2? " << std::boolalpha << line1.is_parallel_to(line2) << "\n";

    // Grade 9: Quadratic Solver Output
    std::cout << "[Grade 9] Solving x^2 - 5x + 6 = 0...\n";
    std::cout << "          Roots found: x1 = " << eq_roots.x1 << ", x2 = " << eq_roots.x2 << "\n";

    // Test Cubic Equation: x^3 - 6x^2 + 11x - 6 = 0 (Roots: 1, 2, 3 -> Multi-root variant)
    // To test the 1-root Cardano path easily in compile-time: x^3 + 3x + 4 = 0
    constexpr CompileTimeMath::CubicFunction<double> cubic{1.0, 0.0, 3.0, 4.0};
    constexpr auto                                   cub_root = cubic.solve();
    static_assert(cub_root.root_count == 1);

    // Test Bi-Quadratic Equation: x^4 - 5x^2 + 4 = 0 -> t^2 - 5t + 4 = 0 -> t=1, t=4 -> x = -2, -1, 1, 2
    constexpr CompileTimeMath::BiQuadraticFunction<double> quartic{1.0, -5.0, 4.0};
    constexpr auto                                         quart_roots = quartic.solve();
    static_assert(quart_roots.root_count == 4);
    static_assert(quart_roots.roots[3] == 2.0);  // Verifying upper bound root

    // Output results to Terminal Screen
    std::cout << "=== HIGH SCHOOL HIGHER-DEGREE ALGEBRA ===\n\n";
    std::cout << "[Cubic] x^3 + 3x + 4 = 0 has real root at: " << cub_root.x1 << "\n";
    std::cout << "[Quartic] x^4 - 5x^2 + 4 = 0 roots count: " << quart_roots.root_count << "\n";
    std::cout << "          Roots: " << quart_roots.roots[0] << ", " << quart_roots.roots[1] << ", "
              << quart_roots.roots[2] << ", " << quart_roots.roots[3] << "\n";
    return 0;
}