#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <gnuplot-iostream.h>
#include "../wraper_lib/exprtk_wrapper.h"

double derivative(ExprEvaluator &f, double x, double h = 1e-5)
{
    return (f.eval(x + h) - f.eval(x - h)) / (2 * h);
}

double second_derivative(ExprEvaluator &f, double x, double h = 1e-5) {
    return (f.eval(x + h) - 2*f.eval(x) + f.eval(x - h)) / (h * h);
}

std::vector<std::pair<double, double>> find_extrema(ExprEvaluator &f, double start, double end, double step)
{
    std::vector<std::pair<double, double>> extrema;
    double prev_x = start;
    double prev_d = derivative(f, prev_x);
    for (double x = start + step; x <= end; x += step)
    {
        double d = derivative(f, x);
        if (prev_d * d < 0)
        { // If the derivative changes sign, this is the extreme value.
            double mid = (x + prev_x) / 2.0;
            extrema.emplace_back(mid, f.eval(mid));
        }
        prev_x = x;
        prev_d = d;
    }
    return extrema;
}

std::vector<std::pair<double, double>> find_extrema_new(
        ExprEvaluator &f,
        double a, double b, double step)
{
    std::vector<std::pair<double, double>> extrema;
    double prev = f.eval(a);
    double prev_d = derivative(f, a);

    for (double x = a + step; x <= b; x += step) {
        double curr_d = derivative(f, x);
        if (prev_d * curr_d < 0) {
            double mid = x - step / 2;
            extrema.push_back({mid, f.eval(mid)});
        }
        prev_d = curr_d;
    }

    // Check at x=0; if 0 exist in [a,b]
    if (a <= 0 && b >= 0) {
        extrema.push_back({0.0, f.eval(0.0)});
    }

    // Boundation
    extrema.push_back({a, f.eval(a)});
    extrema.push_back({b, f.eval(b)});

    return extrema;
}

int main()
{
    std::string expr_str;
    std::cout << "Input function f(x): ";
    std::getline(std::cin, expr_str);

    ExprEvaluator f(expr_str);
    if (!f.valid())
    {
        std::cerr << "ERROR: Can not parse the expression.\n";
        return 1;
    }

    double start = -10, end = 10, step = 0.05;
    std::vector<std::pair<double, double>> data;
    for (double x = start; x <= end; x += step)
        data.emplace_back(x, f.eval(x));

    auto extrema = find_extrema_new(f, start, end, step);

    Gnuplot gp;
    gp << "set title 'Function graphs and extreme points'\n";
    gp << "set xlabel 'x'\nset ylabel 'f(x)'\n";
    gp << "set grid\n";
    gp << "plot '-' with lines lw 2 title 'f(x)', "
          "'-' with points pt 7 ps 1.5 lc 'red' title 'extreme value'\n";

    gp.send1d(data);
    gp.send1d(extrema);

    for (auto &p : extrema)
        gp << "set label sprintf('(%.2f, %.2f)', " << p.first << ", " << p.second
           << ") at " << p.first << "," << p.second << " offset 0,1\n";
    gp << "replot\n";


    std::vector<std::tuple<double,double,std::string>> extrema_typed;
    // -------------------------
    for (auto &p : extrema) {
        double d2 = second_derivative(f, p.first);
        std::string type;
        if (d2 > 0) type = "Minimum";
        else if (d2 < 0) type = "Maximum";
        else type = "No decleare";

        extrema_typed.push_back({p.first, p.second, type});
    }
    for (auto &[x, y, type] : extrema_typed) {
        std::cout << type << " at x = " << x << ", f(x) = " << y << "\n";
    }
    for (auto &[x, y, type] : extrema_typed)
    // gp << "set label sprintf('%s(%.2f, %.2f)', '" << type << "', " 
    //     << x << ", " << y << ") at " << x << "," << y << " offset 0,1\n";
    // gp << "replot\n";

    return 0;
}
