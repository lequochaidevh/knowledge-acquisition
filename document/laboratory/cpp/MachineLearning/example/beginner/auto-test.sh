#/bin/bash

_ROOT_DIR="$(realpath $(dirname ${0}))"
_BUILD_DIR="${_ROOT_DIR}/build"

mkdir "${_BUILD_DIR}"

# g++ 02_test_conv.cpp \
# -o "${_BUILD_DIR}"/sandbox2 \
# -I"${LOCAL_MINOR_ROOT}/include/stb/" \
# -std=gnu++17 -O3 -fopenmp -mavx2 

# g++ 03_test_pool.cpp \
# -o "${_BUILD_DIR}"/sandbox3 \
# -I"${LOCAL_MINOR_ROOT}/include/stb/" \
# -std=gnu++17 -O3 -fopenmp -mavx2 

g++ 04_test_full_pipeline.cpp \
-o "${_BUILD_DIR}"/sandbox4 \
-I"${LOCAL_MINOR_ROOT}/include/stb/" \
-std=gnu++17 -O3 -fopenmp -mavx2 

cp -rf test.jpeg "${_BUILD_DIR}"

cd "${_BUILD_DIR}"

# ./sandbox2
# ./sandbox3
./sandbox4
# rm -rf "${_BUILD_DIR}"

sync
