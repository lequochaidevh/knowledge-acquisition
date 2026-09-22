#pragma once

#include "mathHelper.h"

// ---------- Robot class ----------
struct IKResult {
    double theta1, theta2;
};

class Robot2DOF {
 public:
    double l1, l2;
    double theta1, theta2;  // (rad)
    double mx, my;          // arm-destination (end-effector)
    double offsetX = 0.0;
    double offsetY = 0.0;
    // Todo: Add singleton patter
    Robot2DOF(double _l1, double _l2) : l1(_l1), l2(_l2), theta1(0), theta2(0) { UpdatePosition(); }

    virtual ~Robot2DOF() = default;

    // --- Update mx, my base on theta1, theta2 ---
    void UpdatePosition();

    // --- Forward kinematics ---
    std::vector<Mat4> forwardKinematics(double th1, double th2) const;

    // Convenience: get end-effector pos
    Vec2 endEffector(double th1, double th2) const;

    // Jacobian 2x2 (filled into array J[2][2])
    void jacobian(double th1, double th2, double J[2][2]) const;

    // --- Find Solx IK (inverse) ---
    std::vector<IKResult> inverseKinematics(double x, double y) const;

    // --- Getter Position ---
    double GetCurrentX() const;
    double GetCurrentY() const;

    double GetCurrentX_Work() const;
    double GetCurrentY_Work() const;

    double GetCurrentX_Machine() const;
    double GetCurrentY_Machine() const;

    // --- Robot move to Position (x, y) ---
    void MoveTo(double x, double y);

    void SetWorkOffset(double wx, double wy);

    void moveLinearSmooth(double tx, double ty, double feedrate, double dt);
};