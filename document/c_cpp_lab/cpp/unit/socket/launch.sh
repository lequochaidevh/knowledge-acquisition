#!/bin/bash

usage() {
    echo "Please run with number prefix of the file"
    echo "Example: $0 0"
    echo ""
    echo "Please export SRC_FILE (cpp) before run"
    echo "dgram.cpp or stream.cpp"
}

if [ ! -e "build" ]; then
    mkdir build/
fi

if [ ! -z $1 ] && [ ! -z ${SRC_FILE} ]; then
    PROJECT_FILE=$(find -name $1*)
    echo "Build project ${PROJECT_FILE} with ${SRC_FILE}"
    g++ ${PROJECT_FILE}/${SRC_FILE} -o build/execute -lpthread -std=c++17 && ./build/execute
else
    usage
fi