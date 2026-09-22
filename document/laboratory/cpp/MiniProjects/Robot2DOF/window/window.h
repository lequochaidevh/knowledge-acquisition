#pragma once
#include <string>
#include <memory>
#include <functional>

#include "../application.h"

#include <GLFW/glfw3.h>
#include "keyGLFW.h"

enum class EWindowSpec { GLFW, Vulkan, SDL, Unknown };

// Forward declare GLFWwindow
typedef struct WindowConfigure {
    std::string title;
    uint32_t    width, height;
} WindowConfigure_t;

class NativeWindow {
 public:
    virtual ~NativeWindow() = default;

    virtual void      PollEvents()            = 0;
    virtual bool      ShouldClose() const     = 0;
    virtual void      SwapBuffers()           = 0;
    virtual void*     GetNativeHandle() const = 0;
    virtual double    GetTime() const         = 0;
    WindowConfigure_t mWindowConfig;

    // Generic callbacks
    using KeyCallbackFn = std::function<void(int key, int action)>;
    using MouseClickFn  = std::function<void(double x, double y)>;

    virtual void SetKeyCallback(KeyCallbackFn fn)       = 0;
    virtual void SetMouseClickCallback(MouseClickFn fn) = 0;

    virtual void DrawCircle(double cx, double cy, double r, int segs = 24) = 0;
    virtual void DrawLine(double x1, double y1, double x2, double y2)      = 0;

    virtual Vec2 screenToWorld(double xpos, double ypos, double scale = 1.2) const = 0;

    // Factory
    static std::unique_ptr<NativeWindow> Create(const EWindowSpec, const WindowConfigure_t);
};

class GLFWWindow : public NativeWindow {
 public:
    GLFWWindow(const WindowConfigure_t mConfig);

    ~GLFWWindow() override;

    void   PollEvents() override;
    bool   ShouldClose() const override;
    void   SwapBuffers() override;
    void*  GetNativeHandle() const override;
    double GetTime() const override;

    void SetKeyCallback(KeyCallbackFn fn) override;
    void SetMouseClickCallback(MouseClickFn fn) override;

    void DrawCircle(double cx, double cy, double r, int segs = 24) override;
    void DrawLine(double x1, double y1, double x2, double y2) override;

    Vec2 screenToWorld(double xpos, double ypos, double scale = 1.2) const override;

 private:
    GLFWwindow*   window = nullptr;
    KeyCallbackFn keyCallback;
    MouseClickFn  mouseClickCallback;
};
