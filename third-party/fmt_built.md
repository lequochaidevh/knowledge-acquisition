
# Set up root to install libraries
echo $LOCAL_MINOR_ROOT

# Get fmt source for ubuntu 20
cd ..
<!-- git clone  -->
cd fmt/

# Pin it to a specific stable release tag (recommended for Ubuntu 20.04 compatibility)
<!-- git checkout 10.2.1 -->
mkdir build/
cd build/
cmake .. -DCMAKE_INSTALL_PREFIX=$LOCAL_MINOR_ROOT -DFMT_TEST=OFF -DFMT_DOC=OFF
make
make install
