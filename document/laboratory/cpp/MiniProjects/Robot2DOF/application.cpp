
#include "application.h"
#include "window/window.h"

static std::vector<GCodeCommand> gcodeCmds;

bool sim_enabled = false;
double target_theta1 = 0.0;
double target_theta2 = 0.0;
double target_x = 0.6, target_y = 0.0;

std::atomic<uint8_t> flag_impl{1};
// Draw path

std::vector<std::pair<double, double>> pathPoints;
bool drawPath = false; // toggle by D Key

int main(){
    WindowConfigure_t cfg{"Robot2DOF Sim", 800, 800};
    auto window = NativeWindow::Create(EWindowSpec::GLFW, cfg);
    GCodeParser parser;
    // ---------- callbacks ----------
    window->SetKeyCallback([](int key, int action) {
        if (action != KEY_PRESS) return;

        switch (key) {
            case KEY_R:
                render_enabled = !render_enabled;
                printf("render = %d\n", render_enabled);
                break;
            case KEY_S:
                sim_enabled = !sim_enabled;
                printf("sim = %d\n", sim_enabled);
                break;
            case KEY_1: robot.theta1 += 0.1; break;
            case KEY_2: robot.theta1 -= 0.1; break;
            case KEY_3: robot.theta2 += 0.1; break;
            case KEY_4: robot.theta2 -= 0.1; break;
            case KEY_D: {
                drawPath = !drawPath;
                printf("drawPath = %d\n", drawPath);
                break;
            }
            case KEY_G: {
                flag_impl.store(1);
                printf("flag_impl.load() = %d\n", flag_impl.load());
                pathPoints.clear();
                break;
            }
            case KEY_I: {
                flag_impl.store(2);
                printf("flag_impl.load() = %d\n", flag_impl.load());
                auto sols = robot.inverseKinematics(target_x, target_y);
                if (!sols.empty()) {
                    target_theta1 = sols[0].theta1;
                    target_theta2 = sols[0].theta2;
                    printf("IK target set. theta1=%.3f theta2=%.3f\n",
                           target_theta1, target_theta2);
                } else {
                    printf("IK failed: target unreachable\n");
                }
                break;
            }
        }
    });

    window->SetMouseClickCallback([](double wx, double wy) {
        target_x = wx;
        target_y = wy;
        show_target = true;
        printf("Set target (%.3f, %.3f)\n", wx, wy);
    });

    // ---------- Simulation loop ----------
    std::thread execThread;
    std::atomic<bool> thread_done{true}; 

    double lastt = window->GetTime();
    while (!window->ShouldClose()) {
        double now = window->GetTime();
        double dt = now - lastt;
        lastt = now;
        if (sim_enabled) {
            {
                if(flag_impl.load() == 2) {
                    time_acc += dt;                
                    double alpha = 1.0 - exp(-4.0 * dt); //speed
                    robot.theta1 += alpha * (target_theta1 - robot.theta1);
                    robot.theta2 += alpha * (target_theta2 - robot.theta2);
                    robot.UpdatePosition();

                    // ---  ---
                    double err1 = fabs(target_theta1 - robot.theta1);
                    double err2 = fabs(target_theta2 - robot.theta2);
                    double delta_e = std::max(err1, err2);  // or sqrt(err1²+err2²)

                    if (delta_e < 0.007) {
                        flag_impl.store(0);
                        printf("Reached target (Δθ=%.4f). flag_impl=0\n", delta_e); // break loop
                    }
                }
            }
        }

        if(flag_impl.load() == 1 && thread_done.load()) {
            if (execThread.joinable()) execThread.join();
            thread_done = false;
            execThread = std::thread([&]() {
                gcodeCmds = \
                    parser.Parse(\
                        "/home/devh/linux_std/c_driver_stream/"
                        "robot2dof/initSimp/src/gcode/demo.gcode"
                    );
                ExecuteGCodeStep(robot, gcodeCmds, 0.07);
                thread_done = true;
                if(flag_impl.load() == 1) { flag_impl.store(0); } // stop when implement GCODE file
            });
        }

        // --- Clear screen ---
        glViewport(0, 0, 800, 800);
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (render_enabled) {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            double s = 1.2;
            glOrtho(-s, s, -s, s, -1, 1);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            // Draw workspace circle
            glColor3f(0.9f, 0.9f, 0.9f);
            window->DrawCircle(0, 0, robot.l1 + robot.l2);

            // Forward kinematics
            auto Ts = robot.forwardKinematics(robot.theta1, robot.theta2);
            Vec2 p0 = Ts[0].getXY();
            Vec2 p1 = Ts[1].getXY();
            Vec2 p2 = Ts[2].getXY();

            // Draw links
            // TODO: Need factory by GLFW
            glLineWidth(8.0f);
            glColor3f(0.2f, 0.2f, 0.8f);
            window->DrawLine(p0.x, p0.y, p1.x, p1.y);
            glLineWidth(8.0f);
            glColor3f(0.2f, 0.8f, 0.2f);
            window->DrawLine(p1.x, p1.y, p2.x, p2.y);
            glLineWidth(1.0f);

            // Draw joints
            glColor3f(0.1f, 0.1f, 0.1f);
            window->DrawCircle(p0.x, p0.y, 0.03);
            window->DrawCircle(p1.x, p1.y, 0.03);
            // EE
            glColor3f(0.8f, 0.1f, 0.1f);
            window->DrawCircle(p2.x, p2.y, 0.03);

            // Draw target if present
            if (show_target) {
                glColor3f(0.6f, 0.0f, 0.6f);
                window->DrawCircle(target_x, target_y, 0.02);
            }

            // Draw Jacobian vectors
            double J[2][2];
            robot.jacobian(robot.theta1, robot.theta2, J);
            double scale = 0.15;
            glColor3f(0.0f, 0.0f, 0.0f);
            window->DrawLine(p2.x, p2.y, p2.x + scale * J[0][0], p2.y + scale * J[1][0]);
            window->DrawLine(p2.x, p2.y, p2.x + scale * J[0][1], p2.y + scale * J[1][1]);

            // Draw the path of Robot when Moveto() x y
            if (drawPath && !pathPoints.empty()) {
                glColor3f(0.1f, 0.8f, 0.1f); // green color
                glBegin(GL_LINE_STRIP);
                for (auto &p : pathPoints)
                    glVertex2f(p.first, p.second);
                glEnd();
            }
        }

        window->SwapBuffers();
        window->PollEvents();

    }
    if(execThread.joinable()) { execThread.join(); }
    return 0;
}
