#include "../../include/ImageLoader.h"
#include "../../include/Conv2DLayer.h"
#include "../../include/Tensor2D.h"

int main() {
    std::cout << "==================================================\n";
    std::cout << "--- CONVOLUTIONAL LAYER REAL-WORLD VALIDATION --- \n";
    std::cout << "==================================================\n\n";

    // 1. Hydrate your 225x225 image in Grayscale layout (1 Channel instead of 3)
    std::string input_path = "test.jpeg";
    std::cout << "Step 1: Loading image target into 1-Channel layout...\n";
    Tensor4D gray_image;
    try {
        gray_image = ImageLoader::load_to_tensor(input_path, 1);
        std::cout << ">> Image Shape: [B=" << gray_image.get_batch() << ", C=" << gray_image.get_channels()
                  << ", H=" << gray_image.get_height() << ", W=" << gray_image.get_width() << "]\n\n";
    } catch (const std::exception& e) {
        std::cerr << ">> PIPELINE ERROR: " << e.what() << "\n";
        return 1;
    }

    // 2. Instantiate a Conv2D layer with Padding=1 to preserve physical resolution
    // Specs: 1 Input Channel, 1 Output Channel, Kernel Size = 3, Stride = 1, Padding = 1
    std::cout << "Step 2: Constructing Conv2DLayer instance (Padding=1, Stride=1)...\n";
    Conv2DLayer edge_detector(1, 1, 3, 1, 1);

    // 3. HARDCODE OVERWRITE: Load standard Sobel Horizontal Edge extraction matrices
    // Sobel Kernel configuration shape:
    //  [ -1  0  1 ]
    //  [ -2  0  2 ]
    //  [ -1  0  1 ]
    edge_detector.weights.at(0, 0, 0, 0) = -1.0f;
    edge_detector.weights.at(0, 0, 0, 1) = 0.0f;
    edge_detector.weights.at(0, 0, 0, 2) = 1.0f;
    edge_detector.weights.at(0, 0, 1, 0) = -2.0f;
    edge_detector.weights.at(0, 0, 1, 1) = 0.0f;
    edge_detector.weights.at(0, 0, 1, 2) = 2.0f;
    edge_detector.weights.at(0, 0, 2, 0) = -1.0f;
    edge_detector.weights.at(0, 0, 2, 1) = 0.0f;
    edge_detector.weights.at(0, 0, 2, 2) = 1.0f;

    // 4. Execution Core: Pass image through the custom Conv2D layer forward pass
    std::cout << "Step 3: Triggering multi-threaded forward pass convolution sweep...\n";
    Tensor2D dummy_test_link;  // Structural backport reference marker if required
    Tensor4D output_feature_map = edge_detector.forward(gray_image);

    std::cout << ">> Output Feature Map Shape: [B=" << output_feature_map.get_batch()
              << ", C=" << output_feature_map.get_channels() << ", H=" << output_feature_map.get_height()
              << ", W=" << output_feature_map.get_width() << "]\n\n";

    // 5. Present performance success and serialize feature extraction result back to file
    std::string output_path = "output_edges.png";
    std::cout << "Step 4: Writing output tensor map back to disk as '" << output_path << "'...\n";
    try {
        ImageLoader::save_from_tensor(output_path, output_feature_map);
        std::cout << ">> VALIDATION STATUS: SUCCESS! Verification map exported cleanly.\n";
        std::cout << "==================================================\n";
    } catch (const std::exception& e) {
        std::cerr << ">> WRITE ERROR FLUSHED: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
