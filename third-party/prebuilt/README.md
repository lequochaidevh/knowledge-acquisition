
### Install prebuilt package

```sh
# 1. Download the Ubuntu package without installing it
apt-get download libopengl0:amd64

# 2. Extract the contents directly into your directory
dpkg-deb -x libopengl0_*.deb "$LOCAL_MINOR_ROOT"

# 3. Create symlink
ln -sf $LOCAL_MINOR_ROOT/usr/lib/x86_64-linux-gnu/libOpenGL.so.0.0.0 \
$LOCAL_MINOR_ROOT/usr/lib/x86_64-linux-gnu/libOpenGL.so
```

### Script support remove package install specifically path (-x)

```sh
export PKG_REMOVE="name.deb"
remove_package.sh
```