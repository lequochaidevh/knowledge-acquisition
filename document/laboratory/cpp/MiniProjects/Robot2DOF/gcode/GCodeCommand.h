#pragma once
#include <vector>
#include <string>

enum class GCodeType { G0, G1, Unknown };
class Robot2DOF;
struct GCodeCommand {
    // GCodeType type;
    std::string type;  // G0, G1, G3, G4, ...
    double      x         = 0;
    double      y         = 0;
    double      f         = 1.0;    // feedrate (speed)
    double      i         = 0.0;    // offset X for G3/G2 (if need)
    double      j         = 0.0;    // offset Y for G3/G2
    double      feedrate  = 100.0;  // speed mm/s
    double      dwellTime = 0.0;    // G4

    bool hasX = true;
    bool hasY = true;
};

void ExecuteGCodeStep(Robot2DOF& robot, const std::vector<GCodeCommand>& cmds, double dt);