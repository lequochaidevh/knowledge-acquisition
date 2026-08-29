#pragma once

#include "Tensor2D.h"

class ModelCheckpoint {
 private:
    std::map<std::string, Tensor2D*> state_dict;

 public:
    // Register a layer parameter into the dictionary
    void register_parameter(const std::string& name, Tensor2D& parameter) { state_dict[name] = &parameter; }

    // Save all registered parameters into a single binary file
    void save(const std::string& filepath) {
        std::ofstream os(filepath, std::ios::binary);
        if (!os.is_open()) {
            throw std::runtime_error("Failed to open file for saving model.");
        }

        // 1. Write the total number of tensor keys
        size_t num_tensors = state_dict.size();
        os.write(reinterpret_cast<const char*>(&num_tensors), sizeof(num_tensors));

        // 2. Serialize each tensor entry (Key String Length -> Key String -> Tensor Binary)
        for (const auto& [name, tensor_ptr] : state_dict) {
            size_t name_len = name.size();
            os.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            os.write(name.data(), name_len);

            // Serialize tensor shape and structural data block
            tensor_ptr->write_binary(os);
        }
        os.close();
    }

    // Load weights from a single binary file back into registered parameters
    void load(const std::string& filepath) {
        std::ifstream is(filepath, std::ios::binary);
        if (!is.is_open()) {
            throw std::runtime_error("Failed to open model checkpoint file.");
        }

        // 1. Read total tensor count
        size_t num_tensors;
        is.read(reinterpret_cast<char*>(&num_tensors), sizeof(num_tensors));

        // 2. Deserialize key and parse data maps
        for (size_t i = 0; i < num_tensors; ++i) {
            size_t name_len;
            is.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));

            std::string name(name_len, ' ');
            is.read(&name[0], name_len);

            // Locate registered reference target by Key ID
            auto it = state_dict.find(name);
            if (it != state_dict.end()) {
                // Load binary chunk into target parameter matrix buffer
                it->second->read_binary(is);
            } else {
                // Skip unknown tensor payload safely if model architecture altered
                size_t skip_rows, skip_cols;
                is.read(reinterpret_cast<char*>(&skip_rows), sizeof(skip_rows));
                is.read(reinterpret_cast<char*>(&skip_cols), sizeof(skip_cols));
                is.seekg(skip_rows * skip_cols * sizeof(float), std::ios::cur);
            }
        }
        is.close();
    }
};