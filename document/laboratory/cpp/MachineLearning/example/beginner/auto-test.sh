#/bin/bash

_ROOT_DIR="$(realpath $(dirname ${0}))"
_BUILD_DIR="${_ROOT_DIR}/build"

mkdir "${_BUILD_DIR}"

g++ 02_test_conv.cpp \
-o "${_BUILD_DIR}"/sandbox \
-I"${LOCAL_MINOR_ROOT}/include/stb/" \
-std=gnu++17 -O3 -fopenmp -mavx2 

cp -rf test.jpeg "${_BUILD_DIR}"

cd "${_BUILD_DIR}"

./sandbox

# rm -rf "${_BUILD_DIR}"

sync
