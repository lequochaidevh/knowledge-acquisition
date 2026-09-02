#include "../../include/Tensor2D.h"

#include "../../include/DenseLayer.h"
#include "../../include/ReLULayer.h"
#include "../../include/LeakyReLULayer.h"
#include "../../include/SigmoidLayer.h"
#include "../../include/Sequential.h"

#include "../../include/Loss.h"
#include "../../include/Optimizer.h"
#include "../../include/AdamOptimizer.h"

#include "../../include/ModelCheckpoint.h"
#include "../../include/DataLoader.h"
#include "../../include/DatasetUtils.h"

const size_t TOTAL_SAMPLES   = 1000;  // Easily upscale this to 1000 or more
const size_t CACHE_POOL_SIZE = TOTAL_SAMPLES;

struct DataCachePool {
    std::vector<float> normal_income;
    std::vector<float> normal_freq;
    std::vector<float> normal_age;

    std::vector<float> vip_income;
    std::vector<float> vip_freq;
    std::vector<float> vip_age;
};

DataCachePool initialize_global_cache() {
    DataCachePool cache;
    cache.normal_income.resize(CACHE_POOL_SIZE);
    cache.normal_freq.resize(CACHE_POOL_SIZE);
    cache.normal_age.resize(CACHE_POOL_SIZE);
    cache.vip_income.resize(CACHE_POOL_SIZE);
    cache.vip_freq.resize(CACHE_POOL_SIZE);
    cache.vip_age.resize(CACHE_POOL_SIZE);

    std::random_device rd;
    std::mt19937       gen(rd());

    std::normal_distribution<float> d_normal_income(1.5f, 0.8f);
    std::normal_distribution<float> d_normal_freq(2.0f, 1.0f);
    std::normal_distribution<float> d_normal_age(0.8f, 0.4f);

    std::normal_distribution<float> d_vip_income(5.5f, 1.2f);
    std::normal_distribution<float> d_vip_freq(8.0f, 1.5f);
    std::normal_distribution<float> d_vip_age(3.0f, 0.8f);

    for (size_t i = 0; i < CACHE_POOL_SIZE; ++i) {
        cache.normal_income[i] = std::max(0.1f, d_normal_income(gen));
        cache.normal_freq[i]   = std::max(0.1f, d_normal_freq(gen));
        cache.normal_age[i]    = std::max(0.1f, d_normal_age(gen));

        cache.vip_income[i] = std::max(0.1f, d_vip_income(gen));
        cache.vip_freq[i]   = std::max(0.1f, d_vip_freq(gen));
        cache.vip_age[i]    = std::max(0.1f, d_vip_age(gen));
    }
    return cache;
}
void generate_customer_data_fast(Tensor2D& input, Tensor2D& target, size_t num_samples, const DataCachePool& cache) {
    size_t half_samples = num_samples / 2;

    // RANDOM
    size_t start_offset = std::rand() % CACHE_POOL_SIZE;

    for (size_t i = 0; i < half_samples; ++i) {
        size_t idx = (start_offset + i) % CACHE_POOL_SIZE;

        input.at(i, 0)  = cache.normal_income[idx];
        input.at(i, 1)  = cache.normal_freq[idx];
        input.at(i, 2)  = cache.normal_age[idx];
        target.at(i, 0) = 0.0f;
    }

    for (size_t i = half_samples; i < num_samples; ++i) {
        size_t idx = (start_offset + i) % CACHE_POOL_SIZE;

        input.at(i, 0)  = cache.vip_income[idx];
        input.at(i, 1)  = cache.vip_freq[idx];
        input.at(i, 2)  = cache.vip_age[idx];
        target.at(i, 0) = 1.0f;
    }
}

