#include "../include/Loss.h"
#include "../include/DenseLayer.h"

int main() {
    // 1. Setup mock data
    Tensor2D input(1, 3, 1.0f);   // 1 sample, 3 features
    Tensor2D target(1, 2, 0.5f);  // 1 sample, 2 target outputs

    DenseLayer layer(3, 2);
    MSELoss    criterion;

    // 2. Forward execution
    Tensor2D output = layer.forward(input);
    float    loss   = criterion.forward(output, target);
    std::cout << "Initial Loss: " << loss << "\n";

    // 3. Backward execution
    Tensor2D loss_grad  = criterion.backward(output, target);
    Tensor2D input_grad = layer.backward(loss_grad);

    std::cout << "Weights Gradient (d_weights):\n";
    layer.d_weights.print();

    std::cout << "Bias Gradient (d_bias):\n";
    layer.d_bias.print();

    return 0;
}