#include "../../include/ImageLoader.h"
#include "../../include/Conv2DLayer.h"
#include "../../include/Tensor2D.h"
#include "../../include/MaxPool2DLayer.h"
#include "../../include/FlattenLayer.h"
#include "../../include/DenseLayer.h"

int main() {
    std::cout << "====================================================================\n";
    std::cout << "---      MASTER VALIDATION: END-TO-END COMPUTER VISION PIPELINE  ---\n";
    std::cout << "====================================================================\n\n";

    // 1. Load raw image file from disk into a 1-Channel layout
    std::string input_path = "test.jpeg";
    std::cout << "Step 1: Hydrating input image to 4D Tensor space...\n";
    Tensor4D raw_image;
    try {
        raw_image = ImageLoader::load_to_tensor(input_path, 1);
        std::cout << ">> [INPUT] Image Tensor Shape: [B=" << raw_image.get_batch() << ", C=" << raw_image.get_channels()
                  << ", H=" << raw_image.get_height() << ", W=" << raw_image.get_width() << "]\n\n";
    } catch (const std::exception& e) {
        std::cerr << ">> PIPELINE EXECUTION CRASHED: " << e.what() << "\n";
        return 1;
    }

    // 2. Instantiate and configure individual system architecture layers
    std::cout << "Step 2: Constructing network modular layers skeleton...\n";
    Conv2DLayer    conv_layer(1, 1, 3, 1, 1);  // 1 in_channel, 1 out_channel, Kernel=3, Stride=1, Padding=1
    MaxPool2DLayer pool_layer(2, 2);           // Pool window = 2, Stride = 2
    FlattenLayer   flatten_layer;              // Bridge layer to unroll 4D to 2D

    // Hardcode Sobel horizontal kernel weights to ensure deterministic edge activations
    conv_layer.weights.at(0, 0, 0, 0) = -1.0f;
    conv_layer.weights.at(0, 0, 0, 1) = 0.0f;
    conv_layer.weights.at(0, 0, 0, 2) = 1.0f;
    conv_layer.weights.at(0, 0, 1, 0) = -2.0f;
    conv_layer.weights.at(0, 0, 1, 1) = 0.0f;
    conv_layer.weights.at(0, 0, 1, 2) = 2.0f;
    conv_layer.weights.at(0, 0, 2, 0) = -1.0f;
    conv_layer.weights.at(0, 0, 2, 1) = 0.0f;
    conv_layer.weights.at(0, 0, 2, 2) = 1.0f;

    // 3. Execution Phase: Flow data forward across the whole stack sequentially
    std::cout << "\nStep 3: Streaming forward pass data flow...\n";

    std::cout << "-> Executing Conv2D Layer...\n";
    Tensor4D feature_maps = conv_layer.forward(raw_image);

    std::cout << "-> Executing MaxPool2D Layer...\n";
    Tensor4D pooled_maps = pool_layer.forward(feature_maps);

    std::cout << "-> Executing Zero-Copy Flatten Layer...\n";
    Tensor2D flattened_vector = flatten_layer.forward(pooled_maps);
    std::cout << ">> [BRIDGE] Flattened Matrix 2D Shape: [" << flattened_vector.get_rows() << "x"
              << flattened_vector.get_cols() << "]\n\n";

    // 4. Final Classification: Connect the flattened vector to a DenseLayer
    // Input features dimension = 112 * 112 = 12544 nodes. Output = 2 target neurons (e.g., Object Class A vs B)
    size_t in_features  = flattened_vector.get_cols();  // Dynamically extracts 12544
    size_t out_features = 2;
    std::cout << "Step 4: Initializing final classification head (DenseLayer)..." << std::endl;
    std::cout << "   Connecting " << in_features << " inputs to " << out_features << " outputs." << std::endl;
    DenseLayer classifier_head(in_features, out_features);

    // Compute the final predictions matrix layout output score profiles
    Tensor2D output_predictions = classifier_head.forward(flattened_vector);

    // 5. Present structural integrity check summary logs
    std::cout << "\n====================================================================\n";
    std::cout << "---                   PIPELINE INTEGRITY SUMMARY                 ---\n";
    std::cout << "====================================================================\n";
    std::cout << "-> Conv2D Output Shape    : [" << feature_maps.get_batch() << ", " << feature_maps.get_channels()
              << ", " << feature_maps.get_height() << ", " << feature_maps.get_width() << "]\n";
    std::cout << "-> MaxPool2D Output Shape : [" << pooled_maps.get_batch() << ", " << pooled_maps.get_channels()
              << ", " << pooled_maps.get_height() << ", " << pooled_maps.get_width() << "]\n";
    std::cout << "-> Flatten Output Matrix  : [" << flattened_vector.get_rows() << "x" << flattened_vector.get_cols()
              << "]\n";
    std::cout << "-> Final Classification   : [" << output_predictions.get_rows() << "x"
              << output_predictions.get_cols() << "]\n\n";

    std::cout << ">> Final Raw Output Matrix Values: [ " << output_predictions.at(0, 0) << " , "
              << output_predictions.at(0, 1) << " ]\n";
    std::cout << ">> STATUS: SUCCESS! End-to-End vision network pipeline linked flawlessly.\n";
    std::cout << "====================================================================\n";

    return 0;
}