#include <iostream>
#include <vector>
#include <gnuplot-iostream.h>
#include "exprtk_wrapper.h"

double compute_derivative(std::function<double(double)> f, double x, double h = 1e-5) {
    return (f(x + h) - f(x - h)) / (2 * h);
}

int main() {
    std::string expr_str;
    std::cout << "Input funtion f(x): ";
    std::getline(std::cin, expr_str);

    ExprEvaluator evaluator(expr_str);
    if (!evaluator.valid()) return 1;

    auto f = [&](double xv) { return evaluator.eval(xv); };

    std::vector<std::pair<double,double>> data_f, data_df;
    for (double xi = -5; xi <= 5; xi += 0.05) {
        data_f.emplace_back(xi, f(xi));
        data_df.emplace_back(xi, compute_derivative(f, xi));
    }

    Gnuplot gp;
    gp << "set title 'f(x) and f`(x)'\n"
          "set grid\n"
          "plot '-' with lines lw 2 title 'f(x)', "
          "'-' with lines lw 2 dashtype 2 title \"f'(x)\"\n";
    gp.send1d(data_f);
    gp.send1d(data_df);
}
