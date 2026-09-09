cmake_minimum_required(VERSION 3.10)
project(TaskQueueProject VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard to C++17 or higher (std::thread requires at least C++11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the threads library (pthread on Linux)
find_package(Threads REQUIRED)

# Define your executable and its source files
add_executable(sandbox task_queue.cpp)

# Specify the directory for header files
target_include_directories(sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../)

# Link the threads library to your executable
target_link_libraries(sandbox PRIVATE Threads::Threads)
