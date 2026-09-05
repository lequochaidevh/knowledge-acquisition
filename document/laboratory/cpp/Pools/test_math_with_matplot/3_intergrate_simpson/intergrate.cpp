#include <iostream>
#include <vector>
#include <cmath>
#include <gnuplot-iostream.h>
#include "../wraper_lib/exprtk_wrapper.h"

double integrate_trapezoid(const ExprEvaluator &f, double a, double b, int n) {
    double h = (b - a) / n;
    double sum = 0.5 * (f.eval(a) + f.eval(b));
    for (int i = 1; i < n; ++i)
        sum += f.eval(a + i * h);
    return sum * h;
}

double integrate_simpson(const ExprEvaluator &f, double a, double b, int n) {
    if (n % 2 != 0) n++; // have to %2 = 0
    double h = (b - a) / n;
    double sum = f.eval(a) + f.eval(b);
    for (int i = 1; i < n; ++i)
        sum += f.eval(a + i * h) * (i % 2 == 0 ? 2 : 4);
    return sum * h / 3.0;
}

int main() {
    std::string expr;
    std::cout << "Input function f(x): ";
    std::getline(std::cin, expr);
    ExprEvaluator f(expr);
    if (!f.valid()) {
        std::cerr << "ERROR: Can not parse the function.\n";
        return 1;
    }

    double a, b;
    std::cout << "Input bound [a, b]: ";
    std::cin >> a >> b;

    int n = 200;
    double area_trap = integrate_trapezoid(f, a, b, n);
    double area_simp = integrate_simpson(f, a, b, n);

    // PP Hinh  thang
    std::cout << "Integrate Area (Trapezoid): " << area_trap << "\n";
    //
    std::cout << "Integrate Area (Simpson):   " << area_simp << "\n";

    // Draw
    std::vector<std::pair<double,double>> curve;
    for (double x = a; x <= b; x += (b-a)/200.0)
        curve.emplace_back(x, f.eval(x));

    Gnuplot gp;
    gp << "set title 'Integrate of the function f(x)'\n";
    gp << "set style fill transparent solid 0.3\n";
    gp << "plot '-' with lines title 'f(x)', "
          "'-' with filledcurves y=0 title 'Area'\n";
    gp.send1d(curve);
    gp.send1d(curve);

    return 0;
}
