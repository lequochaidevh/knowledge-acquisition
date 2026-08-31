// g++ test.cpp -std=c++17 && ./a.out && rm a.out
#include "optimizer.h"

int basic_cross_entropy() {
    // 1. Target labels (Actual facts): 1 means Spam, 0 means Regular Email
    constexpr std::array<double, 4> targets{1.0, 0.0, 1.0, 0.0};

    // 2. Predictions (Model probability output): Close to 1.0 or 0.0
    // The model is confident on sample 0, 1, 2 but made a wrong guess on sample 3 (predicted 0.8 instead of near 0.0)
    constexpr std::array<double, 4> predictions{0.92, 0.05, 0.88, 0.80};

    // Entire loss loop is computed during compilation!
    constexpr double loss = compute_binary_cross_entropy<4>(targets, predictions);

    std::cout << "--- Classification Optimization Engine ---\n";
    std::cout << "Calculated Binary Cross-Entropy Loss: " << loss << "\n";

    return 0;
}

int main() {
    // Initialize Weight Matrix at a tricky coordinate prone to getting stuck in local minima
    Matrix<3, 3> W{1.5, 0.5, 1.3, 1.5, 0.5, -0.8};

    double learning_rate     = 0.05;
    double momentum_friction = 0.9;  // Retain 90% velocity

    // Run the momentum accelerated engine
    train_with_momentum<3, 3>(W, learning_rate, momentum_friction, 100);

    std::cout << "\nOptimized Weight Final Value: \n";
    W.print();
    std::cout << "\n";

    return 0;
}