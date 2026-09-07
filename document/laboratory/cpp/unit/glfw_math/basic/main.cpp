#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <array>
#include <fstream>
#include <random>
#include <sstream>

// // Simple Compile-Time Pseudo-Random Generator (LCG)
// constexpr float compile_time_random(unsigned int seed, int index) {
//     unsigned long long state = seed + index * 1103515245ULL;
//     state                    = (state * 1103515245ULL + 12345ULL);
//     // Scale to a float between 0.0f and 1.0f
//     return static_cast<float>(state % 65536) / 65535.0f;
// }

// // Generates a fully baked static array at compile time
// template <size_t Size>
// constexpr std::array<float, Size> generate_static_matrix(unsigned int seed) {
//     std::array<float, Size> arr{};
//     for (size_t i = 0; i < Size; ++i) {
//         arr[i] = compile_time_random(seed, i);
//     }
//     return arr;
// }

// Reads shader source code from a file
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

// Compiles the Compute Shader and links the program
GLuint compile_compute_shader(const char* filepath) {
    std::string shaderCode = read_shader_file(filepath);
    const char* src        = shaderCode.c_str();

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    // Check compilation errors
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

    // Check linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "❌ Shader Program Linking Error:\n" << infoLog << std::endl;
        exit(EXIT_FAILURE);
    }

    glDeleteShader(shader);
    return program;
}

int main() {
    // 1. Initialize GLFW and create a Headless/Windowless context for max performance
    if (!glfwInit()) {
        std::cerr << "❌ Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Hidden window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1, 1, "Compute Context", NULL, NULL);
    if (!window) {
        std::cerr << "❌ Failed to create hidden GLFW Context" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 2. Initialize GLAD to load OpenGL function pointers (linked from $LOCAL_MINOR_ROOT)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "❌ Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int M = 1024, N = 2048, K = 1024;
    std::cout << "🚀 Allocating and generating matrices in-place..." << std::endl;

    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    // 1. Allocate space immediately (No push_back, no sizing overhead)
    std::vector<float> h_A(M * N);
    std::vector<float> h_B(N * K);
    std::vector<float> h_C(M * K, 0.0f);

    // 2. Generate random numbers directly into the final memory locations
    for (size_t i = 0; i < h_A.size(); ++i) h_A[i] = dis(gen);
    for (size_t i = 0; i < h_B.size(); ++i) h_B[i] = dis(gen);

    // 3. Initialize SSBOs (Shader Storage Buffer Objects) on GPU VRAM
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
    glBufferData(GL_SHADER_STORAGE_BUFFER, h_C.size() * sizeof(float), NULL, GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboC);

    // 4. Compile and activate Compute Shader
    GLuint shaderProgram = compile_compute_shader("matrix_mul.comp");
    glUseProgram(shaderProgram);

    // Pass matrix dimensions via uniforms
    glUniform1i(0, M);
    glUniform1i(1, N);
    glUniform1i(2, K);

    // 5. Dispatch Compute Work Groups
    // Local work group size is 16x16 (defined inside matrix_mul.comp)
    GLuint num_groups_x = (K + 15) / 16;
    GLuint num_groups_y = (M + 15) / 16;

    glDispatchCompute(num_groups_x, num_groups_y, 1);

    // Memory Barrier: Force CPU to wait until GPU finish all memory writes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 6. Read back the calculated results from VRAM to Host RAM
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboC);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, h_C.size() * sizeof(float), h_C.data());

    std::cout << "✅ Computation complete!\n";

    // Print a 5x5 preview of the top-left corner
    int preview_rows = std::min(M, 15);
    int preview_cols = std::min(K, 10);
    std::cout << "📋 Matrix C Preview (" << preview_rows << "x" << preview_cols << " out of " << M << "x" << K << "):\n";

    for (int i = 0; i < preview_rows; ++i) {
        for (int j = 0; j < preview_cols; ++j) {
            std::cout << h_C[i * K + j] << "\t\t";
        }
        std::cout << "...\n\n";
    }
    for (int j = 0; j < preview_cols; ++j) {
        std::cout << "...\t\t";
    }
    std::cout << "...\n";

    // Cleanup resources
    glDeleteBuffers(1, &ssboA);
    glDeleteBuffers(1, &ssboB);
    glDeleteBuffers(1, &ssboC);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}