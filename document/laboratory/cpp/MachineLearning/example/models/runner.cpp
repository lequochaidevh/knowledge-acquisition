#include "../../include/Tensor2D.h"

#include "../../include/DenseLayer.h"
#include "../../include/ReLULayer.h"
#include "../../include/SigmoidLayer.h"
#include "../../include/Sequential.h"

#include "../../include/Loss.h"
#include "../../include/Optimizer.h"

#include "../../include/ModelCheckpoint.h"

int main() {
    std::cout << "==================================================\n";
    std::cout << "--- PRODUCTION RUNNER: STANDALONE INFERENCE ---  \n";
    std::cout << "==================================================\n\n";

    // 1. Reconstruct the EXACT same layer topology as used in training
    // Topology blueprint: 3 Inputs -> 9 Hidden Units (ReLU) -> 1 Output (Sigmoid)
    Sequential model;
    model.add(new DenseLayer(3, 9, "relu"));
    model.add(new ReLULayer());
    // model.add(new LeakyReLULayer()); // same relu
    model.add(new DenseLayer(9, 1, "sigmoid"));
    model.add(new SigmoidLayer());

    // 2. Hydrate model parameters directly from the serialized file checkpoint
    std::cout << "Attempting to load trained parameters from disk...\n";
    try {
        model.load("mymodel.safetensors");
        std::cout << ">> Success: 'mymodel.safetensors' loaded smoothly.\n\n";
    } catch (const std::exception& e) {
        std::cerr << ">> CRITICAL LOAD ERROR: " << e.what() << "\n";
        std::cerr << ">> Please ensure you have executed the training script first.\n";
        return 1;
    }

    // 3. Setup a batch of multiple unknown test customer samples (4 customers, 3 features each)
    // Features array format layout: [Income, Purchase Frequency, Account Age]
    size_t   num_test_samples = 4;
    Tensor2D new_customers(num_test_samples, 3);

    // Customer 0: High income, very active, long account age -> Definitely VIP
    new_customers.at(0, 0) = 5.5f;
    new_customers.at(0, 1) = 8.5f;
    new_customers.at(0, 2) = 2.5f;

    // Customer 1: Low income, rarely buys, brand new account -> Definitely Normal
    new_customers.at(1, 0) = 0.8f;
    new_customers.at(1, 1) = 1.5f;
    new_customers.at(1, 2) = 0.2f;

    // Customer 2: Medium-high income, medium frequency -> Borderline case
    new_customers.at(2, 0) = 6.0f;
    new_customers.at(2, 1) = 5.0f;
    new_customers.at(2, 2) = 1.5f;

    // Customer 3: Low income, but extremely loyal and frequent buyer -> Potential VIP
    new_customers.at(3, 0) = 1.2f;
    new_customers.at(3, 1) = 9.0f;
    new_customers.at(3, 2) = 4.0f;

    // 4. Execution Core: Run a single batch feed-forward pass
    // Matrix dimensions: (4x3) passing through network results in a (4x1) prediction matrix
    std::cout << "Processing batch inference feed-forward pipeline...\n";
    Tensor2D batch_preds = model.forward(new_customers);

    // 5. Present the prediction analysis and definitive decision threshold outputs for each profile
    std::cout << "\n-------------------------------------------------------------\n";
    std::cout << "                    BATCH INFERENCE PROFILE                  \n";
    std::cout << "-------------------------------------------------------------\n";

    for (size_t i = 0; i < num_test_samples; ++i) {
        float probability_score = batch_preds.at(i, 0);

        std::cout << "Customer #" << i << " | Features: [" << new_customers.at(i, 0) << ", " << new_customers.at(i, 1)
                  << ", " << new_customers.at(i, 2) << "]\n";
        std::cout << "            | Probability Score: " << probability_score << "\n";

        // Evaluate binary threshold metric (Standard classification boundary = 0.5)
        std::cout << "            | Final Decision   : ";
        if (probability_score >= 0.5f) {
            std::cout << ">> CUSTOMER IS A VIP! <<\n";
        } else {
            std::cout << ">> CUSTOMER IS NORMAL. <<\n";
        }
        std::cout << "-------------------------------------------------------------\n";
    }

    return 0;
}