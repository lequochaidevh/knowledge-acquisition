#!/usr/bin/bash -e

mkdir build/

cd build/

cmake ..

make

./logging_test
