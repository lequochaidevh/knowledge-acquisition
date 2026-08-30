// g++ test.cpp -std=gnu++17
#include "v0_OP.h"

int basic_function() {
    // Define a Loss Function we want to minimize: f(x1, x2) = (x1 - 3)^2 + (x2 + 5)^2
    // Mathematically, the perfect minimum is exactly at x1 = 3.0 and x2 = -5.0 (where Loss = 0)
    auto square_loss_function = [](const std::array<double, 2>& x) {
        double diff1 = x[0] - 3.0;
        double diff2 = x[1] + 5.0;
        return (diff1 * diff1) + (diff2 * diff2);
    };

    // Start from a terrible initial guess far away: x1 = 10.0, x2 = 10.0
    std::array<double, 2> start_point{10.0, 10.0};
    double                alpha = 0.1;  // Learning Rate

    // Run optimization
    std::array<double, 2> optimal_weights = gradient_descent<2>(square_loss_function, start_point, alpha);

    // Print out final trained parameters
    std::cout << "--- Gradient Descent Optimization Results ---\n";
    std::cout << "Optimized Parameter x1 (Expected ~3.0): " << optimal_weights[0] << "\n";
    std::cout << "Optimized Parameter x2 (Expected ~-5.0): " << optimal_weights[1] << "\n";
    return 0;
}

int dummy_loss() {
    // Let's optimize a 2x2 Weight Matrix for a dummy Neural Network Layer
    // Ideal target weights where Loss should equal 0 are:
    // W_target = [ 2.0,  1.5 ]
    //            [-1.0,  4.0 ]
    auto dummy_nn_loss = [](const Matrix<2, 2>& W) {
        double diff1 = W(0, 0) - 2.0;
        double diff2 = W(0, 1) - 1.5;
        double diff3 = W(1, 0) - (-1.0);
        double diff4 = W(1, 1) - 4.0;
        return (diff1 * diff1) + (diff2 * diff2) + (diff3 * diff3) + (diff4 * diff4);
    };

    // Initialize with random bad weights
    Matrix<2, 2> initial_weights{0.0, 0.0, 0.0, 0.0};

    // Run Gradient Descent over the Matrix
    Matrix<2, 2> optimized_W = matrix_gradient_descent<2, 2>(dummy_nn_loss, initial_weights);

    // Print trained Matrix weights
    std::cout << "--- Optimized Weights Matrix ---\n";

    optimized_W.print();

    return 0;
}

int main() {
    // --- REAL DATASET INITIALIZATION ---
    // X (3x2 Matrix): 3 houses, 2 features (Size divided by 100 for normalization, and Bedrooms)
    // House 1: 1.0 (100m2), 2 beds
    // House 2: 1.5 (150m2), 3 beds
    // House 3: 2.0 (200m2), 4 beds
    Matrix<3, 2> X{1.0, 2.0, 1.5, 3.0, 2.0, 4.0};

    // Y (3x1 Matrix): Real Target Prices (in hundreds of thousands USD)
    // House 1 Price: 5.0 ($500k)  -> (Formulated secretly as: 1.0 * 1.0 + 2.0 * 2.0 = 5.0)
    // House 2 Price: 7.5 ($750k)  -> (Formulated secretly as: 1.5 * 1.0 + 3.0 * 2.0 = 7.5)
    // House 3 Price: 10.0 ($1M)   -> (Formulated secretly as: 2.0 * 1.0 + 4.0 * 2.0 = 10.0)
    Matrix<3, 1> Y{5.0, 7.5, 10.0};

    // --- GRADIENT DESCENT TRAINING LOOP ---
    // Start with completely wrong initial weights: W = [0.0, 0.0]
    Matrix<2, 1> W{0.01f, 0.01f};

    double alpha  = 0.001;  // Learning Rate
    size_t epochs = 180;

    std::cout << "Starting Training Process...\n";

    for (size_t epoch = 0; epoch <= epochs; ++epoch) {
        // Create a lambda function wrapping our MSE calculation with fixed X and Y data
        auto loss_lambda = [&](const Matrix<2, 1>& current_W) { return compute_mse_loss<3, 2>(X, Y, current_W); };

        // Compute gradient vector for weights W (Dimension: 2x1)
        Matrix<2, 1> grad = compute_weight_gradient<3, 2>(loss_lambda, W);

        // Print loss progress every 10 epochs
        if (epoch % 10 == 0) {
            double current_loss = loss_lambda(W);
            std::cout << "Epoch " << epoch << " | Current MSE Loss: " << current_loss << "\n";
        }

        // Update step: W = W - alpha * Gradient
        W(0, 0) = W(0, 0) - (alpha * grad(0, 0));
        W(1, 0) = W(1, 0) - (alpha * grad(1, 0));
    }

    // --- EVALUATING TRAINED WEIGHTS ---
    std::cout << "\n--- Model Training Completed ---\n";
    std::cout << "Learned Weight for House Size (Expected ~1.0): " << W(0, 0) << "\n";
    std::cout << "Learned Weight for Bedrooms   (Expected ~2.0): " << W(1, 0) << "\n";

    return 0;
}