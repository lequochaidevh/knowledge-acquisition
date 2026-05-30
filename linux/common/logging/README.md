# Logging Static Library

### Setup environment
**Prerequisites:** `source setup_environment.sh` before build and install.

### Build library
```sh
mkdir build/
cd build/
cmake ..
make
```

### Install library
```sh
make install
```
**Result:**
```css
[100%] Built target logging
Install the project...
-- Install configuration: ""
-- Installing: /home/USER/knowledge-acquisition/linux/common/logging/lib/liblogging.a
-- Installing: /home/USER/knowledge-acquisition/linux/common/logging/include/logger.h
```
