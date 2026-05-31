#!/usr/bin/bash

rm -rf build/

mkdir build/

cp *.cpp build/
# cp *.h build/

mkdir -p build/logging/
cp ../../*.cpp build/logging/
cp ../../*.h build/logging/

cd build/

g++ -std=c++17 -c logging/logger.cpp -o logger.o

ar rcs liblogging.a logger.o

g++ -std=c++17 main.cpp module_a.cpp module_b.cpp -L. -llogging -lfmt -lpthread -o logging_app

./logging_app
