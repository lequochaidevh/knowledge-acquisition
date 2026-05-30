#!/usr/bin/bash -e

# sudo apt-get install libfmt-dev

mkdir build/

g++ -std=c++17 logging_sample.cpp -lfmt -o ./build/logger

./build/logger
