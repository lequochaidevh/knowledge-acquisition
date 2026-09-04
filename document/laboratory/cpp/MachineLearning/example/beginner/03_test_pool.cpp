#include "../../include/ImageLoader.h"
#include "../../include/Conv2DLayer.h"
#include "../../include/Tensor2D.h"
#include "../../include/MaxPool2DLayer.h"

int main() {
    std::cout << "==================================================\n";
    std::cout << "--- MAX POOLING LAYER REAL-WORLD VALIDATION ---  \n";
    std::cout << "==================================================\n\n";

    // 1. Hydrate the edge-detected feature map image from the previous test
    std::string input_path = "output_edges.png";
    std::cout << "Step 1: Loading feature map image into 1-Channel layout...\n";
    Tensor4D edge_image;
    try {
        edge_image = ImageLoader::load_to_tensor(input_path, 1);
        std::cout << ">> Input Feature Map Shape: [B=" << edge_image.get_batch() << ", C=" << edge_image.get_channels()
                  << ", H=" << edge_image.get_height() << ", W=" << edge_image.get_width() << "]\n\n";
    } catch (const std::exception& e) {
        std::cerr << ">> PIPELINE ERROR: " << e.what() << "\n";
        std::cerr << ">> Please ensure that 'output_edges.png' exists by running the Conv2D test first.\n";
        return 1;
    }

    // 2. Instantiate MaxPool2D layer with default parameters (Window Size = 2, Stride = 2)
    std::cout << "Step 2: Constructing MaxPool2DLayer instance (Pool=2, Stride=2)...\n";
    MaxPool2DLayer pool_layer(2, 2);

    // 3. Execution Core: Run forward downsampling pass over the feature matrix
    std::cout << "Step 3: Triggering multi-threaded max-pooling execution pass...\n";
    Tensor4D pooled_output = pool_layer.forward(edge_image);

    std::cout << ">> Downsampled Output Shape  : [B=" << pooled_output.get_batch()
              << ", C=" << pooled_output.get_channels() << ", H=" << pooled_output.get_height()
              << ", W=" << pooled_output.get_width() << "]\n\n";

    // 4. Verify mathematical dimension constraints scaling alignment
    // Mathematical formula verification rule: (225 - 2) / 2 + 1 = 223 / 2 + 1 = 111 + 1 = 112
    if (pooled_output.get_height() == 112 && pooled_output.get_width() == 112) {
        std::cout << ">> ARITHMETIC CHECK: SUCCESS! Matrix downscaling ratio matches perfectly.\n";
    } else {
        std::cout << ">> ARITHMETIC CHECK: FAILED! Dimension mismatch detected.\n";
        return 1;
    }

    // 5. Serialize the downsampled feature matrix back to disk
    std::string output_path = "output_pooled.png";
    std::cout << "\nStep 4: Writing downsampled tensor back to disk as '" << output_path << "'...\n";
    try {
        ImageLoader::save_from_tensor(output_path, pooled_output);
        std::cout << ">> VALIDATION STATUS: SUCCESS! Pooled asset exported cleanly.\n";
        std::cout << "==================================================\n";
    } catch (const std::exception& e) {
        std::cerr << ">> WRITE ERROR ENCOUNTERED: " << e.what() << "\n";
        return 1;
    }

    return 0;
}