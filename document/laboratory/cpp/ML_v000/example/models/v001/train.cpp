#include "../../../include/Tensor2D.h"

#include "../../../include/DenseLayer.h"
#include "../../../include/ReLULayer.h"
#include "../../../include/LeakyReLULayer.h"
#include "../../../include/SigmoidLayer.h"
#include "../../../include/Sequential.h"

#include "../../../include/Loss.h"
#include "../../../include/Optimizer.h"

#include "../../../include/ModelCheckpoint.h"
#include "../../../include/DataLoader.h"
#include "../../../include/DatasetUtils.h"

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
    const size_t TOTAL_SAMPLES = 2000;  // Easily upscale this to 1000 or more
    Tensor2D     global_input(TOTAL_SAMPLES, 3);
    Tensor2D     global_target(TOTAL_SAMPLES, 1);

    // Automatically populate the tensors with simulated data patterns
    std::cout << "Generating " << TOTAL_SAMPLES << " synthetic client profiles...\n";
    generate_customer_data(global_input, global_target, TOTAL_SAMPLES);
    std::cout << "Data matrices successfully allocated and populated.\n\n";

    // Segment data into independent subsets (80% train, 20% validation)
    Tensor2D train_input(0,0), train_target(0,0), val_input(0,0), val_target(0,0);
    DatasetUtils::train_test_split(global_input, global_target, train_input, train_target, val_input, val_target, 0.2f);
    std::cout << "Data split complete: Train samples = " << train_input.get_rows()
	    << " | Validation samples = " << val_input.get_rows() << "\n\n";
    
    // 2. Initialize the PyTorch-style DataLoader container
    // Configuration: Processing 200 rows in mini-batches of 32 rows each with active shuffling
    size_t     batch_size = 100;
    DataLoader dataloader(train_input, train_target, batch_size, true);

    // 3. Network Architecture Topology setup
    // Architecture: 3 Inputs -> 14 Hidden Units (ReLU) -> 1 Output (Sigmoid Probability)
    Sequential model;
    model.add(new DenseLayer(3, 14));
    model.add(new ReLULayer());
    // model.add(new LeakyReLULayer()); // same relu
    model.add(new DenseLayer(14, 1));
    model.add(new SigmoidLayer());

    // 3. Evaluation and Optimization criteria setup
    // MSELoss      criterion;
    BCELoss criterion;  // Using Binary Cross-Entropy Loss now
    // SGDOptimizer optimizer(0.01f);  // Learning rate = 0.01
    float        LEARNING_RATE = 0.006f;
    SGDOptimizer optimizer(LEARNING_RATE);  // Learning rate = 0.01

    // 4. Execution Core: Main Training Loop
    const int TOTAL_EPOCHS = 120;
    std::cout << "--- STARTING MODEL TRAINING ---\n";

    // Test Scenario A: Your failing stable freeze configuration (120 epochs, small updates)
    // Formula: (200 / 32) * 120 * 0.01 = 7.5 (Mathematical boundary is ok, but failed due to dead ReLU)
    HyperparameterAnalyzer::check_stability(200, 32, 0.01f, 120);

    // Test Scenario B: Your exploded NaN configuration (2000 epochs, high learning rate)
    // Formula: (200 / 32) * 2000 * 0.03 = 375.0 (Massive overshoot -> NaN alert)
    HyperparameterAnalyzer::check_stability(200, 32, 0.03f, 2000);

    // Test Scenario C: Your perfect convergence setup (Slowing step limits down)
    // Formula: (200 / 100) * 500 * 0.005 = 5.0 (Perfect balanced configuration)
    HyperparameterAnalyzer::check_stability(200, 100, 0.005f, 500);

    // Test Scenario D: Upscaling dataset simulation check (1500 samples)
    // Formula: (1500 / 250) * 400 * 0.002 = 4.8 (Stable and safe configuration)
    HyperparameterAnalyzer::check_stability(1500, 250, 0.002f, 400);

    HyperparameterAnalyzer::check_stability(TOTAL_SAMPLES, batch_size,  //
                                            LEARNING_RATE, TOTAL_EPOCHS);

    for (int epoch = 0; epoch < TOTAL_EPOCHS; ++epoch) {
        // Reset loader tracking indices and reshuffle rows sequence at each epoch boundary
        dataloader.reset();

        // Placeholders to pull data chunks out of the stream
        Tensor2D batch_inputs(0, 0);
        Tensor2D batch_targets(0, 0);

        // Inner Loop: Step through individual mini-batch pieces continuously
        while (dataloader.next_batch(batch_inputs, batch_targets)) {
            // Step A: Reset and wipe structural parameter gradient records
            model.zero_grad();
            float  epoch_cumulative_loss = 0.0f;
            size_t batch_counter         = 0;
            // Step B: Forward pass processing localized chunk dimensions (e.g., 32x3)
            Tensor2D pred = model.forward(batch_inputs);

            // Step C: Track and evaluate continuous performance loss metrics
            float loss = criterion.forward(pred, batch_targets);
            epoch_cumulative_loss += loss;
            batch_counter++;

            // Step D: Run reverse backpropagation passes over the local batch
            Tensor2D loss_grad = criterion.backward(pred, batch_targets);
            model.backward(loss_grad);

            // Log training telemetry information every 70 epochs
		// EVALUATION HOOK: Calculate performance and accuracy every 50 epochs
	       	if (epoch % 50 == 0 || epoch == TOTAL_EPOCHS - 1) {
	       	    float avg_train_loss = epoch_cumulative_loss / static_cast<float>(batch_counter);
	       	    
	       	    // CRITICAL STEP: Run a forward-only pass over the unseen Validation set (NO backward, NO step)
	       	    Tensor2D val_predictions = model.forward(val_input);
	       	    float val_loss = criterion.forward(val_predictions, val_target);
	       	    
	       	    // Calculate accuracy metric percentage over the unseen validation array
	       	    float val_accuracy = DatasetUtils::calculate_accuracy(val_predictions, val_target);
	
	       	    std::cout << "Epoch [" << epoch << "/" << TOTAL_EPOCHS - 1 
	       	              << "] | Train Loss: " << avg_train_loss 
	       	              << " | Val Loss: " << val_loss 
	       	              << " | Val Accuracy: " << val_accuracy << "%\n";
	       	}
            // Step E: Apply gradient descent parameter updates to all layers
            // Note: Pass the model container directly so the optimizer can update its internal layers
            optimizer.step(model);
        }
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
