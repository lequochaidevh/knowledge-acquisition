#!/usr/bin/bash

mkdir build
g++ main.cpp -std=c++17 -o ./build/generate_wizard_complete
./build/generate_wizard_complete cli_tree.json > ./build/auto_complete.sh
bash -n ./build/auto_complete.sh
source ./build/auto_complete.sh
rm -rf build
