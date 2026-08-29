#include "../include/Tensor2D.h"
#include "../include/DenseLayer.h"
#include "../include/Loss.h"
#include "../include/Optimizer.h"
#include <iostream>

int main() {
    // 1. Data Setup: 4 samples, 3 features each
    // Features: [Income, Purchase Frequency, Account Age]
    Tensor2D input(4, 3);
    input.at(0, 0) = 5.0f;
    input.at(0, 1) = 9.0f;
    input.at(0, 2) = 2.0f;  // Sample 1 (VIP)
    input.at(1, 0) = 1.0f;
    input.at(1, 1) = 2.0f;
    input.at(1, 2) = 0.5f;  // Sample 2 (Normal)
    input.at(2, 0) = 6.0f;
    input.at(2, 1) = 8.0f;
    input.at(2, 2) = 3.0f;  // Sample 3 (VIP)
    input.at(3, 0) = 1.5f;
    input.at(3, 1) = 1.0f;
    input.at(3, 2) = 1.0f;  // Sample 4 (Normal)

    // Targets: 1 output neuron (1.0 = VIP, 0.0 = Normal)
    Tensor2D target(4, 1);
    target.at(0, 0) = 1.0f;
    target.at(1, 0) = 0.0f;
    target.at(2, 0) = 1.0f;
    target.at(3, 0) = 0.0f;

    // 2. Network Setup: Multi-Layer architecture
    // Layer 1 (Hidden): 3 Inputs -> 14 Hidden Features (Extracts complex patterns)
    DenseLayer layer1(3, 14);
    // Layer 2 (Output): 14 Hidden Features -> 1 Output Prediction (Final decision)
    DenseLayer layer2(14, 1);

    MSELoss      criterion;
    SGDOptimizer optimizer(0.01f);  // Learning rate

    // 3. Training Loop (20 epochs to show convergence)
    std::cout << "--- START TRAINING ---\n";
    for (int epoch = 0; epoch < 120; ++epoch) {
        optimizer.zero_grad(layer1);
        optimizer.zero_grad(layer2);
        // --- FORWARD PASS ---
        Tensor2D h1           = layer1.forward(input);
        Tensor2D h1_activated = h1.relu();  // Apply non-linear activation
        Tensor2D pred         = layer2.forward(h1_activated);

        float loss = criterion.forward(pred, target);
        if (epoch % 5 == 0 || epoch == 19) {
            std::cout << "Epoch " << epoch << " - Loss: " << loss << "\n";
        }

        // --- BACKWARD PASS ---
        Tensor2D loss_grad = criterion.backward(pred, target);

        // Backprop through Layer 2
        Tensor2D grad_h1_activated = layer2.backward(loss_grad);
        // Backprop through ReLU
        Tensor2D grad_h1 = h1.relu_backward(grad_h1_activated);
        // Backprop through Layer 1
        layer1.backward(grad_h1);

        // --- UPDATE WEIGHTS ---
        optimizer.step(layer1);
        optimizer.step(layer2);
    }

    // 4. Inference / Recognition test on a brand new unknown sample
    std::cout << "\n--- RECOGNITION TEST ON NEW DATA ---\n";
    Tensor2D new_customer(1, 3);
    new_customer.at(0, 0) = 5.5f;
    new_customer.at(0, 1) = 8.5f;
    new_customer.at(0, 2) = 2.5f;  // Looks like a VIP

    Tensor2D out_h1     = layer1.forward(new_customer);
    Tensor2D out_h1_act = out_h1.relu();
    Tensor2D out_final  = layer2.forward(out_h1_act);
    Tensor2D final_pred = out_final.sigmoid();

    std::cout << "Input features: [5.5, 8.5, 2.5]\n";
    std::cout << "Predicted Score (Closer to 1.0 means VIP): " << final_pred.at(0, 0) << "\n";

    return 0;
}
