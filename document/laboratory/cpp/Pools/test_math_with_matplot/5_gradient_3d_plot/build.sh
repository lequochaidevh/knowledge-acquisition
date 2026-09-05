#!/usr/bin/bash

mkdir -p build

g++ *.cpp ../wraper_lib/exprtk_wrapper.o -std=c++17 -O2 \
    -lboost_iostreams -lboost_system -lboost_filesystem -o build/output

./build/output


# set view 60,30
# set dgrid3d 30,30
# set hidden3d
# splot "surface_gradient.dat" using 1:2:3 with lines title "Surface", \
#       "surface_gradient.dat" using 1:2:($3+0.2):4:5:(0.2) with vectors head filled lt 2 title "Gradient"
