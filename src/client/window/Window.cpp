/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "Window.hpp"
#include "common/profiler/TraceEvents.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

// 静态计数器，跟踪GLFW初始化
static int s_glfwInitCount = 0;

Window::Window() = default;

Window::~Window()
{
    destroy();
}

Window::Window(Window&& other) noexcept
    : m_window(other.m_window)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_framebufferWidth(other.m_framebufferWidth)
    , m_framebufferHeight(other.m_framebufferHeight)
    , m_fullscreen(other.m_fullscreen)
    , m_cursorVisible(other.m_cursorVisible)
    , m_initialized(other.m_initialized)
    , m_resizeCallback(other.m_resizeCallback)
    , m_resizeUserData(other.m_resizeUserData)
    , m_keyCallback(other.m_keyCallback)
    , m_keyUserData(other.m_keyUserData)
    , m_mouseCallback(other.m_mouseCallback)
    , m_mouseUserData(other.m_mouseUserData)
    , m_mouseButtonCallback(other.m_mouseButtonCallback)
    , m_mouseButtonUserData(other.m_mouseButtonUserData)
    , m_scrollCallback(other.m_scrollCallback)
    , m_scrollUserData(other.m_scrollUserData)
{
    other.m_window = nullptr;
    other.m_initialized = false;
}

Window& Window::operator=(Window&& other) noexcept
{
    if (this != &other) {
        destroy();
        m_window = other.m_window;
        m_width = other.m_width;
        m_height = other.m_height;
        m_framebufferWidth = other.m_framebufferWidth;
        m_framebufferHeight = other.m_framebufferHeight;
        m_fullscreen = other.m_fullscreen;
        m_cursorVisible = other.m_cursorVisible;
        m_initialized = other.m_initialized;
        m_resizeCallback = other.m_resizeCallback;
        m_resizeUserData = other.m_resizeUserData;
        m_keyCallback = other.m_keyCallback;
        m_keyUserData = other.m_keyUserData;
        m_mouseCallback = other.m_mouseCallback;
        m_mouseUserData = other.m_mouseUserData;
        m_mouseButtonCallback = other.m_mouseButtonCallback;
        m_mouseButtonUserData = other.m_mouseButtonUserData;
        m_scrollCallback = other.m_scrollCallback;
        m_scrollUserData = other.m_scrollUserData;

        other.m_window = nullptr;
        other.m_initialized = false;
    }
    return *this;
}

Result<void> Window::create(const WindowConfig& config)
{
    // 初始化GLFW（如果需要）
    if (s_glfwInitCount == 0) {
        if (!glfwInit()) {
            return Error(ErrorCode::Unknown, "Failed to initialize GLFW");
        }
    }
    ++s_glfwInitCount;

    // 设置GLFW提示
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Vulkan
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, config.decorated ? GLFW_TRUE : GLFW_FALSE);

    // 创建窗口
    m_window = glfwCreateWindow(config.width,
        config.height,
        config.title.c_str(),
        config.fullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr);

    if (!m_window) {
        if (s_glfwInitCount > 0) {
            --s_glfwInitCount;
            if (s_glfwInitCount == 0) {
                glfwTerminate();
            }
        }
        return Error(ErrorCode::Unknown, "Failed to create window");
    }

    // 设置用户指针
    glfwSetWindowUserPointer(m_window, this);

    // 设置回调
    glfwSetFramebufferSizeCallback(m_window, _framebufferSizeCallback);
    glfwSetKeyCallback(m_window, _keyCallback);
    glfwSetCursorPosCallback(m_window, _cursorPosCallback);
    glfwSetMouseButtonCallback(m_window, _mouseButtonCallback);
    glfwSetScrollCallback(m_window, _scrollCallback);

    // 获取尺寸
    glfwGetWindowSize(m_window, &m_width, &m_height);
    glfwGetFramebufferSize(m_window, &m_framebufferWidth, &m_framebufferHeight);

    // 设置VSync
    glfwSwapInterval(config.vsync ? 1 : 0);

    m_fullscreen = config.fullscreen;
    if (!config.fullscreen) {
        glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
        m_windowedWidth = m_width;
        m_windowedHeight = m_height;
    } else {
        m_windowedWidth = config.width;
        m_windowedHeight = config.height;
    }
    m_initialized = true;

    spdlog::info(
        "Window created: {}x{} (framebuffer: {}x{})", m_width, m_height, m_framebufferWidth, m_framebufferHeight);

    return Result<void>::ok();
}

void Window::destroy()
{
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        spdlog::info("Window destroyed");
    }

    if (m_initialized) {
        --s_glfwInitCount;
        if (s_glfwInitCount == 0) {
            glfwTerminate();
        }
        m_initialized = false;
    }
}

bool Window::shouldClose() const
{
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void Window::pollEvents()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Window::pollEvents");
    glfwPollEvents();
}

void Window::swapBuffers()
{
    if (m_window) {
        glfwSwapBuffers(m_window);
    }
}

void Window::setTitle(const std::string& title)
{
    if (m_window) {
        glfwSetWindowTitle(m_window, title.c_str());
    }
}

void Window::setSize(i32 width, i32 height)
{
    if (m_window) {
        glfwSetWindowSize(m_window, width, height);
    }
}

void Window::setVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setFullscreen(bool fullscreen)
{
    if (!m_window || m_fullscreen == fullscreen) {
        return;
    }

    if (fullscreen) {
        // 保存窗口化模式的位置和尺寸
        glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (!monitor || !mode) {
            spdlog::warn("Failed to enter fullscreen: monitor or video mode unavailable");
            return;
        }

        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    } else {
        glfwSetWindowMonitor(
            m_window, nullptr, m_windowedX, m_windowedY, m_windowedWidth, m_windowedHeight, GLFW_DONT_CARE);
    }

    m_fullscreen = fullscreen;
}

void Window::setCursorVisible(bool visible)
{
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        m_cursorVisible = visible;
    }
}

void Window::setResizeCallback(ResizeCallback callback, void* userData)
{
    m_resizeCallback = callback;
    m_resizeUserData = userData;
}

void Window::setKeyCallback(KeyCallback callback, void* userData)
{
    m_keyCallback = callback;
    m_keyUserData = userData;
}

void Window::setMouseCallback(MouseCallback callback, void* userData)
{
    m_mouseCallback = callback;
    m_mouseUserData = userData;
}

void Window::setMouseButtonCallback(MouseButtonCallback callback, void* userData)
{
    m_mouseButtonCallback = callback;
    m_mouseButtonUserData = userData;
}

void Window::setScrollCallback(ScrollCallback callback, void* userData)
{
    m_scrollCallback = callback;
    m_scrollUserData = userData;
}

void Window::_framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_framebufferWidth = width;
        win->m_framebufferHeight = height;
        glfwGetWindowSize(window, &win->m_width, &win->m_height);

        if (win->m_resizeCallback) {
            win->m_resizeCallback(width, height, win->m_resizeUserData);
        }
    }
}

void Window::_keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_keyCallback) {
        win->m_keyCallback(key, scancode, action, mods, win->m_keyUserData);
    }
}

void Window::_cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_mouseCallback) {
        win->m_mouseCallback(xpos, ypos, win->m_mouseUserData);
    }
}

void Window::_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_mouseButtonCallback) {
        win->m_mouseButtonCallback(button, action, mods, win->m_mouseButtonUserData);
    }
}

void Window::_scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win && win->m_scrollCallback) {
        win->m_scrollCallback(xoffset, yoffset, win->m_scrollUserData);
    }
}

} // namespace mc::client
