```sh
### ==================================================
git submodule add https://github.com/CLIUtils/CLI11.git
cd CLI11

# Create build directory
mkdir build && cd build

cmake .. \
  -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
  -DCLI11_BUILD_TESTS=OFF

make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/gabime/spdlog.git

cd spdlog

mkdir build && cd build

cmake .. \
  -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
  -DSPDLOG_BUILD_SHARED=ON \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DSPDLOG_BUILD_TESTS=OFF

make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
pip3 install --target=/tmp/cmake_temp cmake==3.30.2

mkdir -p "$LOCAL_MINOR_ROOT/cmake"

mv /tmp/cmake_temp/cmake/data/* "$LOCAL_MINOR_ROOT/cmake/"

rm -rf /tmp/cmake_temp

$LOCAL_MINOR_ROOT/cmake/bin/cmake --version
### ==================================================

### ==================================================
git submodule add https://github.com/gazebosim/gz-utils

cd gz-utils

mkdir build && cd build

$LOCAL_MINOR_ROOT/cmake/bin/cmake .. \
  -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
  -DCMAKE_PREFIX_PATH=$LOCAL_MINOR_ROOT


make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/gazebosim/gz-cmake.git

cd gz-cmake && mkdir build && cd build

$LOCAL_MINOR_ROOT/cmake/bin/cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT

$LOCAL_MINOR_ROOT/cmake/bin/cmake .. \
  -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
  -DCMAKE_PREFIX_PATH=$LOCAL_MINOR_ROOT


make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/swig/swig.git
cd swig

git checkout tags/v4.0.2

./autogen.sh

./configure --prefix=$LOCAL_MINOR_ROOT

make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/gazebosim/gz-math.git
cd ~/gz-math

mkdir build && cd build

$LOCAL_MINOR_ROOT/cmake/bin/cmake .. \
  -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
  -DCMAKE_PREFIX_PATH=$LOCAL_MINOR_ROOT \
  -DPython3_EXECUTABLE=/usr/bin/python3 \
  -DPython3_FIND_STRATEGY=LOCATION \
    -Wno-dev

make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/iplinux/xorg-util-macros.git
cd xorg-util-macros/
./autogen.sh --prefix=$LOCAL_MINOR_ROOT
./configure --prefix=$LOCAL_MINOR_ROOT
make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/deepin-community/libxmu.git
cd libxmu
./autogen.sh --prefix=$LOCAL_MINOR_ROOT
./configure --prefix=$LOCAL_MINOR_ROOT
touch aclocal.m4 configure Makefile.am Makefile.in config.h.in Makefile
make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/deepin-community/libxt.git
cd libxt

export ACLOCAL_PATH=$LOCAL_MINOR_ROOT/share/aclocal

# make distclean 2>/dev/null || make clean 2>/dev/null
# rm -rf autom4te.cache/ config.status config.log aclocal.m4

autoreconf -vfi

./configure --prefix=$LOCAL_MINOR_ROOT
make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
export PKG_CONFIG_PATH="$LOCAL_MINOR_ROOT/lib/pkgconfig:$LOCAL_MINOR_ROOT/share/pkgconfig:$PKG_CONFIG_PATH"

git submodule add https://github.com/winlibs/libxpm.git
cd libxpm
git checkout libxpm-3.5.12

autoreconf -vfi
./configure --prefix=$LOCAL_MINOR_ROOT
make -j$(( $(nproc) - 3 ))
make install
### ==================================================

### ==================================================
git submodule add https://github.com/deepin-community/libxaw.git

autoreconf -vfi

./configure --prefix=$LOCAL_MINOR_ROOT
make -j$(( $(nproc) - 3 )) && make install
### ==================================================

```