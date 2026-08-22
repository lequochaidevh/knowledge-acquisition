#include "../../../include/Tensor2D.h"

#include "../../../include/DenseLayer.h"
#include "../../../include/ReLULayer.h"
#include "../../../include/SigmoidLayer.h"
#include "../../../include/Sequential.h"

#include "../../../include/Loss.h"
#include "../../../include/Optimizer.h"

#include "../../../include/ModelCheckpoint.h"

// Helper function to automatically generate large-scale realistic customer datasets
void generate_customer_data(Tensor2D& input, Tensor2D& target, size_t num_samples) {
    std::random_device rd;
    std::mt19937       gen(rd());

    // Statistical profiles for Normal customers (Low-mid income, low frequency, new accounts)
    std::normal_distribution<float> normal_income(1.5f, 0.8f);
    std::normal_distribution<float> normal_freq(2.0f, 1.0f);
    std::normal_distribution<float> normal_age(0.8f, 0.4f);

    // Statistical profiles for VIP customers (High income, high frequency, mature accounts)
    std::normal_distribution<float> vip_income(5.5f, 1.2f);
    std::normal_distribution<float> vip_freq(8.0f, 1.5f);
    std::normal_distribution<float> vip_age(3.0f, 0.8f);

    size_t half_samples = num_samples / 2;

    for (size_t i = 0; i < num_samples; ++i) {
        if (i < half_samples) {
            // Generate synthetic Normal Customers (Class 0.0)
            input.at(i, 0)  = std::max(0.1f, normal_income(gen));  // Clamp to avoid negative values
            input.at(i, 1)  = std::max(0.1f, normal_freq(gen));
            input.at(i, 2)  = std::max(0.1f, normal_age(gen));
            target.at(i, 0) = 0.0f;
        } else {
            // Generate synthetic VIP Customers (Class 1.0)
            input.at(i, 0)  = std::max(0.1f, vip_income(gen));
            input.at(i, 1)  = std::max(0.1f, vip_freq(gen));
            input.at(i, 2)  = std::max(0.1f, vip_age(gen));
            target.at(i, 0) = 1.0f;
        }
    }
}

int main() {
    std::cout << "--- INITIALIZING DEEP LEARNING TRAINING PIPELINE ---\n\n";

    // 1. Scalable Data Setup: Allocating a big batch matrix directly
    const size_t TOTAL_SAMPLES = 200;  // Easily upscale this to 1000 or more
    Tensor2D     input(TOTAL_SAMPLES, 3);
    Tensor2D     target(TOTAL_SAMPLES, 1);

    // Automatically populate the tensors with simulated data patterns
    std::cout << "Generating " << TOTAL_SAMPLES << " synthetic client profiles...\n";
    generate_customer_data(input, target, TOTAL_SAMPLES);
    std::cout << "Data matrices successfully allocated and populated.\n\n";

    // 2. Network Construction using the Sequential wrapper
    // Architecture: 3 Inputs -> 14 Hidden Units (ReLU) -> 1 Output (Sigmoid Probability)
    Sequential model;
    model.add(new DenseLayer(3, 14));
    model.add(new ReLULayer());
    model.add(new DenseLayer(14, 1));
    model.add(new SigmoidLayer());

    // 3. Evaluation and Optimization criteria setup
    // MSELoss      criterion;
    BCELoss criterion;  // Using Binary Cross-Entropy Loss now
    // SGDOptimizer optimizer(0.01f);  // Learning rate = 0.01
    SGDOptimizer optimizer(0.006f);  // Learning rate = 0.01

    // 4. Execution Core: Main Training Loop
    const int TOTAL_EPOCHS = 1330;
    std::cout << "--- STARTING MODEL TRAINING ---\n";

    for (int epoch = 0; epoch < TOTAL_EPOCHS; ++epoch) {
        // Step A: Reset and clear accumulated gradients from previous iteration
        model.zero_grad();

        // Step B: Execution of Forward Pass through the whole layer stack
        Tensor2D pred = model.forward(input);

        // Step C: Calculate the loss metric
        float loss = criterion.forward(pred, target);

        // Log training telemetry information every 10 epochs
        if (epoch % 10 == 0 || epoch == TOTAL_EPOCHS - 1) {
            std::cout << "Epoch [" << epoch << "/" << TOTAL_EPOCHS - 1 << "] - Batch Loss: " << loss << "\n";
        }

        // Step D: Execution of Backward Pass (Compute gradients in reverse order)
        Tensor2D loss_grad = criterion.backward(pred, target);
        model.backward(loss_grad);

        // Step E: Apply gradient descent parameter updates to all layers
        // Note: Pass the model container directly so the optimizer can update its internal layers
        optimizer.step(model);
    }
    std::cout << "--- TRAINING LOOP SUCCESSFULLY COMPLETED ---\n\n";

    // 5. Serialization: Persist model weight parameters to a single checkpoint file
    std::cout << "Saving trained weights and biases to file...\n";
    try {
        model.save("mymodel.safetensors");
        std::cout << "Successfully exported architecture checkpoint: 'mymodel.safetensors'\n";
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: Failed to save model file: " << e.what() << "\n";
        return 1;
    }

    return 0;
}