#include <iostream>
#include <cmath>
#include "../wraper_lib/exprtk_wrapper.h"
#include <gnuplot-iostream.h>

double derivative(const ExprEvaluator &f, double x, double h = 1e-5) {
    return (f.eval(x + h) - f.eval(x - h)) / (2 * h);
}

double newton_solve(const ExprEvaluator &f, double x0, double tol = 1e-6, int max_iter = 100) {
    double x = x0;
    for (int i = 0; i < max_iter; ++i) {
        double fx = f.eval(x);
        double dfx = derivative(f, x);
        if (std::fabs(dfx) < 1e-10) {
            std::cerr << "Derivative too small, stop.\n";
            break;
        }
        double x_next = x - fx / dfx;
        if (std::fabs(x_next - x) < tol) {
            std::cout << "Converged after " << i+1 << " iterations.\n";
            return x_next;
        }
        x = x_next;
    }
    std::cerr << "Not converged after " << max_iter << " iterations.\n";
    return x;
}

void plot_function_with_root(const ExprEvaluator &f, double root,
                             double x_min = -10, double x_max = 10) {
    Gnuplot gp;
    std::vector<std::pair<double, double>> data;
    for (double x = x_min; x <= x_max; x += 0.05)
        data.emplace_back(x, f.eval(x));

    std::vector<std::pair<double, double>> root_point = {{root, f.eval(root)}};

    gp << "set title 'Graph of f(x) and root'\n";
    gp << "set xlabel 'x'\nset ylabel 'f(x)'\n";
    gp << "set grid\n";
    gp << "plot '-' with lines lw 2 lc 'blue' title 'f(x)', "
          "'-' with points pt 7 ps 1.5 lc 'red' title 'Root'\n";
    gp.send1d(data);
    gp.send1d(root_point);
}

int main() {
    std::string expr_str;
    std::cout << "Input the function f(x): ";
    std::getline(std::cin, expr_str);

    ExprEvaluator f(expr_str);
    if (!f.valid()) {
        std::cerr << "ERROR: Can not parse the expression.\n";
        return 1;
    }

    double x0;
    std::cout << "Input init value x0 = ";
    std::cin >> x0;

    double root = newton_solve(f, x0);
    std::cout << "Approximate solution: x = " << root
              << ", f(x) = " << f.eval(root) << "\n";

    plot_function_with_root(f, root);

    return 0;
}
