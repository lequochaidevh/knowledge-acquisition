#include "../wraper_lib/exprtk_wrapper.h"
#include <iostream>
#include <vector>
#include <tuple>
#include <cmath>
#include <fstream>
#include <gnuplot-iostream.h>

// --- Derivative ---
double partial_x(const ExprEvaluator2D &f, double x, double y, double h = 1e-4) {
    return (f.eval(x + h, y) - f.eval(x - h, y)) / (2 * h);
}

double partial_y(const ExprEvaluator2D &f, double x, double y, double h = 1e-4) {
    return (f.eval(x, y + h) - f.eval(x, y - h)) / (2 * h);
}

// --- Gradient descent ---
std::pair<double, double> gradient_descent(
    const ExprEvaluator2D &f, double x0, double y0,
    double alpha = 0.1, int max_iter = 1000, double eps = 1e-6)
{
    double x = x0, y = y0;
    for (int i = 0; i < max_iter; ++i) {
        double gx = partial_x(f, x, y);
        double gy = partial_y(f, x, y);
        double gnorm = std::sqrt(gx * gx + gy * gy);
        if (gnorm < eps) break;

        x -= alpha * gx;
        y -= alpha * gy;
    }
    return {x, y};
}

// --- Put data to surface + vector gradient ---
void export_surface_and_gradient(const ExprEvaluator2D &f,
                                 double xmin, double xmax,
                                 double ymin, double ymax,
                                 double step)
{
    std::ofstream file("surface_gradient.dat");
    for (double x = xmin; x <= xmax; x += step) {
        for (double y = ymin; y <= ymax; y += step) {
            double z = f.eval(x, y);
            double gx = partial_x(f, x, y);
            double gy = partial_y(f, x, y);
            file << x << " " << y << " " << z << " " << gx << " " << gy << "\n";
        }
        file << "\n";
    }
    std::cout << "Export data to: surface_gradient.dat\n";
}

// ---Draw 3D ---
void plot_surface(const ExprEvaluator2D &f,
                  double xmin = -3, double xmax = 3,
                  double ymin = -3, double ymax = 3,
                  double step = 0.2)
{
    Gnuplot gp;
    std::vector<std::tuple<double, double, double>> surface;
    for (double x = xmin; x <= xmax; x += step)
        for (double y = ymin; y <= ymax; y += step)
            surface.emplace_back(x, y, f.eval(x, y));

    gp << "set title 'f(x,y) Surface'\n";
    gp << "set xlabel 'x'\nset ylabel 'y'\nset zlabel 'f(x,y)'\n";
    gp << "set hidden3d\nset dgrid3d 30,30 qnorm 2\n";
    gp << "splot '-' with lines title 'Surface'\n";
    gp.send1d(surface);
}

// --- Vẽ đường đồng mức + vector gradient + điểm cực trị ---
void plot_gradient_field(const ExprEvaluator2D &f,
                         double xmin, double xmax,
                         double ymin, double ymax,
                         double step,
                         double min_x, double min_y)
{
    Gnuplot gp;
    gp << "set title '3D Surface with Gradient Vectors'\n";
    gp << "set xlabel 'x'\nset ylabel 'y'\nset zlabel 'f(x,y)'\n";
    gp << "set hidden3d\nset ticslevel 0\n";
    gp << "unset key\n";

    // --- Data ---
    std::vector<std::tuple<double, double, double>> surface;
    std::vector<std::tuple<double, double, double, double, double, double>> vectors;
    for (double x = xmin; x <= xmax; x += step) {
        for (double y = ymin; y <= ymax; y += step) {
            double z = f.eval(x, y);
            double gx = partial_x(f, x, y);
            double gy = partial_y(f, x, y);
            surface.emplace_back(x, y, z);

            // mỗi vài ô thì vẽ một vector nhỏ ở vị trí (x, y, z)
            if (int((x - xmin) / step) % 3 == 0 && int((y - ymin) / step) % 3 == 0)
                vectors.emplace_back(x, y, z, gx, gy, 0.0);
        }
    }

    double zmin = f.eval(min_x, min_y);
    std::vector<std::tuple<double, double, double>> minimum_point = {
        {min_x, min_y, zmin}
    };

    // --- Draw ---
    gp << "splot '-' using 1:2:3 with lines lc 'gray' title 'Surface', "
          "'-' using 1:2:3:4:5:6 with vectors head filled lc 'red' title 'Gradient', "
          "'-' using 1:2:3 with points pt 7 ps 1.5 lc 'blue' title 'Minimum'\n";
    gp.send1d(surface);
    gp.send1d(vectors);
    gp.send1d(minimum_point);
}

int main() {
    std::string expr_str;
    std::cout << "Input f(x,y) (ex: x^2 + y^2): ";
    std::getline(std::cin, expr_str);

    ExprEvaluator2D f(expr_str);
    if (!f.valid()) {
        std::cerr << "ERROR: Can not parse the function!\n";
        return 1;
    }

    double x, y;
    std::cout << "Input point (x, y): ";
    std::cin >> x >> y;

    double fx = partial_x(f, x, y);
    double fy = partial_y(f, x, y);
    std::cout << "Gradient at (" << x << "," << y << "): "
              << "df/dx = " << fx << ", df/dy = " << fy << "\n";

    // Gradient descent
    auto [xmin, ymin] = gradient_descent(f, x, y, 0.1);
    double zmin = f.eval(xmin, ymin);

    std::cout << "Minimum point nearest: (" << xmin << ", " << ymin
              << "), f = " << zmin << "\n";

    plot_surface(f);
    export_surface_and_gradient(f, -3, 3, -3, 3, 0.2);
    plot_gradient_field(f, -3, 3, -3, 3, 0.2, xmin, ymin);

    return 0;
}
