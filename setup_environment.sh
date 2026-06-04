#!/usr/bin/bash -e

AT_HERE_DIR=$(pwd)

cd internal_root/
export LOCAL_MINOR_ROOT=$(pwd)
echo $LOCAL_MINOR_ROOT
cd $AT_HERE_DIR

