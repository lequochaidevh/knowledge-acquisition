#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>

std::string read_shader_file(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open shader file: " << filepath << std::endl;
        exit(EXIT_FAILURE);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compile_compute_shader(const char* filepath) {
    std::string shaderCode = read_shader_file(filepath);
    const char* src        = shaderCode.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "❌ Compute Shader Compilation Error:\n" << infoLog << std::endl;
        exit(EXIT_FAILURE);
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    return program;
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1, 1, "Benchmark", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // Define matrix dimensions (1024x2048 * 2048x1024)
    const int M = 1024, N = 2048, K = 1024;
    const int ITERATIONS = 1;  // Number of loops for benchmark

    std::cout << "🖥️  GPU Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "🎮 GPU Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "🚀 Benchmarking over " << ITERATIONS << " iterations...\n";

    // Random runtime generation in-place
    std::mt19937                          gen(42);  // Keep fixed seed for uniform benchmark loads
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    std::vector<float> h_A(M * N);
    std::vector<float> h_B(N * K);
    for (int i = 0; i < M * N; ++i) h_A[i] = dis(gen);
    for (int i = 0; i < N * K; ++i) h_B[i] = dis(gen);

    GLuint ssboA, ssboB, ssboC;
    glGenBuffers(1, &ssboA);
    glGenBuffers(1, &ssboB);
    glGenBuffers(1, &ssboC);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboA);
    glBufferData(GL_SHADER_STORAGE_BUFFER, h_A.size() * sizeof(float), h_A.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboA);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboB);
    glBufferData(GL_SHADER_STORAGE_BUFFER, h_B.size() * sizeof(float), h_B.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboB);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboC);
    glBufferData(GL_SHADER_STORAGE_BUFFER, M * K * sizeof(float), NULL, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboC);

    GLuint shaderProgram = compile_compute_shader("benchmark.comp");
    glUseProgram(shaderProgram);
    glUniform1i(0, M);
    glUniform1i(1, N);
    glUniform1i(2, K);

    GLuint num_groups_x = (K + 15) / 16;
    GLuint num_groups_y = (M + 15) / 16;

    // Setup OpenGL Time Queries for precise hardware measurement
    GLuint query;
    glGenQueries(1, &query);

    // Warm-up run (GPU initialization latency spike exclusion)
    glDispatchCompute(num_groups_x, num_groups_y, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glFinish();

    // --- BENCHMARK START ---
    auto cpu_start = std::chrono::high_resolution_clock::now();

    glBeginQuery(GL_TIME_ELAPSED, query);

    for (int i = 0; i < ITERATIONS; ++i) {
        glDispatchCompute(num_groups_x, num_groups_y, 1);
        // Force dependency execution ordering between dispatches without reading back to CPU
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    glEndQuery(GL_TIME_ELAPSED);
    glFinish();  // Wait for all GPU tasks to explicitly complete

    auto cpu_end = std::chrono::high_resolution_clock::now();
    // --- BENCHMARK END ---

    // Retrieve GPU Time query results (in nanoseconds)
    GLuint64 gpu_time_ns = 0;
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &gpu_time_ns);

    double total_gpu_time_ms = static_cast<double>(gpu_time_ns) / 1000000.0;
    double avg_gpu_time_ms   = total_gpu_time_ms / ITERATIONS;
    double total_cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    // Calculate Flops count: Each element requires N multiplications and N additions => 2 * M * N * K
    double operations_per_iter = 2.0 * M * N * K;
    double gflops              = (operations_per_iter / (avg_gpu_time_ms / 1000.0)) / 1e9;

    std::cout << "\n📊 --- BENCHMARK RESULTS ---" << std::endl;
    std::cout << "⏱️ Total GPU Time (" << ITERATIONS << " runs): " << total_gpu_time_ms << " ms" << std::endl;
    std::cout << "⏱️ Avg Execution Time per multiplication: " << avg_gpu_time_ms << " ms" << std::endl;
    std::cout << "⏱️ Total Wall Clock Time (CPU perspective): " << total_cpu_time_ms << " ms" << std::endl;
    std::cout << "⚡ Performance Density: " << gflops << " GFLOPS" << std::endl;

    glDeleteQueries(1, &query);
    glDeleteBuffers(1, &ssboA);
    glDeleteBuffers(1, &ssboB);
    glDeleteBuffers(1, &ssboC);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
