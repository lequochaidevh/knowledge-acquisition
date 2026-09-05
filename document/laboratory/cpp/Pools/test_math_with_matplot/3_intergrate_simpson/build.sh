#!/usr/bin/bash

mkdir -p build

g++ intergrate.cpp ../wraper_lib/exprtk_wrapper.o -std=c++17 -O2 \
    -lboost_iostreams -lboost_system -lboost_filesystem -o build/intergrate

./build/intergrate
