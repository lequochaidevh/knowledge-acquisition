#include "../../../include/Tensor2D.h"
#include "../../../include/DenseLayer.h"
#include "../../../include/Loss.h"
#include "../../../include/Optimizer.h"
#include "../../../include/ModelCheckpoint.h"

int main() {
    std::cout << "--- PRODUCTION INFERENCE APPLICATION ---\n";

    // 1. Recreate the EXACT same network structure skeleton (Topology must match!)
    DenseLayer layer1(3, 14);  // 3 inputs -> 14 hidden units
    DenseLayer layer2(14, 1);  // 14 hidden units -> 1 output

    // 2. Bind the layer parameters to the Checkpoint Manager
    ModelCheckpoint checkpoint;
    checkpoint.register_parameter("layer1.weights", layer1.weights);
    checkpoint.register_parameter("layer1.bias", layer1.bias);
    checkpoint.register_parameter("layer2.weights", layer2.weights);
    checkpoint.register_parameter("layer2.bias", layer2.bias);

    // 3. Hydrate all matrix parameters directly from the saved file
    std::cout << "Loading trained weights from 'mymodel.safetensors'...\n";
    checkpoint.load("mymodel.safetensors");
    std::cout << "Model successfully loaded! Ready for deployment.\n\n";

    // 4. Hardcode your static unknown test sample directly
    Tensor2D new_customer(1, 3);
    new_customer.at(0, 0) = 5.5f;  // Income
    new_customer.at(0, 1) = 8.5f;  // Purchase Frequency
    new_customer.at(0, 2) = 2.5f;  // Account Age (Looks like a VIP)

    // 5. Execution Phase: Run standard Forward Pass (NO loss, NO backward, NO optimizer!)
    Tensor2D out_h1     = layer1.forward(new_customer);
    Tensor2D out_h1_act = out_h1.relu();
    Tensor2D out_final  = layer2.forward(out_h1_act);
    Tensor2D final_pred = out_final.sigmoid();  // Probability scaled between [0.0, 1.0]

    // 6. Present the output results
    std::cout << "--- INFERENCE RESULT ---\n";
    std::cout << "Input sample features : [5.5, 8.5, 2.5]\n";
    std::cout << "Predicted VIP Probability: " << final_pred.at(0, 0) << "\n";

    if (final_pred.at(0, 0) >= 0.5f) {
        std::cout << "Classification Decision: CUSTOMER IS A VIP!\n";
    } else {
        std::cout << "Classification Decision: CUSTOMER IS NORMAL.\n";
    }

    return 0;
}