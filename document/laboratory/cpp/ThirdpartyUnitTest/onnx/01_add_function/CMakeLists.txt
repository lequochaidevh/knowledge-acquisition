cmake_minimum_required(VERSION 3.15)
project(onnx_matrix_sandbox)

set(CMAKE_CXX_STANDARD 17)

# 1. Paths configuration
set(INTERNAL_ROOT "$ENV{LOCAL_MINOR_ROOT}")
set(MODEL_GENERATOR_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/create_asset.py")
set(GENERATED_MODEL_PATH "${CMAKE_CURRENT_BINARY_DIR}/matrix_add.onnx")

# 2. Automation: Rule to generate matrix_add.onnx using Python
add_custom_command(
    OUTPUT ${GENERATED_MODEL_PATH}
    COMMAND python3 ${MODEL_GENERATOR_SCRIPT}
    DEPENDS ${MODEL_GENERATOR_SCRIPT}
    COMMENT "Generating ONNX model asset via Python script..."
)

# Create a custom target that forces the custom command to run
add_custom_target(generate_onnx_model DEPENDS ${GENERATED_MODEL_PATH})

# 3. Executable configuration
add_executable(onnx_matrix_sandbox main.cpp)

# Make the executable depend on the model generation target
add_dependencies(onnx_matrix_sandbox generate_onnx_model)

# 4. Include directories & Link libraries (ONNX Runtime)
target_include_directories(onnx_matrix_sandbox PRIVATE "${INTERNAL_ROOT}/include")
target_link_libraries(onnx_matrix_sandbox PRIVATE "${INTERNAL_ROOT}/lib/libonnxruntime.so")

# Runtime path for Linux shared library (.so) lookup
set_target_properties(onnx_matrix_sandbox PROPERTIES LINK_FLAGS "-Wl,-rpath,${INTERNAL_ROOT}/lib")
