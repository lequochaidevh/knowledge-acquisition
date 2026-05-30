#!/usr/bin/bash -e

# sudo apt-get install libfmt-dev

mkdir build/

cp *.cpp build/
cp *.h build/

cd build/

g++ -std=c++17 -c logger.cpp -o logger.o

ar rcs liblogging.a logger.o

g++ -std=c++17 main.cpp module_a.cpp module_b.cpp -L. -llogging -lfmt -o logging_app

./logging_app
