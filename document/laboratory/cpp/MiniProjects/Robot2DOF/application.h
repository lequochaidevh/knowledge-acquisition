#pragma once
   
#include "pch.h"
#include "robot/2DOF/robot2Dof.h"
#include "gcode/GCodeParser.h"
#include "gcode/GCodeCommand.h"

// ---------- Globals for simple app ----------
static bool render_enabled = true;
static double time_acc = 0.0;
static bool show_target = false;
extern std::atomic<uint8_t> flag_impl; // 1: gcode; 2: manual
//---

extern std::vector<std::pair<double, double>> pathPoints;
extern bool drawPath; // toggle by D Key

// Pos
#define HOME_X -100
#define HOME_Y 0

static Robot2DOF robot(0.5, 0.5);