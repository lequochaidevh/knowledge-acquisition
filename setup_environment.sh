#!/usr/bin/bash -e

SETUP_ENV_SCRIPT_PATH=$(pwd)

cd internal_root/
export LOCAL_MINOR_ROOT=$(pwd)
export MINOR_ROOT_PATH=${LOCAL_MINOR_ROOT}
echo "Minor root path: $MINOR_ROOT_PATH"
cd $SETUP_ENV_SCRIPT_PATH

cd util/cmake/
export CMAKE_UTIL_PATH=$(pwd)
echo "CMake utils path: $CMAKE_UTIL_PATH"
cd $SETUP_ENV_SCRIPT_PATH

