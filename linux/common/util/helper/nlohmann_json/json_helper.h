#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

class JsonHelper {
 public:
    /**
     * @brief Loads and parses a JSON file from the disk.
     * @param file_path Path to the JSON file.
     * @param root Reference to the json object to store the parsed data.
     * @return true if successful, false otherwise.
     */
    static bool load_from_file(const std::string& file_path, nlohmann::json& root);

    /**
     * @brief Saves the JSON structure to a file on the disk.
     * @param file_path Path where the file should be saved.
     * @param root The json object containing the data to save.
     * @param prettify If true, formats the JSON with indentations.
     * @return true if successful, false otherwise.
     */
    static bool save_to_file(const std::string& file_path, const nlohmann::json& root, bool prettify = true);

    static bool save_to_file_ordered(const std::string& file_path, const nlohmann::json& root, bool prettify = true);

    /**
     * @brief Safely extracts a string value from a JSON node.
     * @param node The parent JSON node.
     * @param key The key to look for.
     * @param default_val Value to return if the key is missing or not a string.
     * @return The extracted string or default_val.
     */
    static std::string get_string(const nlohmann::json& node, const std::string& key,
                                  const std::string& default_val = "");

    /**
     * @brief Safely extracts an integer value from a JSON node.
     * @param node The parent JSON node.
     * @param key The key to look for.
     * @param default_val Value to return if the key is missing or not an integer.
     * @return The extracted int or default_val.
     */
    static int get_int(const nlohmann::json& node, const std::string& key, int default_val = 0);

    /**
     * @brief Safely extracts a boolean value from a JSON node.
     * @param node The parent JSON node.
     * @param key The key to look for.
     * @param default_val Value to return if the key is missing or not a boolean.
     * @return The extracted bool or default_val.
     */
    static bool get_bool(const nlohmann::json& node, const std::string& key, bool default_val = false);

    /**
     * @brief Safely extracts a child node (Object or Array).
     * @param node The parent JSON node.
     * @param key The key to look for.
     * @return The child JSON node, or an empty JSON object if not found.
     */
    static nlohmann::json get_node(const nlohmann::json& node, const std::string& key);

    /**
     * @brief Appends a value to the END of a JSON array. Creates the array if missing.
     * @return true if successful, false if the key exists but is not an array.
     */
    static bool append_back(nlohmann::json& node, const std::string& key, const nlohmann::json& value);

    /**
     * @brief Inserts a value to the BEGINNING (front) of a JSON array.
     * @return true if successful, false if the key is not an array.
     */
    static bool append_front(nlohmann::json& node, const std::string& key, const nlohmann::json& value);

    /**
     * @brief Inserts a value at a SPECIFIC INDEX of a JSON array.
     * @return true if successful, false if index is out of bounds or not an array.
     */
    static bool insert_at(nlohmann::json& node, const std::string& key, size_t index, const nlohmann::json& value);
};
