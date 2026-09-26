#pragma once

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "AppConfig.h"

// Because Json parse lib not take many CPU
//  No need support more API intergrate.
using json = nlohmann::json;
namespace VIO {
class ConfigLoader {
 public:
    static AppConfig Load(const std::string& path) {
        AppConfig cfg;

        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open config file: " + path);
        }

        json root;
        file >> root;

        // Safe parsing with default fallback
        cfg.camera_id = root.value("camera_id", 0);
        cfg.width     = root.value("width", 640);
        cfg.height    = root.value("height", 480);
        cfg.fps       = root.value("fps", 30);
        cfg.enable_ui = root.value("enable_ui", true);

        return cfg;
    }
};
}  // namespace VIO