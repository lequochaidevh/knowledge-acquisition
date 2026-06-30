# How to intergrate fmt to the system.

## Description
- Intergrate fmt to internal system of the project for feature: logging, string processing, ...
- Support for c++ 17 without fmt.

## Install fmt for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get fmt source for ubuntu 20 (c++17). And fmt available in c++20.**
```sh
git submodule add https://github.com/fmtlib/fmt.git
cd fmt/
```

**Pin it to a specific stable release tag (recommended for Ubuntu 20.04 compatibility)**
```sh
<!-- git checkout 10.2.1 -->
mkdir build/
cd build/
```

**Internal project installation.**
```sh
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT -DFMT_TEST=OFF -DFMT_DOC=OFF
make
make install
```
