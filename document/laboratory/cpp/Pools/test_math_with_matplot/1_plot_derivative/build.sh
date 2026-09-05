#!/usr/bin/bash

mkdir -p build

g++ plot_derivative.cpp $(python3-config --includes) $(python3-config --ldflags) -std=c++17 -O2 -o plot_derivative

# wget https://raw.githubusercontent.com/ArashPartow/exprtk/master/exprtk.hpp -O exprtk.hpp
#  git clone https://github.com/lava/matplotlib-cpp.git

# sudo apt install gnuplot gnuplot-x11 libboost-iostreams-dev
# sudo apt install gnuplot gnuplot-x11 libboost-iostreams-dev libboost-system-dev

# g++ -v plot_derivative_v2.cpp -std=c++17 -O2 -lboost_iostreams -lboost_system -lboost_filesystem -o build/plot_function
# BUILD FAST - NO OPTIMIZE: -O0 -g
./build/plot_derivative

#rm -rf build