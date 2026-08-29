#include "../include/Tensor2D.h"
#include "../include/DenseLayer.h"
#include "../include/Loss.h"
#include "../include/Optimizer.h"

int main() {
    // 1. Initialize input and target data
    Tensor2D input(1, 3, 1.0f);   // 1 sample with 3 features // Batch * Input Feature
    Tensor2D target(1, 2, 0.5f);  // Target output values

    // 2. Initialize components
    DenseLayer   layer(3, 2);  // Input * Output feature
    MSELoss      criterion;
    SGDOptimizer optimizer(0.1f);  // Learning rate = 0.1

    // 3. Run training loop for 5 epochs
    for (int epoch = 0; epoch < 5; ++epoch) {
        optimizer.zero_grad(layer);
        // Forward pass
        Tensor2D pred = layer.forward(input);
        float    loss = criterion.forward(pred, target);
        std::cout << "Epoch " << epoch << " - Loss: " << loss << "\n";

        // Backward pass
        Tensor2D loss_grad = criterion.backward(pred, target);
        layer.backward(loss_grad);

        // Update weights and biases via optimizer
        optimizer.step(layer);
    }

    return 0;
}
