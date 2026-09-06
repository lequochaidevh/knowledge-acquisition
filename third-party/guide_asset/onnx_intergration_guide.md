# How to intergrate onnx to the system.

## Description
- Intergrate onnx to internal system of the project.
A helper to accelerate matrix math with the GPU.

## Install onnx for internal project.

**Check set up root to install libraries**
```sh
echo $LOCAL_MINOR_ROOT
```

**Get onnx source for ubuntu 20 (c++17).**
```sh
### manual clone source suitable with hardware type.
### todo wget
https://github.com/microsoft/onnxruntime/releases/tag/v1.29.0
```

```sh
# 1. Download the archive directly using your verified link
cd /tmp
wget https://github.com/microsoft/onnxruntime/releases/download/v1.29.0/onnxruntime-linux-x64-1.29.0.tgz

tar -xvf onnxruntime-linux-x64-1.29.0.tgz \
--strip-components=1 -C $LOCAL_MINOR_ROOT \
    onnxruntime-linux-x64-1.29.0/include \
    onnxruntime-linux-x64-1.29.0/lib
```

**Internal project installation.**
```sh
cp -rf include/ lib/ $LOCAL_MINOR_ROOT
```
**Note:** Check internal_root when counter failed.
