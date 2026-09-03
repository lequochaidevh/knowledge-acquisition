# How to intergrate terminal ui to the system.

## Description
- Intergrate terminal ui to internal system of the project.
- Terminal can use UI basic, mouse on-click.

## Install terminal ui for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get terminal ui source for ubuntu 20 (c++17).**
```sh
git submodule add https://github.com/ArthurSonzogni/FTXUI.git
```

**Pin it to a specific stable release tag.**
```sh
cd FTXUI/
git checkout v7.0.1
mkdir build
cd build/
```

**Internal project installation.**
```sh
# Configure the build targeting the custom installation path and enforcing C++17 standard
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT \
         -DCMAKE_CXX_STANDARD=17 \
         -DCMAKE_BUILD_TYPE=Release

# Compile using multiple cores and install files to the target directory
make -j$(nproc)
make install
```
**Note:** Check internal_root when counter failed.
