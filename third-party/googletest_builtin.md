git submodule add https://github.com/google/googletest.git

cd googletest/

mkdir build

cd build/

**Note:** Need source setup_env.
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT

make

make install

**Note:** Check internal_root.

