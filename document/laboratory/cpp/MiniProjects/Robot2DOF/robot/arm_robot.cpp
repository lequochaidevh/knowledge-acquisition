#include "arm_robot.h"
#include "application.h"  //for pathPoints

// ---------- Robot class ----------
// ArmRobot::ArmRobot(double _l1, double _l2)
//     : l1(_l1), l2(_l2), theta1(0), theta2(0) {
//     UpdatePosition();
// }

// --- Update mx, my base on theta1, theta2 ---
void ArmRobot::UpdatePosition() {
    mx = l1 * cos(theta1) + l2 * cos(theta1 + theta2);
    my = l1 * sin(theta1) + l2 * sin(theta1 + theta2);
    // std::cout << "mX = " << mx << " ," << "mY = " << my << std::endl;

    // Save the path.
    pathPoints.emplace_back(mx, my);
}

// --- Forward kinematics ---
std::vector<Mat4> ArmRobot::forwardKinematics(double th1, double th2) const {
    Mat4 T0  = Mat4();  // base
    Mat4 T1  = Mat4::rotZ(th1) * Mat4::translate(l1, 0, 0);
    Mat4 T2  = Mat4::rotZ(th2) * Mat4::translate(l2, 0, 0);
    Mat4 T01 = T0 * Mat4::rotZ(th1) * Mat4::translate(l1, 0, 0);
    Mat4 T02 = T01 * Mat4::rotZ(th2) * Mat4::translate(l2, 0, 0);
    return {T0, T01, T02};
}

// Convenience: get end-effector pos
Vec2 ArmRobot::endEffector(double th1, double th2) const {
    double x = l1 * cos(th1) + l2 * cos(th1 + th2);
    double y = l1 * sin(th1) + l2 * sin(th1 + th2);
    return Vec2(x, y);
}

// Jacobian 2x2 (filled into array J[2][2])
void ArmRobot::jacobian(double th1, double th2, double J[2][2]) const {
    double s1 = sin(th1), c1 = cos(th1);
    double s12 = sin(th1 + th2), c12 = cos(th1 + th2);
    J[0][0] = -l1 * s1 - l2 * s12;
    J[0][1] = -l2 * s12;
    J[1][0] = l1 * c1 + l2 * c12;
    J[1][1] = l2 * c12;
}

// --- SolX IK (inverse) ---
std::vector<IKResult> ArmRobot::inverseKinematics(double x, double y) const {
    std::vector<IKResult> sols;
    double                r2      = x * x + y * y;
    double                cos_th2 = (r2 - l1 * l1 - l2 * l2) / (2.0 * l1 * l2);
    if (cos_th2 < -1.0 - 1e-12 || cos_th2 > 1.0 + 1e-12) return sols;
    if (cos_th2 > 1.0) cos_th2 = 1.0;
    if (cos_th2 < -1.0) cos_th2 = -1.0;
    double sin_th2_pos = sqrt(std::max(0.0, 1.0 - cos_th2 * cos_th2));
    double sin_th2_neg = -sin_th2_pos;

    auto make_sol = [&](double s2, double c2) {
        double th2 = atan2(s2, c2);
        double k1  = l1 + l2 * c2;
        double k2  = l2 * s2;
        double th1 = atan2(y, x) - atan2(k2, k1);
        return IKResult{th1, th2};
    };

    sols.push_back(make_sol(sin_th2_pos, cos_th2));
    if (fabs(sin_th2_pos - sin_th2_neg) > 1e-9) sols.push_back(make_sol(sin_th2_neg, cos_th2));
    return sols;
}

// --- Getter Position ---
double ArmRobot::GetCurrentX() const { return mx; }
double ArmRobot::GetCurrentY() const { return my; }

double ArmRobot::GetCurrentX_Work() const { return mx - offsetX; }
double ArmRobot::GetCurrentY_Work() const { return my - offsetY; }

double ArmRobot::GetCurrentX_Machine() const { return mx; }
double ArmRobot::GetCurrentY_Machine() const { return my; }

// --- robot move to Position (x, y) ---
void ArmRobot::MoveTo(double x, double y) {
    double d = sqrt(x * x + y * y);
    if (d > l1 + l2) {
        std::cerr << "Target d > l1 + l2 unreachable: (" << d << "," << x << "," << y << ")\n";
        return;
    }

    if (d < fabs(l1 - l2)) {
        std::cerr << "Target d < fabs(l1 - l2) unreachable: (" << d << "," << x << "," << y << ")\n";
        return;
    }

    double cos_t2 = (x * x + y * y - l1 * l1 - l2 * l2) / (2 * l1 * l2);
    double sin_t2 = sqrt(1 - cos_t2 * cos_t2);
    theta2        = atan2(sin_t2, cos_t2);
    theta1        = atan2(y, x) - atan2(l2 * sin_t2, l1 + l2 * cos_t2);
    UpdatePosition();  // Update mx, my
}

void ArmRobot::SetWorkOffset(double wx, double wy) {
    offsetX = mx - wx / 100;
    offsetY = my - wy / 100;
    printf("offsetX = %f, offsetY = %f \n", offsetX, offsetY);
}
