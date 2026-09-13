#include <onnxruntime_cxx_api.h>
#include <vector>
#include <iostream>

int main() {
    // 1. Initialize the ONNX Runtime environment
    Ort::Env            env(ORT_LOGGING_LEVEL_WARNING, "MatrixMathTest");
    Ort::SessionOptions session_options;

    // (Optional) Enable GPU acceleration if using the GPU pre-built binary:
    // OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 0);

    // 2. Load the target ONNX model file
    // Ensure the file exists in your execution directory or use an absolute path
    const char*  model_path = "matrix_add.onnx";
    Ort::Session session(env, model_path, session_options);

    // 3. Prepare input data (e.g., a flat vector representing a 2x2 matrix)
    std::vector<float>   input_tensor_values = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<int64_t> input_shape         = {2, 2};  // Matrix dimensions: 2x2

    // Allocate memory and map the CPU buffer to an ONNX Runtime Value object
    auto       memory_info  = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

    // 4. Define input and output node names matching the exported ONNX model
    const char* input_names[]  = {"input"};
    const char* output_names[] = {"output"};

    // 5. Execute the computation (Inference)
    auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);

    // 6. Extract and verify the output matrix data
    float* float_arr = output_tensors.front().GetTensorMutableData<float>();
    std::cout << "First element of the output matrix: " << float_arr[0] << std::endl;

    return 0;
}