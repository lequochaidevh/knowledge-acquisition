#include "window.h"
#include <cstdio>
#include <stdexcept>
#include <cmath>

GLFWWindow::GLFWWindow(const WindowConfigure_t mConfig) {
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
    mWindowConfig = mConfig;

    window = glfwCreateWindow(mConfig.width, mConfig.height, mConfig.title.c_str(), nullptr, nullptr);

    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    // Link instance to ptr user data
    glfwSetWindowUserPointer(window, this);

    // Subribe callback
    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int scancode, int action, int mods) {
        auto self = static_cast<GLFWWindow *>(glfwGetWindowUserPointer(win));
        if (self->keyCallback) self->keyCallback(key, action);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int mods) {
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
        auto self = static_cast<GLFWWindow *>(glfwGetWindowUserPointer(win));
        if (!self->mouseClickCallback) return;

        double mx, my;
        int    w, h;
        glfwGetCursorPos(win, &mx, &my);
        glfwGetWindowSize(win, &w, &h);
        // Map to world coordinates, matching your glOrtho(-s, s, -s, s, -1, 1)
        double s  = 1.2;  // same as in your render() // calib possiton with mouse
        double wx = ((mx / w) * 2.0 - 1.0) * s;
        double wy = (1.0 - (my / h) * 2.0) * s;  // [-1,1] (mirror Y axist)
        self->mouseClickCallback(wx, wy);
    });

    printf("[GLFW] Created window %dx%d: %s\n", mConfig.width, mConfig.height, mConfig.title.c_str());
}

GLFWWindow::~GLFWWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
    printf("[GLFW] Terminated.\n");
}

void  GLFWWindow::PollEvents() { glfwPollEvents(); }
bool  GLFWWindow::ShouldClose() const { return glfwWindowShouldClose(window); }
void  GLFWWindow::SwapBuffers() { glfwSwapBuffers(window); }
void *GLFWWindow::GetNativeHandle() const { return window; }

double GLFWWindow::GetTime() const { return glfwGetTime(); }

void GLFWWindow::SetKeyCallback(KeyCallbackFn fn) { keyCallback = std::move(fn); }
void GLFWWindow::SetMouseClickCallback(MouseClickFn fn) { mouseClickCallback = std::move(fn); }

inline void GLFWWindow::DrawCircle(double cx, double cy, double r, int segs) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= segs; i++) {
        double a = (2.0 * M_PI * i) / segs;
        glVertex2d(cx + cos(a) * r, cy + sin(a) * r);
    }
    glEnd();
}

inline void GLFWWindow::DrawLine(double x1, double y1, double x2, double y2) {
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
}

Vec2 GLFWWindow::screenToWorld(double xpos, double ypos, double scale) const {
    int    w = mWindowConfig.width, h = mWindowConfig.height;
    double aspect = (double)w / h;
    double x_ndc  = (xpos / w) * 2.0 - 1.0;
    double y_ndc  = 1.0 - (ypos / h) * 2.0;
    return Vec2{x_ndc * scale * aspect, y_ndc * scale};
}
