# Step 1: Tell pkg-config to prefer your custom installed GStreamer packages
export PKG_CONFIG_PATH=$LOCAL_MINOR_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH

# Step 2: Initialize the CMake build directory
mkdir build
cd build
cmake ..

# Step 3: Compile your C++ application
make

# Tell the system runtime where to find your custom shared libraries
export LD_LIBRARY_PATH=$LOCAL_MINOR_ROOT/lib:$LD_LIBRARY_PATH

# Run your application
./GStreamerApp