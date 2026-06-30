# How to intergrate googletest to the system.

## Description
- Intergrate googletest to internal system of the project for testing, (todo CICD).

## Install googletest for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get googletest source for ubuntu 20 (c++17).**
```sh
git submodule add https://github.com/google/googletest.git
cd fmt/
```

**Pin it to a specific stable release tag.**
```sh
cd googletest/
mkdir build
cd build/
```

**Internal project installation.**
```sh
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT
make
make install
```
**Note:** Check internal_root when counter failed.
