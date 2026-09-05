#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <gnuplot-iostream.h>
#include "../wraper_lib/exprtk_wrapper.h"

double derivative(std::function<double(double)> f, double x, double h=1e-5) {
    return (f(x+h) - f(x-h)) / (2*h);
}

double second_derivative(std::function<double(double)> f, double x, double h=1e-5) {
    return (f(x+h) - 2*f(x) + f(x-h)) / (h*h);
}

int main() {
    using namespace std;

    string expr_str;
    cout << "Nhap ham f(x) (VD: x^3 - 3*x + 1): ";
    getline(cin, expr_str);

    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double> expression_t;
    typedef exprtk::parser<double> parser_t;

    double x;
    symbol_table_t sym_table;
    sym_table.add_variable("x", x);
    sym_table.add_constants();

    expression_t expr;
    expr.register_symbol_table(sym_table);

    parser_t parser;
    if (!parser.compile(expr_str, expr)) {
        cerr << "Loi: khong the phan tich bieu thuc!\n";
        return 1;
    }

    auto f = [&](double xv) {
        x = xv;
        return expr.value();
    };

    // Tính dữ liệu đồ thị
    vector<pair<double,double>> data_f;
    for (double xi=-5; xi<=5; xi+=0.05)
        data_f.emplace_back(xi, f(xi));

    // Quét tìm điểm cực trị
    vector<pair<double,double>> extrema;
    for (double xi=-5; xi<=5; xi+=0.05) {
        double d1 = derivative(f, xi);
        double d2 = derivative(f, xi+0.05);

        if (d1*d2 < 0) { // dấu đổi => có nghiệm gần xi
            double x0 = xi;
            double fx = f(x0);
            double f2 = second_derivative(f, x0);
            extrema.emplace_back(x0, fx);
            cout << "Cuc " << (f2>0 ? "tieu" : "dai")
                 << " tai x = " << x0 << ", f(x)=" << fx << endl;
        }
    }

    // Vẽ đồ thị
    Gnuplot gp;
    gp << "set title 'Do thi f(x) va cac diem cuc tri'\n";
    gp << "set grid\n";
    gp << "set xlabel 'x'\nset ylabel 'f(x)'\n";
    gp << "plot '-' with lines lw 2 title 'f(x)', "
          "'-' with points pt 7 ps 1.5 lc rgb 'red' title 'Cuc tri'\n";
    gp.send1d(data_f);
    gp.send1d(extrema);

    return 0;
}
