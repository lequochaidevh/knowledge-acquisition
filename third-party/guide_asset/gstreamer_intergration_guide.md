# How to intergrate gstreamer to the system.

## Description
- Intergrate gstreamer to internal system of the project.
- Support for c++ 17.

## Install gstreamer for internal project.

**Check set up root to install libraries**
```sh
sudo apt install meson
echo $LOCAL_MINOR_ROOT
```

**Get gstreamer source for ubuntu 20 (c++17). And gstreamer available in c++20.**
```sh
git submodule add https://github.com/GStreamer/gst-build.git
cd gst-build
```

**Pin it to a specific stable release tag (recommended for Ubuntu 20.04 compatibility)**
```sh
<!-- git checkout 1.16.3 -->
```

**Internal project installation.**
```sh
meson setup build/ --prefix=$LOCAL_MINOR_ROOT --libdir=lib -Dtests=disabled -Dexamples=disabled
cd build/
ninja
DESTDIR=$LOCAL_MINOR_ROOT ninja install
```
