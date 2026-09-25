#include "GCodeCommand.h"
#include "../application.h"  // ArmRobot
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

static void moveLinearSmooth(ArmRobot& robot, double tx, double ty, double feedrate, double dt);
static void moveLinear(ArmRobot& robot, double targetX, double targetY, double feedrate, double dt);
static void moveArcCW(ArmRobot& robot, double cx, double cy, double targetX, double targetY, double feedrate,
                      double dt);
static void moveArcCCW(ArmRobot& robot, double cx, double cy, double targetX, double targetY, double feedrate,
                       double dt);
static void dwell(double seconds);
static bool absoluteMode = true;
// TODO: Declare scaler => 100 and flexiable
// Execute G-code every line
void ExecuteGCodeStep(ArmRobot& robot, const std::vector<GCodeCommand>& cmds, double dt) {
    for (auto& cmd : cmds) {
        if (flag_impl.load() != 1) break;
        if (cmd.type == "G0" || cmd.type == "G1") {
            double tx = cmd.x;
            double ty = cmd.y;
            if (absoluteMode) {
                tx += (robot.offsetX * 100);  // offset = 0 or offset of G92
                ty += (robot.offsetY * 100);
                printf("tx = %f, ty = %f \n", tx, ty);
            } else {
                tx += (robot.GetCurrentX_Machine() * 100);  // G91
                ty += (robot.GetCurrentY_Machine() * 100);
            }
            // moveLinear(robot, tx, ty, cmd.feedrate, dt);
            moveLinearSmooth(robot, tx, ty, cmd.feedrate, dt);
        } else if (cmd.type == "G2" || cmd.type == "G3") {
            double tx = cmd.x;
            double ty = cmd.y;
            double ci = cmd.i;
            double cj = cmd.j;

            // --- G90/G91 ---
            if (absoluteMode) {
                tx += (robot.offsetX * 100);  // offset of G92
                ty += (robot.offsetY * 100);
            } else {
                tx += robot.GetCurrentX_Machine() * 100;
                ty += robot.GetCurrentY_Machine() * 100;
            }

            // --- Sol center of circle (I,J) ---
            // In G-code, I,J are vector from *start point* to *center of bow*
            double cx = (robot.GetCurrentX_Machine() * 100) + ci;
            double cy = (robot.GetCurrentY_Machine() * 100) + cj;

            // --- execute ---
            if (cmd.type == "G2") {
                moveArcCW(robot, cx, cy, tx, ty, cmd.feedrate, dt);
            } else {
                moveArcCCW(robot, cx, cy, tx, ty, cmd.feedrate, dt);
            }

            printf("%s: target=(%.3f, %.3f) center=(%.3f, %.3f)\n", cmd.type.c_str(), tx, ty, cx, cy);
        } else if (cmd.type == "G4") {
            dwell(cmd.dwellTime);
        } else if (cmd.type == "G28") {
            double homeX = HOME_X;
            double homeY = HOME_Y;
            moveLinear(robot, homeX, homeY, cmd.feedrate > 0 ? cmd.feedrate : 10.0, dt);
        } else if (cmd.type == "G90") {
            absoluteMode = true;
        } else if (cmd.type == "G91") {
            absoluteMode = false;
        } else if (cmd.type == "G92") {
            double wx = cmd.hasX ? cmd.x : robot.GetCurrentX_Work();
            double wy = cmd.hasY ? cmd.y : robot.GetCurrentY_Work();
            robot.SetWorkOffset(wx, wy);
            std::cout << "Work offset set (G92): now (" << robot.GetCurrentX_Work() << ", " << robot.GetCurrentY_Work()
                      << ") = (0,0)\n";
        } else {
            std::cout << "Unknown G-code: " << cmd.type << std::endl;
            break;
        }
    }
}

// ======== G-Code Implementations ========
//
static void moveLinear(ArmRobot& robot, double targetX, double targetY, double feedrate, double dt) {
    double x0 = robot.GetCurrentX();
    double y0 = robot.GetCurrentY();
    targetX /= 100;
    targetY /= 100;
    feedrate /= 100;
    double dx   = targetX - x0;
    double dy   = targetY - y0;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 1e-8) return;

    int steps = std::max(1, (int)(dist / (feedrate * dt)));

    for (int i = 1; i <= steps; i++) {
        if (flag_impl.load() != 1) return;
        double ratio = (double)i / steps;
        double nx    = x0 + ratio * dx;
        double ny    = y0 + ratio * dy;
        robot.MoveTo(nx, ny);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(dt * 1000)));
    }
}

