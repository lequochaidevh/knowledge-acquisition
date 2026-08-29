#pragma once
#include "Tensor2D.h"

class DatasetUtils {
public:
	// Core function to partition o unified dataset into Train
	// and Validation subsets
	static void train_test_split(const Tensor2D& inputs, const Tensor2D& targets,
			Tensor2D& train_in, Tensor2D& train_tgt,
			Tensor2D& val_in, Tensor2D& val_tgt,
			float val_ratio = 0.2f) {
		size_t total_samples = inputs.get_rows();
		size_t num_features = inputs.get_cols();
		size_t num_targets = targets.get_cols();

		// 1. Calculate split dimension boundaries
		size_t val_samples = static_cast<size_t>(static_cast<float>(total_samples) * val_ratio);
		size_t train_samples = total_samples - val_samples;

		// 2. Initialize and randomize index sequence array mapping
		std::vector<size_t> indices(total_samples);
		std::iota(indices.begin(), indices.end(), 0);
		std::random_device rd;
		std::mt19937 gen(rd());
		std::shuffle(indices.begin(), indices.end(), gen);

		// 3. Allocate memory dimensions for all four subset tensor matrices
		train_in = Tensor2D(train_samples, num_features);
		train_tgt = Tensor2D(train_samples, num_targets);
		val_in = Tensor2D(val_samples, num_features);
		val_tgt =Tensor2D(val_samples,num_targets);
		
		// 4. Hydrate Train Matrix structures
		for(size_t i=0; i < train_samples; ++i) {
			size_t idx = indices[i];
			for(size_t j=0; j<num_features; ++j) train_in.at(i, j) = inputs.at(idx, j);
			for(size_t j=0; j<num_targets; ++j) train_tgt.at(i, j) = targets.at(idx, j);
		}

		// 5. Hydrate Validation Matrix structures
		for(size_t i=0; i<val_samples; ++i) {
			size_t idx = indices[train_samples + i]; // Offset pointer forward past train samples block
			for(size_t j=0; j<num_features; ++j) val_in.at(i, j) = inputs.at(idx, j);
			for(size_t j=0; j<num_targets; ++j) val_tgt.at(i, j) = targets.at(idx, j);
		}
	}

	// Core metric evaluator function to calculate classification success accuracy percentages
	static float calculate_accuracy(const Tensor2D& pred, const Tensor2D& target, float threshold = 0.5f) {
		size_t rows = pred.get_rows();
		size_t correct_predictions = 0;

		for(size_t i=0; i<rows; ++i) {
			// Apply standard boundary rule logic mapping (close to 1.0f vs close to 0,0f)
			float predicted_class = (pred.at(i, 0) >= threshold) ? 1.0f : 0.0f;
			float  actual_class = target.at(i, 0);

			if (predicted_class == actual_class) {
				correct_predictions++;
			}
		}
		// Return scaling ratio output ranging perfectly between [0.0f -> 100.0f]
		return (static_cast<float>(correct_predictions) / static_cast<float>(rows)) * 100.0f;
	}
};	

			

