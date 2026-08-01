# How to intergrate asio to the system.

## Add Asio as a git submodule in your thirdparty directory
```sh
git submodule add https://github.com/chriskohlhoff/asio.git
```
## Move into the asio directory
```sh
cd asio/
```
## Ensure the destination directory exists inside your custom root
```sh
mkdir -p $LOCAL_MINOR_ROOT/include
```
## Copy the master header file and the core library folder
```sh
cp asio/include/asio.hpp $LOCAL_MINOR_ROOT/include/
cp -r asio/include/asio $LOCAL_MINOR_ROOT/include/
```