static void moveLinearSmooth(ArmRobot& robot, double tx, double ty, double feedrate, double dt) {
    double cx = robot.GetCurrentX_Machine();
    double cy = robot.GetCurrentY_Machine();

    tx /= 100;
    ty /= 100;
    // feedrate /= 100;
    double dx   = tx - cx;
    double dy   = ty - cy;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.001) return;

    double dirX = dx / dist;
    double dirY = dy / dist;

    double v     = 0.0;
    double vMax  = feedrate / 160.0;  // mm/s
    double accel = 400.0;             // mm/s²
    double decel = 400.0;

    double traveled = 0.0;

    while (traveled < dist) {
        // --- ACCEL ---
        v += accel * dt;
        if (v > vMax) v = vMax;

        double remain   = dist - traveled;
        double stopDist = (v * v) / (2.0 * decel);

        // --- DECEL ---
        if (stopDist >= remain) {
            v -= decel * dt;
            if (v < 0) v = 0;
        }

        // --- STEP ---
        double step = v * dt;
        if (step > remain) step = remain;

        cx += dirX * step;
        cy += dirY * step;

        // --- Move robot using IK ---
        auto sols = robot.inverseKinematics(cx, cy);
        if (!sols.empty()) {
            // chọn solution elbow-up
            auto sol     = sols[0];
            robot.theta1 = sol.theta1;
            robot.theta2 = sol.theta2;
        }
        robot.UpdatePosition();
        traveled += step;
        usleep(dt * 1e6);
    }

    robot.MoveTo(tx, ty);
}

static void moveArcCW(ArmRobot& robot, double cx, double cy, double tx, double ty, double feedrate, double dt) {
    double x0 = robot.GetCurrentX();
    double y0 = robot.GetCurrentY();
    cx /= 100;
    cy /= 100;
    tx /= 100;
    ty /= 100;
    feedrate /= 100;

    double r = std::sqrt((x0 - cx) * (x0 - cx) + (y0 - cy) * (y0 - cy));

    double startAngle = std::atan2(y0 - cy, x0 - cx);
    double endAngle   = std::atan2(ty - cy, tx - cx);
    if (abs(endAngle - startAngle) <= 1e-8) endAngle -= 2 * M_PI;  // diff

    double arcLength = r * std::fabs(endAngle - startAngle);
    int    steps     = std::max(1, (int)(arcLength / (feedrate * dt)));

    for (int i = 1; i <= steps; i++) {
        if (flag_impl.load() != 1) return;
        double theta = startAngle + (endAngle - startAngle) * ((double)i / steps);
        double nx    = cx + r * std::cos(theta);
        double ny    = cy + r * std::sin(theta);
        robot.MoveTo(nx, ny);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(dt * 1000)));
    }
}

// (G3)
static void moveArcCCW(ArmRobot& robot, double cx, double cy, double tx, double ty, double feedrate, double dt) {
    double x0 = robot.GetCurrentX();
    double y0 = robot.GetCurrentY();
    cx /= 100;
    cy /= 100;
    tx /= 100;
    ty /= 100;
    feedrate /= 100;

    double r          = std::sqrt((x0 - cx) * (x0 - cx) + (y0 - cy) * (y0 - cy));
    double startAngle = std::atan2(y0 - cy, x0 - cx);
    double endAngle   = std::atan2(ty - cy, tx - cx);
    if (abs(endAngle - startAngle) < 1e-8) endAngle += 2 * M_PI;

    double arcLength = r * (endAngle - startAngle);
    int    steps     = std::max(1, (int)(arcLength / (feedrate * dt)));

    for (int i = 1; i <= steps; i++) {
        if (flag_impl.load() != 1) return;
        double theta = startAngle + (endAngle - startAngle) * ((double)i / steps);
        double nx    = cx + r * std::cos(theta);
        double ny    = cy + r * std::sin(theta);
        robot.MoveTo(nx, ny);
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(dt * 1000)));
    }
}

// pause G4 P[t]
static void dwell(double seconds) {
    std::cout << "Dwell for " << seconds << " s" << std::endl;
    uint8_t split_time_to_check = 100;
    seconds /= split_time_to_check;
    for (int i = 0; i < split_time_to_check; i++) {
        if (flag_impl.load() != 1) return;
        std::this_thread::sleep_for(std::chrono::milliseconds((int)(seconds * 1000)));
    }
}
