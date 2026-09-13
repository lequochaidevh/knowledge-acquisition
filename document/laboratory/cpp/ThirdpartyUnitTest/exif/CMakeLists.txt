cmake_minimum_required(VERSION 3.10)
project(YourInternalProject CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)

set(ENV{PKG_CONFIG_PATH} "$ENV{LOCAL_MINOR_ROOT}/lib/pkgconfig:$ENV{LOCAL_MINOR_ROOT}/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")

pkg_check_modules(LIBEXIF REQUIRED IMPORTED_TARGET libexif)

add_executable(sandbox 02_write_exif.cpp)

target_link_libraries(sandbox PRIVATE PkgConfig::LIBEXIF)

add_executable(reader 01_read_exif.cpp)
target_link_libraries(reader PRIVATE PkgConfig::LIBEXIF)
