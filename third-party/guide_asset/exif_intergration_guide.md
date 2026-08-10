# How to intergrate exif to the system.


## Description
- Intergrate lib-exif to internal system of the project.
- Support for c++ 17.

## Install lib-exif for internal project.

**Check set up root to install libraries**
```sh
sudo apt-get update
sudo apt-get install -y autopoint libtool gettext pkg-config
echo $LOCAL_MINOR_ROOT
```

**Get lib-exif source for ubuntu 20 (c++17 - no mention).**
```sh
git submodule add https://github.com/libexif/libexif.git
cd gst-build

cd libexif/
autoreconf -i
./configure --prefix=$LOCAL_MINOR_ROOT --disable-static
```

**Internal project installation.**
```sh
make
make install
```