int main() {
    std::cout << "--- INITIALIZING DEEP LEARNING TRAINING PIPELINE ---\n\n";

    // 1. Scalable Data Setup: Allocating a big batch matrix directly
    Tensor2D      global_input(TOTAL_SAMPLES, 3);
    Tensor2D      global_target(TOTAL_SAMPLES, 1);
    DataCachePool global_cache = initialize_global_cache();
    // Automatically populate the tensors with simulated data patterns
    std::cout << "Generating " << TOTAL_SAMPLES << " synthetic client profiles...\n";
    generate_customer_data_fast(global_input, global_target, TOTAL_SAMPLES, global_cache);
    std::cout << "Data matrices successfully allocated and populated.\n\n";

    // Segment data into independent subsets (80% train, 20% validation)
    Tensor2D train_input(0, 0), train_target(0, 0), val_input(0, 0), val_target(0, 0);
    DatasetUtils::train_test_split(global_input, global_target, train_input, train_target, val_input, val_target, 0.2f);
    std::cout << "Data split complete: Train samples = " << train_input.get_rows()
              << " | Validation samples = " << val_input.get_rows() << "\n\n";

    // 2. Initialize the PyTorch-style DataLoader container
    // Configuration: Processing 200 rows in mini-batches of 32 rows each with active shuffling
    size_t     batch_size = 200;
    DataLoader dataloader(train_input, train_target, batch_size, true);

    // 3. Network Architecture Topology setup
    // Architecture: 3 Inputs -> 9 Hidden Units (ReLU) -> 1 Output (Sigmoid Probability)
    Sequential model;
    model.add(new DenseLayer(3, 9, "relu"));
    model.add(new ReLULayer());
    // model.add(new LeakyReLULayer()); // same relu
    model.add(new DenseLayer(9, 1, "sigmoid"));
    model.add(new SigmoidLayer());

    // 3. Evaluation and Optimization criteria setup
    MSELoss criterion;
    // BCELoss criterion;  // Using Binary Cross-Entropy Loss now
    float LEARNING_RATE = 0.16f;
    // SGDOptimizer optimizer(LEARNING_RATE);  // Learning rate = 0.01
    AdamOptimizer optimizer(LEARNING_RATE);
    // 4. Execution Core: Main Training Loop
    const int TOTAL_EPOCHS = 260;
    std::cout << "--- STARTING MODEL TRAINING ---\n";

    for (int epoch = 0; epoch < TOTAL_EPOCHS; ++epoch) {
        // Reset loader tracking indices and reshuffle rows sequence at each epoch boundary
        dataloader.reset();

        // Placeholders to pull data chunks out of the stream
        Tensor2D batch_inputs(0, 0);
        Tensor2D batch_targets(0, 0);

        float    epoch_cumulative_loss = 0.0f;
        size_t   batch_counter         = 0;
        float    loss                  = 100;
        Tensor2D pred;
        // Inner Loop: Step through individual mini-batch pieces continuously
        while (dataloader.next_batch(batch_inputs, batch_targets)) {
            // Step A: Reset and wipe structural parameter gradient records
            model.zero_grad();
            epoch_cumulative_loss = 0.0f;
            batch_counter         = 0;
            // Step B: Forward pass processing localized chunk dimensions (e.g., 32x3)
            pred = std::move(model.forward(batch_inputs));

            // Step C: Track and evaluate continuous performance loss metrics
            loss = std::move(criterion.forward(pred, batch_targets));

            epoch_cumulative_loss += loss;
            batch_counter++;

            // Step D: Run reverse backpropagation passes over the local batch
            Tensor2D loss_grad = std::move(criterion.backward(pred, batch_targets));
            model.backward(loss_grad);

            // Step E: Apply gradient descent parameter updates to all layers
            // Note: Pass the model container directly so the optimizer can update its internal layers
            optimizer.step(model);
        }

        // Log training telemetry information every 70 epochs
        // EVALUATION HOOK: Calculate performance and accuracy every 50 epochs
        if (epoch % 10 == 0 || epoch == TOTAL_EPOCHS - 1) {
            float avg_train_loss = epoch_cumulative_loss / static_cast<float>(batch_counter);

            // CRITICAL STEP: Run a forward-only pass over the unseen Validation set (NO backward, NO step)
            Tensor2D val_predictions = model.forward(val_input);
            // Calculate accuracy metric percentage over the unseen validation array
            float val_accuracy = DatasetUtils::calculate_accuracy(val_predictions, val_target);

            std::cout << "Epoch [" << epoch << "/" << TOTAL_EPOCHS - 1 << "] | Train Loss: " << avg_train_loss
                      << " | Val Loss: " << loss << " | Val Accuracy: " << val_accuracy << "%\n";

            if (loss < 0.000001) break;  // MSELoss only
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
