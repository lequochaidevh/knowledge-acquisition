# Build Gstreamer app
### Step 1: Tell pkg-config to prefer your custom installed GStreamer packages
```sh
export PKG_CONFIG_PATH=$LOCAL_MINOR_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Step 2: Initialize the CMake build directory
```sh
mkdir build
cd build
cmake ..
```

### Step 3: Compile your C++ application
```sh
make
```

# Run app
### Tell the system runtime where to find your custom shared libraries
```sh
export LD_LIBRARY_PATH=$LOCAL_MINOR_ROOT/lib:$LD_LIBRARY_PATH
```

### Run your application
```sh
./GStreamerApp
```

# Test waylandsink
### Install
```sh
# check echo $XDG_SESSION_TYPE
sudo apt-get update
sudo apt-get install weston
weston
```
### 1. Point paths to your custom local installation folder
```sh
export LD_LIBRARY_PATH=$LOCAL_MINOR_ROOT/lib:$LD_LIBRARY_PATH
export GST_PLUGIN_PATH=$LOCAL_MINOR_ROOT/lib/gstreamer-1.0
```

### 2. Force the application to route down the target nested Wayland environment socket
```sh
export WAYLAND_DISPLAY=wayland-1  ### Change to wayland-0 if wayland-1 fails
export XDG_RUNTIME_DIR=/run/user/$(id -u)
```

### 3. Test your pipeline command
```sh
$LOCAL_MINOR_ROOT/bin/gst-launch-1.0 videotestsrc ! waylandsink
```