#include "gpu_math.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

static GLFWwindow* g_context_window = nullptr;
static GLuint      g_shader_program = 0;

// Hardcoded stable flattened 1D shared memory kernel source
const char* COMPUTE_SHADER_SRC = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(location = 0) uniform int M;
layout(location = 1) uniform int N;
layout(location = 2) uniform int K;
layout(std430, binding = 0) readonly buffer BufferA { float A[]; };
layout(std430, binding = 1) readonly buffer BufferB { float B[]; };
layout(std430, binding = 2) writeonly buffer BufferC { float C[]; };
shared float Asub[16 * 16];
shared float Bsub[16 * 16];
void main() {
    int row = int(gl_GlobalInvocationID.y);
    int col = int(gl_GlobalInvocationID.x);
    int localRow = int(gl_LocalInvocationID.y);
    int localCol = int(gl_LocalInvocationID.x);
    int localIdx = localRow * 16 + localCol;
    float sum = 0.0;
    int numTiles = (N + 15) / 16;
    for (int t = 0; t < numTiles; ++t) {
        int globalColA = t * 16 + localCol;
        int globalRowB = t * 16 + localRow;
        Asub[localIdx] = (row < M && globalColA < N) ? A[row * N + globalColA] : 0.0;
        Bsub[localIdx] = (globalRowB < N && col < K) ? B[globalRowB * K + col] : 0.0;
        barrier();
        for (int k = 0; k < 16; ++k) {
            sum += Asub[localRow * 16 + k] * Bsub[k * 16 + localCol];
        }
        barrier();
    }
    if (row < M && col < K) C[row * K + col] = sum;
}
)";

bool gpu_math_init() {
    if (!glfwInit()) return false;

    // Set core OpenGL Profile requirements
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 🤫 THE INVISIBLE THUNDERBOLT FLAGS: Force total context concealment
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);        // Tell the OS not to map it visually
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);      // Strip away close/minimize borders
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);        // Do not steal focus from your terminal
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);  // Stay dead quiet in the background

    // Scale it to a tiny 1x1 buffer block so it remains completely hidden from the desktop compositor
    g_context_window = glfwCreateWindow(1, 1, "Headless Engine Proxy", NULL, NULL);
    if (!g_context_window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(g_context_window);

    // Bind driver entry points natively
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;
    glfwSwapInterval(0);

    // Compile Compute Shader binaries
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &COMPUTE_SHADER_SRC, NULL);
    glCompileShader(shader);

    g_shader_program = glCreateProgram();
    glAttachShader(g_shader_program, shader);
    glLinkProgram(g_shader_program);
    glDeleteShader(shader);

    return true;
}

GpuMatrix gpu_matrix_create(int rows, int cols) {
    GpuMatrix mat{rows, cols, 0};
    glGenBuffers(1, &mat.ssbo);

    // 🎯 INITIALIZE BUFFER SPACE IMMEDIATELY (Prevents 0-size VRAM allocation bugs)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat.ssbo);
    size_t size = rows * cols * sizeof(float);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_READ);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    return mat;
}

void gpu_matrix_upload(GpuMatrix& mat, const float* host_data) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat.ssbo);
    size_t size = mat.rows * mat.cols * sizeof(float);
    // Use SubData since the storage memory block already exists safely
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, host_data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void gpu_matrix_download(GpuMatrix& mat, float* host_data) {
    // Force complete GPU execution fence synchronization
    glFinish();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mat.ssbo);
    size_t size = mat.rows * mat.cols * sizeof(float);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, host_data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void gpu_matrix_mul_execute(const GpuMatrix& A, const GpuMatrix& B, const GpuMatrix& C) {
    glUseProgram(g_shader_program);
    glUniform1i(0, A.rows);
    glUniform1i(1, A.cols);
    glUniform1i(2, B.cols);
    // Bind the Storage Buffer Objects to slots 0, 1, 2
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, A.ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, B.ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, C.ssbo);
    glDispatchCompute((B.cols + 15) / 16, (A.rows + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void gpu_matrix_destroy(GpuMatrix& mat) {
    if (mat.ssbo) glDeleteBuffers(1, &mat.ssbo);
}

void gpu_math_terminate() {
    if (g_shader_program) glDeleteProgram(g_shader_program);
    if (g_context_window) glfwDestroyWindow(g_context_window);
    glfwTerminate();
}
