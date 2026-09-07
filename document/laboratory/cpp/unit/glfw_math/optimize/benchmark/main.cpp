#include "gpu_math.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>  // Required for std::min

int main() {
    // 1. Initialize the headless GPU library context
    if (!gpu_math_init()) {
        std::cerr << "❌ Library Initialization Failed! Ensure your Nvidia drivers are mapped correctly." << std::endl;
        return -1;
    }

    // Set matrix size constraints (1024x1024 is a perfect multiple of 16 for tiling)
    const int M          = 1024;
    const int N          = 2048;
    const int K          = 1024;
    const int ITERATIONS = 3;

    std::cout << "🚀 Core testing parameters via lib: " << M << "x" << N << " by " << N << "x" << K << "\n";

    // 2. Generate initial pseudo-random matrices on host CPU RAM

    std::random_device                    rd;
    std::mt19937                          gen(rd());  // Fixed seed to maintain stable performance profiling
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    std::vector<float> h_A(M * N);
    std::vector<float> h_B(N * K);
    std::vector<float> h_C(M * K, 0.0f);

    for (int i = 0; i < M * N; ++i) h_A[i] = dis(gen);
    for (int i = 0; i < N * K; ++i) h_B[i] = dis(gen);

    // 3. Allocate matrix handles directly on GPU VRAM
    GpuMatrix gpu_A = gpu_matrix_create(M, N);
    GpuMatrix gpu_B = gpu_matrix_create(N, K);
    GpuMatrix gpu_C = gpu_matrix_create(M, K);

    // 4. Upload initial host payload arrays across the PCIe lane
    gpu_matrix_upload(gpu_A, h_A.data());
    gpu_matrix_upload(gpu_B, h_B.data());

    // Hardware warm-up step to clear out baseline initialization lag spikes
    gpu_matrix_mul_execute(gpu_A, gpu_B, gpu_C);
    gpu_matrix_download(gpu_C, h_C.data());

    std::cout << "⚡ Running " << ITERATIONS << " library loops..." << std::endl;

    // --- BENCHMARK TIME TRACKING WINDOW START ---
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        gpu_matrix_mul_execute(gpu_A, gpu_B, gpu_C);
    }

    // Downloading forces a hardware checkpoint sync before grabbing timestamps
    gpu_matrix_download(gpu_C, h_C.data());

    auto end = std::chrono::high_resolution_clock::now();
    // --- BENCHMARK TIME TRACKING WINDOW END ---

    // 5. Evaluate arithmetic density throughput scores
    double total_time_ms    = std::chrono::duration<double, std::milli>(end - start).count();
    double avg_time_ms      = total_time_ms / ITERATIONS;
    double total_operations = 2.0 * M * N * K;  // Multiplications + Additions per cell
    double gflops           = (total_operations / (avg_time_ms / 1000.0)) / 1e9;

    // 6. Output performance instrumentation data log
    std::cout << "\n📊 --- ISOLATED LIBRARY BENCHMARK RESULTS ---" << std::endl;
    std::cout << "⏱️  Average Pipeline Latency: " << avg_time_ms << " ms" << std::endl;
    std::cout << "⚡ Throughput Density: " << gflops << " GFLOPS" << std::endl;

    // 7. Output a 10x15 submatrix data preview to verify values are non-zero
    int preview_rows = std::min(M, 15);
    int preview_cols = std::min(K, 10);

    std::cout << "\n📋 --- MATRIX C CORNER PREVIEW (" << preview_rows << "x" << preview_cols << ") ---" << std::endl;
    for (int i = 0; i < preview_rows; ++i) {
        for (int j = 0; j < preview_cols; ++j) {
            std::cout << h_C[i * K + j] << "\t\t";
        }
        std::cout << "...\n" << std::endl;
    }
    for (int j = 0; j < preview_cols; ++j) {
        std::cout << "...\t\t";
    }
    std::cout << "\n" << std::endl;

    // 8. Clean resource lifecycle deallocations
    gpu_matrix_destroy(gpu_A);
    gpu_matrix_destroy(gpu_B);
    gpu_matrix_destroy(gpu_C);
    gpu_math_terminate();

    return 0;
}