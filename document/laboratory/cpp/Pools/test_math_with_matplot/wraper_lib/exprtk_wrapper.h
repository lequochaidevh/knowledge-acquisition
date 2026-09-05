#pragma once
#include "exprtk.hpp"
#include <functional>
#include <string>
#include <iostream>

class ExprEvaluator {
public:
    ExprEvaluator(const std::string &expr_str);
    double eval(double x) const;
    bool valid() const { return compiled_; }

private:
    mutable double x_;
    bool compiled_;
    exprtk::symbol_table<double> sym_table_;
    exprtk::expression<double> expression_;
    exprtk::parser<double> parser_;
};


class Expression {
    typedef exprtk::symbol_table<double> symbol_table_t;
    typedef exprtk::expression<double>   expression_t;
    typedef exprtk::parser<double>       parser_t;

    double x_;
    symbol_table_t sym_table_;
    expression_t expression_;
    parser_t parser_;

public:
    Expression(const std::string& expr_str) {
        sym_table_.add_variable("x", x_);
        sym_table_.add_constants();
        expression_.register_symbol_table(sym_table_);
        if (!parser_.compile(expr_str, expression_)) {
            throw std::runtime_error("Failed to parse expression: " + expr_str);
        }
    }

    double eval(double x) {
        x_ = x;
        return expression_.value();
    }
};

class ExprEvaluator2D {
public:
    ExprEvaluator2D(const std::string &expr_str);

    double eval(double x, double y) const;

    bool valid() const;

private:
    mutable double x_, y_;
    bool compiled_;
    exprtk::symbol_table<double> sym_table_;
    exprtk::expression<double> expression_;
    exprtk::parser<double> parser_;
};