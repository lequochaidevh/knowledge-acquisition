#include <iostream>
#include <vector>
#include <cmath>
#include <functional>
#include <gnuplot-iostream.h>
#include "../exprtk.hpp"

double compute_derivative(std::function<double(double)> f, double x, double h = 1e-5) {
    return (f(x + h) - f(x - h)) / (2 * h);
}

int main() {
    std::string expr_str;
    std::cout << "Nhap ham f(x) (vi du: sin(x) + x*x - 3): ";
    std::getline(std::cin, expr_str);

    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double> expression_t;
    typedef exprtk::parser<double> parser_t;

    double x;
    symbol_table_t symbol_table;
    symbol_table.add_variable("x", x);
    symbol_table.add_constants();

    expression_t expression;
    expression.register_symbol_table(symbol_table);

    parser_t parser;
    if (!parser.compile(expr_str, expression)) {
        std::cerr << "Loi: Khong the phan tich bieu thuc!\n";
        return 1;
    }

    std::function<double(double)> f = [&](double xv) {
        x = xv;
        return expression.value();
    };

    std::vector<std::pair<double, double>> data_f, data_df;
    for (double xi = -5; xi <= 5; xi += 0.05) {
        data_f.emplace_back(xi, f(xi));
        data_df.emplace_back(xi, compute_derivative(f, xi));
    }

    Gnuplot gp;
    gp << "set title 'Function Graph f(x) and f`(x)'\n";
    gp << "set xlabel 'x'\n";
    gp << "set ylabel 'Value'\n";
    gp << "plot '-' with lines title 'f(x)', '-' with lines title \"f'(x)\"\n";
    gp.send1d(data_f);
    gp.send1d(data_df);

    return 0;
}
