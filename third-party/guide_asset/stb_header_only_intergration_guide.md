# How to intergrate stb header only to the system.

## Description
- Intergrate stb header only to internal system of the project for image processing.

## Install stb header only for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get stb header only source for ubuntu 20 (c++17).**
```sh
git submodule add https://github.com/nothings/stb.git
```

**Pin it to a specific stable release tag.**
```sh
cd stb/
mkdir -p $LOCAL_MINOR_ROOT/include/stb
cp -rf *.h $LOCAL_MINOR_ROOT/include/stb
```

**Note:** Check internal_root when counter failed.
