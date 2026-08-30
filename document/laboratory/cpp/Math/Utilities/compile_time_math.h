#include "../../MachineLearning/include/std17pch.h"

namespace CompileTimeMath {
// CORE UTILITIES & WRAPPER
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
[[nodiscard]] constexpr T abs(T value) noexcept {
    return value < 0 ? -value : value;
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
[[nodiscard]] constexpr T gcd(T a, T b) noexcept {
    return std::gcd(a, b);
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
[[nodiscard]] constexpr T lcm(T a, T b) noexcept {
    return std::lcm(a, b);
}

// Compile-time square root via Babylonian (Newton-Raphson) method for C++17
[[nodiscard]] constexpr double sqrt(double value) noexcept {
    if (value < 0.0) return std::numeric_limits<double>::quiet_NaN();
    if (value == 0.0 || value == 1.0) return value;

    double current  = value;
    double previous = 0.0;
    // Iterate until convergence within double precision limits
    while (CompileTimeMath::abs(current - previous) > 1e-12) {
        previous = current;
        current  = 0.5 * (current + (value / current));
    }
    return current;
}

// Compile-time Cube Root (Căn bậc 3) via Newton-Raphson Method
[[nodiscard]] constexpr double cbrt(double value) noexcept {
    if (value == 0.0) return 0.0;
    double current  = value > 0.0 ? value : -value;
    double previous = 0.0;
    // Newton's iteration formula for cube root: x_next = (2 * x + val / (x * x)) / 3
    while (CompileTimeMath::abs(current - previous) > 1e-12) {
        previous = current;
        current  = (2.0 * current + (CompileTimeMath::abs(value) / (current * current))) / 3.0;
    }
    return value < 0.0 ? -current : current;
}

// GRADE 6: Fraction Arithmetic
class Fraction {
 public:
    long long num;
    long long den;

    constexpr Fraction() noexcept : num(0), den(1) {}

    constexpr Fraction(long long n, long long d) noexcept : num(n), den(d) {
        if (den == 0) {
            num = 0;
            den = 1;
        } else {
            long long common_devisor = std::gcd(num, den);
            num /= common_devisor;
            den /= common_devisor;
            if (den < 0) {
                num = -num;
                den = -den;
            }
        }
    }

    [[nodiscard]] constexpr Fraction add(const Fraction& other) const noexcept {
        return Fraction(num * other.den + other.num * den, den * other.den);
    }

    [[nodiscard]] constexpr Fraction substract(const Fraction& other) const noexcept {
        return Fraction(num * other.den - other.num * den, den * other.den);
    }

    [[nodiscard]] constexpr Fraction multiply(const Fraction& other) const noexcept {
        return Fraction(num * other.num, den * other.den);
    }

    [[nodiscard]] constexpr Fraction devide(const Fraction& other) const noexcept {
        return Fraction(num * other.den, den * other.num);
    }

    [[nodiscard]] constexpr double to_double() const noexcept {
        return static_cast<double>(num) / static_cast<double>(den);
    }
};

// Cartesian Coordinate (Oxy)
template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
class Point2D {
 public:
    T x;
    T y;

    constexpr Point2D() noexcept : x(0), y(0) {}

    constexpr Point2D(T x_, T y_) noexcept : x(x_), y(y_) {}

    // constexpr Point2D(std::initializer_list<double> init_list)

    [[nodiscard]] constexpr double distance_to(const Point2D& other) const noexcept {
        double dx = static_cast<double>(x) - static_cast<double>(other.x);
        double dy = static_cast<double>(y) - static_cast<double>(other.y);

        return CompileTimeMath::sqrt(dx * dx + dy * dy);
    }
};

template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class LinearFunction {
 public:
    T a;  // slope coefficient
    T b;  // Y-intercept

    constexpr LinearFunction(T slope, T intercept) noexcept : a(slope), b(intercept) {}

    // evalute y for a given x
    [[nodiscard]] constexpr T evaluate(T x) const noexcept { return a * x + b; }

    // find x-intercept (at  y = 0)
    [[nodiscard]] constexpr T find_root() const noexcept {
        if (a == 0) return std::numeric_limits<T>::quiet_NaN();
        return -b / a;
    }

    // Check if  parallel to another line (d // d')
    [[nodiscard]] constexpr bool is_parallel_to(const LinearFunction& other) const noexcept {
        return (a == other.a) && (b != other.b);
    }

    // L
    [[nodiscard]] constexpr bool is_perpendicular_to(const LinearFunction& other) const noexcept {
        return (a * other.a) == static_cast<T>(-1.0);
    }
};

// GRADE 8: THE 7 REMARKABLE IDENTITIES
// =========================================================================
template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
struct Identities {
    // --- Identity 1: Square of a Sum ---
    // (a + b)^2 = a^2 + 2ab + b^2
    [[nodiscard]] static constexpr T    square_of_sum_expanded(T a, T b) noexcept { return a * a + 2 * a * b + b * b; }
    [[nodiscard]] static constexpr bool verify_square_of_sum(T a, T b) noexcept {
        T factored = (a + b) * (a + b);
        return factored == square_of_sum_expanded(a, b);
    }

    // --- Identity 2: Square of a Difference ---
    // (a - b)^2 = a^2 - 2ab + b^2
    [[nodiscard]] static constexpr T    square_of_diff_expanded(T a, T b) noexcept { return a * a - 2 * a * b + b * b; }
    [[nodiscard]] static constexpr bool verify_square_of_diff(T a, T b) noexcept {
        T factored = (a - b) * (a - b);
        return factored == square_of_diff_expanded(a, b);
    }

    // --- Identity 3: Difference of Two Squares ---
    // a^2 - b^2 = (a - b)(a + b)
    [[nodiscard]] static constexpr T    diff_of_squares_expanded(T a, T b) noexcept { return a * a - b * b; }
    [[nodiscard]] static constexpr bool verify_diff_of_squares(T a, T b) noexcept {
        T factored = (a - b) * (a + b);
        return factored == diff_of_squares_expanded(a, b);
    }

    // --- Identity 4: Cube of a Sum ---
    // (a + b)^3 = a^3 + 3a^2b + 3ab^2 + b^3
    [[nodiscard]] static constexpr T cube_of_sum_expanded(T a, T b) noexcept {
        return a * a * a + 3 * a * a * b + 3 * a * b * b + b * b * b;
    }
    [[nodiscard]] static constexpr bool verify_cube_of_sum(T a, T b) noexcept {
        T factored = (a + b) * (a + b) * (a + b);
        return factored == cube_of_sum_expanded(a, b);
    }

    // --- Identity 5: Cube of a Difference ---
    // (a - b)^3 = a^3 - 3a^2b + 3ab^2 - b^3
    [[nodiscard]] static constexpr T cube_of_diff_expanded(T a, T b) noexcept {
        return a * a * a - 3 * a * a * b + 3 * a * b * b - b * b * b;
    }
    [[nodiscard]] static constexpr bool verify_cube_of_diff(T a, T b) noexcept {
        T factored = (a - b) * (a - b) * (a - b);
        return factored == cube_of_diff_expanded(a, b);
    }

    // --- Identity 6: Sum of Two Cubes ---
    // a^3 + b^3 = (a + b)(a^2 - ab + b^2)
    [[nodiscard]] static constexpr T    sum_of_cubes_expanded(T a, T b) noexcept { return a * a * a + b * b * b; }
    [[nodiscard]] static constexpr bool verify_sum_of_cubes(T a, T b) noexcept {
        T factored = (a + b) * (a * a - a * b + b * b);
        return factored == sum_of_cubes_expanded(a, b);
    }

    // --- Identity 7: Difference of Two Cubes ---
    // a^3 - b^3 = (a - b)(a^2 + ab + b^2)
    [[nodiscard]] static constexpr T    diff_of_cubes_expanded(T a, T b) noexcept { return a * a * a - b * b * b; }
    [[nodiscard]] static constexpr bool verify_diff_of_cubes(T a, T b) noexcept {
        T factored = (a - b) * (a * a + a * b + b * b);
        return factored == diff_of_cubes_expanded(a, b);
    }
};

// y = ax^2
template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
class QuadraticFunction {
 public:
    T a;  // Must be non-zero for quadratic behavior

    constexpr QuadraticFunction(T coef_a) noexcept : a(coef_a) {}

    // evalute y for given x
    [[nodiscard]] constexpr T evalute(T x) const noexcept { return a * x * x; }

    struct EquationResult {
        bool has_real_roots;
        int  root_count;
        T    x1;
        T    x2;
    };

    // Solve complete quadratic equation ax^2 + bx +c = 0
    [[nodiscard]] constexpr EquationResult solve_with(T b, T c) const noexcept {
        if (a == 0.0) return {false, 0, 0, 0};  // Guard against degradation to linear

        T delta = b * b - 4 * a * c;
        if (delta < 0) {
            return {false, 0, 0, 0};
        } else if (delta == 0) {
            T root = -b / (2 * a);
            return {true, 1, root, root};
        } else {
            T sqrt_delta = static_cast<T>(CompileTimeMath::sqrt(static_cast<double>(delta)));
            T root1      = (-b - sqrt_delta) / (2 * a);
            T root2      = (-b + sqrt_delta) / (2 * a);
            return {true, 2, root1, root2};
        }
    }
};

template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
class CubicFunction {
 public:
    T a, b, c, d;

    constexpr CubicFunction(T coef_a, T coef_b, T coef_c, T coef_d) noexcept
        : a(coef_a), b(coef_b), c(coef_c), d(coef_d) {}

    [[nodiscard]] constexpr T evaluate(T x) const noexcept { return a * x * x * x + b * x * x + c * x + d; }

    struct CubicResult {
        int root_count;
        T   x1;
        T   x2;
        T   x3;
    };

    // Solve ax^3 + bx^2 + cx + d = 0 using Cardano's analytical method (Real roots only)
    [[nodiscard]] constexpr CubicResult solve() const {
        if (a == 0.0) throw std::invalid_argument("Coefficient 'a' cannot be zero for cubic equation!");

        // Convert to depressed cubic form: t^3 + p*t + q = 0 where x = t - b/(3a)
        double p            = (3.0 * a * c - b * b) / (3.0 * a * a);
        double q            = (2.0 * b * b * b - 9.0 * a * b * c + 27.0 * a * a * d) / (27.0 * a * a * a);
        double discriminant = (q * q / 4.0) + (p * p * p / 27.0);

        T shift = -b / (3.0 * a);

        if (discriminant > 0.0) {
            // One real root case
            double sqrt_disc = CompileTimeMath::sqrt(discriminant);
            double u         = CompileTimeMath::cbrt(-q / 2.0 + sqrt_disc);
            double v         = CompileTimeMath::cbrt(-q / 2.0 - sqrt_disc);
            return {1, static_cast<T>(u + v + shift), 0.0, 0.0};
        } else if (discriminant == 0.0) {
            // Multiple real roots
            if (p == 0.0 && q == 0.0) {
                return {3, shift, shift, shift};  // Triple root
            }
            double u = CompileTimeMath::cbrt(-q / 2.0);
            return {2, static_cast<T>(2.0 * u + shift), static_cast<T>(-u + shift), 0.0};
        } else {
            // Three distinct real roots case (Trigonometric solution required for constexpr real domain)
            // Using nested loops approximation fallback for strict pure-algebraic constexpr without std::acos
            // Let's solve a clean sample case via Cardano-friendly structures or precise factoring.
            // For simplicity of basic C++17 math, we signal a known pattern or handle Cardano branches.
            throw std::domain_error("Three distinct real roots require trigonometric evaluation!");
        }
    }
};

// 3. HIGH SCHOOL: QUARTIC BI-QUADRATIC ( ax^4 + bx^2 + c = 0)
// =========================================================================
template <typename T, typename = std::enable_if_t<std::is_floating_point<T>::value>>
class BiQuadraticFunction {
 public:
    T a, b, c;

    constexpr BiQuadraticFunction(T coef_a, T coef_b, T coef_c) noexcept : a(coef_a), b(coef_b), c(coef_c) {}

    [[nodiscard]] constexpr T evaluate(T x) const noexcept { return a * x * x * x * x + b * x * x + c; }

    struct QuarticResult {
        int root_count;
        T   roots[4];
    };

    // Solve ax^4 + bx^2 + c = 0 by substituting t = x^2 (t >= 0)
    [[nodiscard]] constexpr QuarticResult solve() const {
        if (a == 0.0) throw std::invalid_argument("Coefficient 'a' cannot be zero!");

        // Step 1: Solve auxiliary equation a*t^2 + b*t + c = 0
        double delta = b * b - 4.0 * a * c;
        if (delta < 0.0) return {0, {0.0, 0.0, 0.0, 0.0}};

        double t1 = (-b - CompileTimeMath::sqrt(delta)) / (2.0 * a);
        double t2 = (-b + CompileTimeMath::sqrt(delta)) / (2.0 * a);

        QuarticResult result{0, {0.0, 0.0, 0.0, 0.0}};

        // Step 2: Extract x = +/- sqrt(t) for valid t >= 0
        if (t1 >= 0.0) {
            double r1                         = CompileTimeMath::sqrt(t1);
            result.roots[result.root_count++] = static_cast<T>(-r1);
            result.roots[result.root_count++] = static_cast<T>(r1);
        }
        if (t2 >= 0.0 && CompileTimeMath::abs(t1 - t2) > 1e-9) {  // Avoid duplicate if t1 == t2
            double r2                         = CompileTimeMath::sqrt(t2);
            result.roots[result.root_count++] = static_cast<T>(-r2);
            result.roots[result.root_count++] = static_cast<T>(r2);
        }

        return result;
    }
};

namespace geometry {
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    struct Rectangle {
        T width;
        T height;

        [[nodiscard]] constexpr T perimeter() const noexcept { return 2 * (width + height); }

        [[nodiscard]] constexpr T area() const noexcept { return width * height; }
    };

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
    struct Triangle {
        T base;
        T height;

        [[nodiscard]] constexpr T area() const noexcept { return static_cast<T>(0.5) * base * height; }
    };

}  // namespace geometry

};  // namespace CompileTimeMath