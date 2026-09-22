#pragma once
#include <string>
#include <vector>
#include "GCodeCommand.h"

class GCodeParser {
 public:
    std::vector<GCodeCommand> Parse(const std::string& path);
};