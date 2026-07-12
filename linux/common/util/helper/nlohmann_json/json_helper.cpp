#include "json_helper.h"

bool JsonHelper::load_from_file(const std::string& file_path, nlohmann::json& root) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;  // File does not exist or cannot be opened
    }

    try {
        // Attempt to parse the file content into the JSON object
        root = nlohmann::json::parse(file);
        return true;  // Successfully parsed
    } catch (const nlohmann::json::parse_error& e) {
        // Catch specific JSON syntax errors (like the parse_error.101 you encountered)
        std::cerr << "[JsonHelper] Parse error: " << e.what() << std::endl;
        return false;  // Return false instead of crashing the program
    } catch (const std::exception& e) {
        // Catch any other standard exceptions (e.g., I/O issues)
        std::cerr << "[JsonHelper] Standard exception: " << e.what() << std::endl;
        return false;
    }
}

bool JsonHelper::save_to_file(const std::string& file_path, const nlohmann::json& root, bool prettify) {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    try {
        // Pass 4 for indentation, -1 for minified one-liner json
        file << root.dump(prettify ? 4 : -1);
        return true;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[JsonHelper] Save error: " << e.what() << std::endl;
        return false;
    }
}

bool JsonHelper::save_to_file_ordered(const std::string& file_path, const nlohmann::json& root, bool prettify) {
    nlohmann::ordered_json sorted_output;

    // STEP 1: READ BASELINE (Do this before opening the write stream to avoid file truncation)
    std::ifstream base_file(file_path);
    if (base_file.is_open()) {
        try {
            sorted_output = nlohmann::ordered_json::parse(base_file);
        } catch (...) {
            sorted_output = nlohmann::ordered_json::object();
        }
        base_file.close();
    } else {
        // ANSI escape codes: \033[1;31m targets BOLD RED, \033[0m RESETS terminal color
        // Triggers a highly visible warning log on Ubuntu terminal if the base file is missing
        std::cerr << "\033[1;31m[JsonHelper] Warning: "
                     "Base file does not exist at path: ";
        std::cerr << file_path << "\033[0m" << std::endl;

        // Initialize as an empty object since no baseline template was found to compare order
        sorted_output = nlohmann::ordered_json::object();
    }

    // STEP 2: LOGICAL MERGE (Preserve baseline order, update values, push fresh keys to the bottom)
    // Scenario A: Update existing keys in their original baseline position
    for (auto& [key, value] : sorted_output.items()) {
        if (root.contains(key)) {
            sorted_output[key] = root[key];  // Sync updated memory values into the original line slot
        }
    }

    // Scenario B: Detect brand new keys (like those from append_back) and force them to the bottom
    for (auto& [key, value] : root.items()) {
        if (!sorted_output.contains(key)) {
            sorted_output[key] = value;  // Naturally appends at the absolute bottom of ordered_json
        }
    }

    // STEP 3: WRITE BACK TO DISK
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return false;
    }

    try {
        if (prettify) {
            file << sorted_output.dump(4);
        } else {
            file << sorted_output.dump();
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[JsonHelper] Order-preserving write error: " << e.what() << std::endl;
        return false;
    }
}

std::string JsonHelper::get_string(const nlohmann::json& node, const std::string& key, const std::string& default_val) {
    try {
        if (node.contains(key) && node[key].is_string()) {
            return node[key].get<std::string>();
        }
    } catch (...) {
        // Catch-all block to prevent crashes from any edge-case exceptions
    }
    return default_val;
}

int JsonHelper::get_int(const nlohmann::json& node, const std::string& key, int default_val) {
    try {
        if (node.contains(key) && node[key].is_number_integer()) {
            return node[key].get<int>();
        }
    } catch (...) {
    }
    return default_val;
}

bool JsonHelper::get_bool(const nlohmann::json& node, const std::string& key, bool default_val) {
    try {
        if (node.contains(key) && node[key].is_boolean()) {
            return node[key].get<bool>();
        }
    } catch (...) {
    }
    return default_val;
}

nlohmann::json JsonHelper::get_node(const nlohmann::json& node, const std::string& key) {
    try {
        if (node.contains(key) && (node[key].is_object() || node[key].is_array())) {
            return node[key];
        }
    } catch (...) {
    }
    // Return an empty object instead of null to prevent crash on chained operations
    return nlohmann::json::object();
}

bool JsonHelper::append_back(nlohmann::json& node, const std::string& key, const nlohmann::json& value) {
    // If the key doesn't exist, initialize it as an empty JSON array
    if (!node.contains(key)) {
        node[key] = nlohmann::json::array();
    }

    // Safety check: ensure we are operating on an array, not an object or primitive
    if (!node[key].is_array()) {
        return false;
    }

    node[key].push_back(value);
    return true;
}

bool JsonHelper::append_front(nlohmann::json& node, const std::string& key, const nlohmann::json& value) {
    if (!node.contains(key) || !node[key].is_array()) {
        return false;
    }

    // Insert at the begin iterator to place it at the front (index 0)
    node[key].insert(node[key].begin(), value);
    return true;
}

bool JsonHelper::insert_at(nlohmann::json& node, const std::string& key, size_t index, const nlohmann::json& value) {
    if (!node.contains(key) || !node[key].is_array()) {
        return false;
    }

    // Boundary check: ensure index does not exceed array bounds
    if (index > node[key].size()) {
        return false;
    }

    node[key].insert(node[key].begin() + index, value);
    return true;
}
