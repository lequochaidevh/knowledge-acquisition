#include "window.h"

// Factory helper
std::unique_ptr<NativeWindow> CreateGLFWWindow(const WindowConfigure_t mConfig) {
    return std::make_unique<GLFWWindow>(mConfig);
}

// Factory implementation
std::unique_ptr<NativeWindow> NativeWindow::Create(const EWindowSpec eWindow, WindowConfigure_t mWindowConfig) {
    switch (eWindow) {
        case EWindowSpec::GLFW:
            return CreateGLFWWindow(mWindowConfig);
            break;

        default:
            break;
    }
    return nullptr;
}