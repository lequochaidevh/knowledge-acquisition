#!/usr/bin/bash

mkdir -p build

g++ newton_solve_equation.cpp ../wraper_lib/exprtk_wrapper.o -std=c++17 -O2 \
    -lboost_iostreams -lboost_system -lboost_filesystem -o build/newton_solve_equation

# g++ $1.cpp ../wraper_lib/exprtk_wrapper.o -std=c++17 -O2 -o build/$1

./build/newton_solve_equation