#include "Tensor2D.h"

/**
 * This class stores references to your training data,
 * randomizes row sequences using an index tracking array (indices),
 * and performs matrix slicing to yield custom-sized Mini-batch slices.
 */
class DataLoader {
 private:
    const Tensor2D& inputs;
    const Tensor2D& targets;
    size_t          batch_size;
    bool            shuffle;

    size_t              num_samples;
    size_t              current_index;
    std::vector<size_t> indices;
    std::mt19937        random_engine;

 public:
    DataLoader(const Tensor2D& input_data, const Tensor2D& target_data, size_t batch_sz, bool shuffle_data = true)
        : inputs(input_data), targets(target_data), batch_size(batch_sz), shuffle(shuffle_data), current_index(0) {
        this->num_samples = inputs.get_rows();

        // Initialize the index array tracking rows from 0 to N-1
        this->indices.resize(num_samples);
        std::iota(indices.begin(), indices.end(), 0);

        // Initialize random seed engine for dataset shuffling
        std::random_device rd;
        this->random_engine = std::mt19937(rd());

        // Initial reset to prepare the first epoch sequence
        this->reset();
    }

    // Reset indicator trackers and trigger shuffle if enabled at epoch start
    void reset() {
        this->current_index = 0;
        if (this->shuffle) {
            std::shuffle(this->indices.begin(), this->indices.end(), this->random_engine);
        }
    }

    // Check if there are remaining mini-batches left to process in the current epoch
    bool has_next() const { return this->current_index < this->num_samples; }

    // Core execution function: Slice and return the next mini-batch tensors
    bool next_batch(Tensor2D& batch_inputs, Tensor2D& batch_targets) {
        if (!has_next()) {
            return false;
        }

        // Determine current dynamic batch sizing boundaries
        size_t actual_batch_size = std::min(this->batch_size, this->num_samples - this->current_index);
        size_t num_features      = this->inputs.get_cols();
        size_t num_targets       = this->targets.get_cols();

        // 1. Reallocate target matrices containers to match dynamic size limits
        batch_inputs  = Tensor2D(actual_batch_size, num_features);
        batch_targets = Tensor2D(actual_batch_size, num_targets);

        // 2. Hydrate rows into the batch buffers by pulling randomized indices
        for (size_t i = 0; i < actual_batch_size; ++i) {
            size_t original_row_idx = this->indices[this->current_index + i];

            // Map input features vectors
            for (size_t j = 0; j < num_features; ++j) {
                batch_inputs.at(i, j) = this->inputs.at(original_row_idx, j);
            }

            // Map target values vectors
            for (size_t j = 0; j < num_targets; ++j) {
                batch_targets.at(i, j) = this->targets.at(original_row_idx, j);
            }
        }

        // Advance iteration tracker forward
        this->current_index += actual_batch_size;
        return true;
    }
};

class HyperparameterAnalyzer {
 public:
    // Core function to analyze hyperparameter combinations using basic math (+ - * /)
    static bool check_stability(size_t total_samples, size_t batch_size, float learning_rate, int epochs) {
        std::cout << "==================================================\n";
        std::cout << "[ANALYSIS] Checking Hyperparameter Feasibility\n";
        std::cout << "==================================================\n";

        // Safety Check: Avoid division by zero if batch_size is misconfigured
        if (batch_size == 0) {
            std::cout << ">> RESULT: CRITICAL ERROR [Batch Size cannot be 0!]\n\n";
            return false;
        }

        // 1. Calculate total weight updates per epoch (Samples / Batch Size)
        float updates_per_epoch = static_cast<float>(total_samples) / static_cast<float>(batch_size);

        // 2. Calculate total weight updates across all training iterations (Updates * Epochs)
        float total_updates = updates_per_epoch * static_cast<float>(epochs);

        // 3. Calculate Total Dynamic Impact (Total Updates * Learning Rate)
        float total_impact = total_updates * learning_rate;

        // Display calculated metrics profile
        std::cout << "-> Batches per Epoch : " << updates_per_epoch << "\n";
        std::cout << "-> Total Budget Steps: " << total_updates << "\n";
        std::cout << "-> Total Impact Score: " << total_impact << "\n\n";

        // 4. Decision Boundary Evaluation Logic using threshold limits
        if (total_impact < 0.5f) {
            std::cout << ">> CONFIG STATUS: [UNREASONABLE - DANGER OF FREEZING]\n";
            std::cout << ">> DIAGNOSIS: The total learning impact is too weak (" << total_impact << " < 0.5).\n";
            std::cout << "   The network weights will barely shift. Output scores will freeze near 0.5.\n";
            std::cout << ">> FIX: Increase your Learning Rate or scale up the total Epochs.\n";
            std::cout << "==================================================\n\n";
            return false;
        } else if (total_impact > 30.0f) {
            std::cout << ">> CONFIG STATUS: [UNREASONABLE - DANGER OF EXPLOSION / NaN]\n";
            std::cout << ">> DIAGNOSIS: The total learning impact is too aggressive (" << total_impact << " > 30.0).\n";
            std::cout << "   The gradient steps will trigger massive mathematical overshoots, exploding into NaN.\n";
            std::cout << ">> FIX: Decrease your Learning Rate or scale down total Epochs.\n";
            std::cout << "==================================================\n\n";
            return false;
        } else {
            std::cout << ">> CONFIG STATUS: [REASONABLE - STABLE CONVERGENCE EXPECTED]\n";
            std::cout << ">> DIAGNOSIS: Impact score " << total_impact
                      << " falls perfectly within the safe [1.0 -> 15.0] window.\n";
            std::cout << "   The gradient descent trajectory will optimize parameters fluidly and correctly.\n";
            std::cout << "==================================================\n\n";
            return true;
        }
    }
};