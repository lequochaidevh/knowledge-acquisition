# How to intergrate nlohmann json to the system.

## Description
- Intergrate nlohmann json to internal system of the project for feature: logging, string processing, ...
- Support for c++ 17 without nlohmann json.

## Install nlohmann json for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get nlohmann json source for ubuntu 20 (c++17). And nlohmann json available in c++20.**
```sh
git submodule add https://github.com/nlohmann/json.git
cd nlohmann json/
```

**Pin it to a specific stable release tag (No comment)**
```sh
mkdir build/
cd build/
```

**Internal project installation.**
```sh
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT -DJSON_BuildTests=OFF
make
make install
```

**Cmake config**
```sh
find_package(nlohmann_json REQUIRED PATHS ${INTERNAL_ROOT_SEARCH_PATH} NO_DEFAULT_PATH)
target_link_libraries(<app> PRIVATE <others> nlohmann_json::nlohmann_json)
```