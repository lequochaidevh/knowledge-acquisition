#/bin/bash

_ROOT_DIR="$(realpath $(dirname ${0}))"
_BUILD_DIR="${_ROOT_DIR}/build/"

mkdir "${_BUILD_DIR}"

g++ train.cpp -o build/builder -std=gnu++17 -O3 -fopenmp -mavx2 

g++ runner.cpp -o build/runner -std=gnu++17 -O3 -fopenmp -mavx2 

cd "${_BUILD_DIR}"

./builder

./runner

# careful with build/builder compile not fulfill
rm -rf "${_BUILD_DIR}"

sync
