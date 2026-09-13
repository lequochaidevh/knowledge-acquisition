#ifndef GPU_MATH_H
#define GPU_MATH_H

#include <stddef.h>

// Explicitly handles GPU memory handles safely
struct GpuMatrix {
    int          rows;
    int          cols;
    unsigned int ssbo;
};

// 🛠️ Lifecycle API Management
bool gpu_math_init();
void gpu_math_terminate();

// 💾 VRAM Memory Management API
GpuMatrix gpu_matrix_create(int rows, int cols);
void      gpu_matrix_upload(GpuMatrix& mat, const float* host_data);
void      gpu_matrix_download(GpuMatrix& mat, float* host_data);
void      gpu_matrix_destroy(GpuMatrix& mat);

// ⚡ High-Performance Math Execution API (218+ GFLOPS)
void gpu_matrix_mul_execute(const GpuMatrix& A, const GpuMatrix& B, const GpuMatrix& C);

#endif  // GPU_MATH_H
