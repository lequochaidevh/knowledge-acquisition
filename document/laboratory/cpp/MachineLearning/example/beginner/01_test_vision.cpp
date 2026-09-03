// g++ -O3 -fopenmp test_vision.cpp -o test_vision
#include "../../include/ImageLoader.h"
#include "../../include/Tensor2D.h"

int main() {
    std::cout << "==================================================\n";
    std::cout << "--- COMPUTER VISION CORE MODULES VALIDATION ---   \n";
    std::cout << "==================================================\n\n";

    std::string image_path = "test.jpeg";
    std::cout << "Step 1: Attempting to read image file: '" << image_path << "'...\n";

    Tensor2D dummy_output_test;  // Placeholder if using legacy structures
    Tensor4D image_tensor;

    try {
        // Load the image into 3 channels (RGB) formatted as [Batch=1, Channels=3, Height, Width]
        image_tensor = ImageLoader::load_to_tensor(image_path, 3);
        std::cout << ">> SUCCESS: Image decoded and bound to Tensor4D seamlessly.\n\n";
    } catch (const std::exception& e) {
        std::cerr << ">> CRITICAL PIPELINE ERROR: " << e.what() << "\n";
        std::cerr << ">> Please verify that 'test.jpg' exists in the execution directory.\n";
        return 1;
    }

    // -------------------------------------------------------------------------
    // TEST 1: Inspect Architectural Topology Metadata Layout Dimensions
    // -------------------------------------------------------------------------
    std::cout << "--------------------------------------------------\n";
    std::cout << " [TEST 1] METADATA LAYOUT INSPECTION \n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "-> Batch Dimension (B)    : " << image_tensor.get_batch() << " (1 image processed)\n";
    std::cout << "-> Channels Dimension (C) : " << image_tensor.get_channels() << " (RGB Planar format Layout)\n";
    std::cout << "-> Height Dimension (H)   : " << image_tensor.get_height() << " pixels\n";
    std::cout << "-> Width Dimension (W)    : " << image_tensor.get_width() << " pixels\n\n";

    // Inspect localized center pixel values normalized between [0.0f -> 1.0f]
    size_t mid_h = image_tensor.get_height() / 2;
    size_t mid_w = image_tensor.get_width() / 2;
    std::cout << "Normalized RGB floats profile at center pixel coord [" << mid_h << ", " << mid_w << "]:\n";
    std::cout << "  Red Channel   : " << image_tensor.at(0, 0, mid_h, mid_w) << "\n";
    std::cout << "  Green Channel : " << image_tensor.at(0, 1, mid_h, mid_w) << "\n";
    std::cout << "  Blue Channel  : " << image_tensor.at(0, 2, mid_h, mid_w) << "\n\n";

    // -------------------------------------------------------------------------
    // TEST 2: High-Performance Zero-Copy GEMM Matrix Multiplication Validation
    // -------------------------------------------------------------------------
    std::cout << "--------------------------------------------------\n";
    std::cout << " [TEST 2] OPTIMIZED GEMM MATMUL VALIDATION \n";
    std::cout << "--------------------------------------------------\n";
    std::cout << "Testing inline strides tracking logic without copying memory...\n";

    // Construct Matrix A as a 2x3 slice matrix encapsulated in [B=1, C=1, H=2, W=3]
    Tensor4D matA(1, 1, 2, 3);
    matA.at(0, 0, 0, 0) = 1.0f;
    matA.at(0, 0, 0, 1) = 2.0f;
    matA.at(0, 0, 0, 2) = 3.0f;
    matA.at(0, 0, 1, 0) = 4.0f;
    matA.at(0, 0, 1, 1) = 5.0f;
    matA.at(0, 0, 1, 2) = 6.0f;

    // Construct Matrix B as a 2x3 slice matrix encapsulated in [B=1, C=1, H=2, W=3]
    // Since B is 2x3, we will set transB = true to treat it as a 3x2 matrix on-the-fly!
    Tensor4D matB(1, 1, 2, 3);
    matB.at(0, 0, 0, 0) = 7.0f;
    matB.at(0, 0, 0, 1) = 8.0f;
    matB.at(0, 0, 0, 2) = 9.0f;
    matB.at(0, 0, 1, 0) = 10.0f;
    matB.at(0, 0, 1, 1) = 11.0f;
    matB.at(0, 0, 1, 2) = 12.0f;

    Tensor4D matC;  // Empty container to receive product arrays dynamically

    // Execute product: C (2x2) = A (2x3) * B^T (3x2)
    std::cout << "Executing: C = matmul_gemm(A, transA=false, B, transB=true)...\n";
    Tensor4D::matmul_gemm(matA, false, matB, true, matC);

    // Present localized output matrices evaluation maps
    std::cout << "Result Matrix C dimensions: " << matC.get_height() << "x" << matC.get_width() << "\n";
    std::cout << "Matrix C values array output:\n";
    for (size_t i = 0; i < matC.get_height(); ++i) {
        std::cout << "  [ ";
        for (size_t j = 0; j < matC.get_width(); ++j) {
            std::cout << std::setw(6) << matC.at(0, 0, i, j) << " ";
        }
        std::cout << "]\n";
    }

    // Mathematical assertion verify logic:
    // Row0-Col0 expected calculation: (1*7) + (2*8) + (3*9) = 7 + 16 + 27 = 50
    // Row1-Col1 expected calculation: (4*10) + (5*11) + (6*12) = 40 + 55 + 72 = 167
    if (std::abs(matC.at(0, 0, 0, 0) - 50.0f) < 1e-4f && std::abs(matC.at(0, 0, 1, 1) - 167.0f) < 1e-4f) {
        std::cout << ">> GEMM STATUS: SUCCESS! Dynamic stride indexing operates correctly.\n";
        std::cout << "==================================================\n";
    } else {
        std::cout << ">> GEMM STATUS: CRITICAL ARITHMETIC MISMATCH FLUSHED.\n";
        std::cout << "==================================================\n";
        return 1;
    }

    return 0;
}