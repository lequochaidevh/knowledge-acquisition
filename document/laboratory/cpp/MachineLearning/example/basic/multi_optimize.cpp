#include "../include/Tensor2D.h"
#include "../include/DenseLayer.h"
#include "../include/Loss.h"
#include "../include/Optimizer.h"

int main() {
    // 1. Initialize 3 input samples with 3 features each (Batch size = 3, In features = 3)
    Tensor2D input(3, 3);
    input.at(0, 0) = 1.0f;
    input.at(0, 1) = 2.0f;
    input.at(0, 2) = 3.0f;  // Sample 1
    input.at(1, 0) = 0.5f;
    input.at(1, 1) = 1.0f;
    input.at(1, 2) = 1.5f;  // Sample 2
    input.at(2, 0) = 2.0f;
    input.at(2, 1) = 0.5f;
    input.at(2, 2) = 1.0f;  // Sample 3

    // 2. Initialize corresponding 3 target outputs (Batch size = 3, Out features = 2)
    Tensor2D target(3, 2);
    target.at(0, 0) = 0.8f;
    target.at(0, 1) = 0.2f;  // Target for Sample 1
    target.at(1, 0) = 0.4f;
    target.at(1, 1) = 0.6f;  // Target for Sample 2
    target.at(2, 0) = 0.5f;
    target.at(2, 1) = 0.5f;  // Target for Sample 3

    // 3. Initialize neural network components
    DenseLayer   layer(3, 2);  // 3 input features, 2 output neurons
    MSELoss      criterion;
    SGDOptimizer optimizer(0.05f);  // Learning rate = 0.05

    // 4. Run training loop for 150 epochs
    for (int epoch = 0; epoch < 150; ++epoch) {
        optimizer.zero_grad(layer);
        // Forward pass: (3x3) * (3x2) = (3x2) output matrix
        Tensor2D pred = layer.forward(input);

        // Compute average loss across all 3 samples
        float loss = criterion.forward(pred, target);
        std::cout << "Epoch " << epoch << " - Loss: " << loss << "\n";

        // Backward pass: compute gradients over the batch
        Tensor2D loss_grad = criterion.backward(pred, target);
        layer.backward(loss_grad);

        // Update weights and biases using accumulated batch gradients
        optimizer.step(layer);
    }

    // 5. Print final weights only once after training completes
    std::cout << "\nFinal Weights after training:\n";
    layer.weights.print();

    return 0;
}
