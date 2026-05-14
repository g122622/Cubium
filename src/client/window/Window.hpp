#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <memory>
#include <string>

// 前向声明
struct GLFWwindow;

namespace mc::client {

/**
 * @brief 窗口配置
 */
struct WindowConfig {
    i32 width = 1280;
    i32 height = 720;
    std::string title = "Minecraft Reborn";
    bool fullscreen = false;
    bool vsync = true;
    bool resizable = true;
    bool decorated = true;
    i32 monitorIndex = 0;
};

/**
 * @brief 窗口类
 *
 * 封装GLFW窗口，提供跨平台的窗口管理
 */
class Window {
public:
    Window();
    ~Window();

    // 禁止拷贝
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // 允许移动
    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    /**
     * @brief 创建窗口
     */
    [[nodiscard]] Result<void> create(const WindowConfig& config);

    /**
     * @brief 销毁窗口
     */
    void destroy();

    /**
     * @brief 检查窗口是否应该关闭
     */
    [[nodiscard]] bool shouldClose() const;

    /**
     * @brief 轮询事件
     */
    void pollEvents();

    /**
     * @brief 交换缓冲区
     */
    void swapBuffers();

    /**
     * @brief 获取窗口尺寸
     */
    [[nodiscard]] i32 width() const noexcept { return m_width; }
    [[nodiscard]] i32 height() const noexcept { return m_height; }

    /**
     * @brief 获取帧缓冲尺寸
     */
    [[nodiscard]] i32 framebufferWidth() const noexcept { return m_framebufferWidth; }
    [[nodiscard]] i32 framebufferHeight() const noexcept { return m_framebufferHeight; }

    /**
     * @brief 获取宽高比
     */
    [[nodiscard]] f32 aspectRatio() const noexcept { return static_cast<f32>(m_width) / static_cast<f32>(m_height); }

    /**
     * @brief 设置窗口标题
     */
    void setTitle(const std::string& title);

    /**
     * @brief 设置窗口尺寸
     */
    void setSize(i32 width, i32 height);

    /**
     * @brief 设置VSync
     */
    void setVSync(bool enabled);

    /**
     * @brief 设置全屏模式
     */
    void setFullscreen(bool fullscreen);

    /**
     * @brief 检查是否全屏
     */
    [[nodiscard]] bool isFullscreen() const noexcept { return m_fullscreen; }

    /**
     * @brief 显示/隐藏光标
     */
    void setCursorVisible(bool visible);

    /**
     * @brief 检查光标是否可见
     */
    [[nodiscard]] bool isCursorVisible() const noexcept { return m_cursorVisible; }

    /**
     * @brief 获取原生窗口句柄
     */
    [[nodiscard]] GLFWwindow* handle() const noexcept { return m_window; }

    /**
     * @brief 检查窗口是否有效
     */
    [[nodiscard]] bool isValid() const noexcept { return m_window != nullptr; }

    // 回调设置
    using ResizeCallback = void (*)(i32 width, i32 height, void* userData);
    using KeyCallback = void (*)(i32 key, i32 scancode, i32 action, i32 mods, void* userData);
    using MouseCallback = void (*)(f64 x, f64 y, void* userData);
    using MouseButtonCallback = void (*)(i32 button, i32 action, i32 mods, void* userData);
    using ScrollCallback = void (*)(f64 xoffset, f64 yoffset, void* userData);

    void setResizeCallback(ResizeCallback callback, void* userData = nullptr);
    void setKeyCallback(KeyCallback callback, void* userData = nullptr);
    void setMouseCallback(MouseCallback callback, void* userData = nullptr);
    void setMouseButtonCallback(MouseButtonCallback callback, void* userData = nullptr);
    void setScrollCallback(ScrollCallback callback, void* userData = nullptr);

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* m_window = nullptr;
    i32 m_width = 0;
    i32 m_height = 0;
    i32 m_framebufferWidth = 0;
    i32 m_framebufferHeight = 0;
    bool m_fullscreen = false;
    bool m_cursorVisible = true;
    bool m_initialized = false;

    // 窗口化模式下的位置和尺寸（用于全屏切换恢复）
    i32 m_windowedX = 100;
    i32 m_windowedY = 100;
    i32 m_windowedWidth = 1280;
    i32 m_windowedHeight = 720;

    // 回调
    ResizeCallback m_resizeCallback = nullptr;
    void* m_resizeUserData = nullptr;
    KeyCallback m_keyCallback = nullptr;
    void* m_keyUserData = nullptr;
    MouseCallback m_mouseCallback = nullptr;
    void* m_mouseUserData = nullptr;
    MouseButtonCallback m_mouseButtonCallback = nullptr;
    void* m_mouseButtonUserData = nullptr;
    ScrollCallback m_scrollCallback = nullptr;
    void* m_scrollUserData = nullptr;
};

} // namespace mc::client
