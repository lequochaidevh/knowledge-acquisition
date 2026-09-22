#include "GCodeParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

std::vector<GCodeCommand> GCodeParser::Parse(const std::string& path) {
    std::vector<GCodeCommand> cmds;
    std::ifstream             f(path);
    if (!f.is_open()) {
        std::cerr << "Cannot open GCode file: " << path << std::endl;
        return cmds;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;

        // Ignore Gcode line with comment: ';' or '('
        size_t commentPos = line.find_first_of(";(");
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);

        std::istringstream iss(line);
        std::string        token;
        GCodeCommand       cmd;

        // First token G-code (G0, G1, G3, G4,...)
        iss >> token;
        if (token.empty()) continue;
        cmd.type = token;  // G0, G1, G2, G3, G4...

        // Param: X, Y, I, J, F, P...
        while (iss >> token) {
            if (token.size() < 2) continue;
            char   c   = token[0];
            double val = atof(token.substr(1).c_str());

            switch (c) {
                case 'X':
                    cmd.x = val;
                    break;
                case 'Y':
                    cmd.y = val;
                    break;
                case 'I':
                    cmd.i = val;
                    break;  // offset for G3/G2
                case 'J':
                    cmd.j = val;
                    break;
                case 'F':
                    cmd.feedrate = val;
                    break;  // speed
                case 'P':
                    cmd.dwellTime = val;
                    break;  // Time spause at G4
                default:
                    break;
            }
        }

        cmds.push_back(cmd);
    }

    f.close();
    return cmds;
}
