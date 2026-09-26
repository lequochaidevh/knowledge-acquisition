#pragma once

#include "IVioData.h"

namespace VIO {

enum class Direction { LEFT, RIGHT, UP, DOWN, STATIC };

class MotionEstimator {
 public:
    virtual Direction Process(IFrame& frame) = 0;

 private:
    IMat   prev;
    double prev_cx = 0, prev_cy = 0;
    bool   has_prev_center = false;
};
}  // namespace VIO