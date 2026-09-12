#/bin/bash

_ROOT_DIR="$(realpath $(dirname ${0}))"
_BUILD_DIR="${_ROOT_DIR}/build"

mkdir "${_BUILD_DIR}"

g++ QOI4MCU.cpp \
-o "${_BUILD_DIR}"/qoi_tool \
-I"${LOCAL_MINOR_ROOT}/include/stb/" \
-std=gnu++17 -O3

cp -rf test.jpeg "${_BUILD_DIR}"

cd "${_BUILD_DIR}"

./qoi_tool test.jpeg
# ./sandbox
# rm -rf "${_BUILD_DIR}"

g++ ../DecompessQOI.cpp \
-o "${_BUILD_DIR}"/DecompessQOI \
-I"${LOCAL_MINOR_ROOT}/include/stb/" \
-std=gnu++17 -O3

./DecompessQOI

sync
