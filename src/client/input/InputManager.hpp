#pragma once

#include "common/core/Types.hpp"

#include <functional>
#include <unordered_map>
#include <unordered_set>

// 前向声明
struct GLFWwindow;

namespace mc::client {

/**
 * @brief 输入管理器
 *
 * 管理键盘和鼠标输入
 */
class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    // 禁止拷贝
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /**
     * @brief 初始化输入管理器
     */
    void initialize(GLFWwindow* window);

    /**
     * @brief 更新输入状态（每帧调用）
     */
    void update();

    /**
     * @brief 结束当前帧，清理瞬时输入状态
     */
    void endFrame();

    /**
     * @brief 检查按键是否按下
     */
    [[nodiscard]] bool isKeyPressed(i32 key) const;

    /**
     * @brief 检查按键是否刚按下
     */
    [[nodiscard]] bool isKeyJustPressed(i32 key) const;

    /**
     * @brief 检查按键是否刚释放
     */
    [[nodiscard]] bool isKeyJustReleased(i32 key) const;

    /**
     * @brief 检查鼠标按键是否按下
     */
    [[nodiscard]] bool isMouseButtonPressed(i32 button) const;

    /**
     * @brief 检查鼠标按键是否刚按下
     */
    [[nodiscard]] bool isMouseButtonJustPressed(i32 button) const;

    /**
     * @brief 检查鼠标按键是否刚释放
     */
    [[nodiscard]] bool isMouseButtonJustReleased(i32 button) const;

    /**
     * @brief 获取鼠标位置
     */
    [[nodiscard]] f64 mouseX() const noexcept { return m_mouseX; }
    [[nodiscard]] f64 mouseY() const noexcept { return m_mouseY; }

    /**
     * @brief 获取鼠标增量
     */
    [[nodiscard]] f64 mouseDeltaX() const noexcept { return m_mouseDeltaX; }
    [[nodiscard]] f64 mouseDeltaY() const noexcept { return m_mouseDeltaY; }

    /**
     * @brief 获取滚轮增量
     */
    [[nodiscard]] f64 scrollDeltaX() const noexcept { return m_scrollDeltaX; }
    [[nodiscard]] f64 scrollDeltaY() const noexcept { return m_scrollDeltaY; }

    /**
     * @brief 设置鼠标锁定模式
     */
    void setMouseLocked(bool locked);

    /**
     * @brief 检查鼠标是否锁定
     */
    [[nodiscard]] bool isMouseLocked() const noexcept { return m_mouseLocked; }

    // ========== 字符输入 ==========

    /**
     * @brief 字符输入回调
     */
    using CharCallback = std::function<void(u32 codepoint)>;

    /**
     * @brief 设置字符输入回调
     * @param callback 回调函数
     */
    void setCharCallback(CharCallback callback) { m_charCallback = std::move(callback); }

    /**
     * @brief 清除字符输入回调
     */
    void clearCharCallback() { m_charCallback = nullptr; }

    // ========== 键盘事件回调 ==========

    /**
     * @brief 键盘事件回调（用于UI输入处理）
     * @param key GLFW键码
     * @param action GLFW动作 (GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT)
     * @param mods 修饰键
     */
    using KeyEventCallback = std::function<void(i32 key, i32 action, i32 mods)>;

    /**
     * @brief 设置键盘事件回调
     * @param callback 回调函数
     */
    void setKeyEventCallback(KeyEventCallback callback) { m_keyEventCallback = std::move(callback); }

    /**
     * @brief 清除键盘事件回调
     */
    void clearKeyEventCallback() { m_keyEventCallback = nullptr; }

    // 按键绑定
    using ActionCallback = std::function<void()>;

    void bindKeyAction(i32 key, const std::string& action);
    void bindActionCallback(const std::string& action, ActionCallback callback);

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void charCallback(GLFWwindow* window, unsigned int codepoint);

    void handleKey(i32 key, i32 action);
    void handleMouseButton(i32 button, i32 action);
    void handleMouseMove(f64 x, f64 y);
    void handleScroll(f64 x, f64 y);
    void handleCharInput(u32 codepoint);

    GLFWwindow* m_window = nullptr;

    // 按键状态
    std::unordered_set<i32> m_keysPressed;
    std::unordered_set<i32> m_keysJustPressed;
    std::unordered_set<i32> m_keysJustReleased;

    // 鼠标按键状态
    std::unordered_set<i32> m_mouseButtonsPressed;
    std::unordered_set<i32> m_mouseButtonsJustPressed;
    std::unordered_set<i32> m_mouseButtonsJustReleased;
    std::unordered_set<i32> m_previousMouseButtonsPressed;

    // 鼠标位置
    f64 m_mouseX = 0.0;
    f64 m_mouseY = 0.0;
    f64 m_lastMouseX = 0.0;
    f64 m_lastMouseY = 0.0;
    f64 m_mouseDeltaX = 0.0;
    f64 m_mouseDeltaY = 0.0;

    // 滚轮
    f64 m_scrollDeltaX = 0.0;
    f64 m_scrollDeltaY = 0.0;

    // 鼠标锁定
    bool m_mouseLocked = false;
    bool m_initialized = false;

    // 按键绑定
    std::unordered_map<i32, std::string> m_keyBindings;
    std::unordered_map<std::string, ActionCallback> m_actionCallbacks;

    // 字符输入回调
    CharCallback m_charCallback;

    // 键盘事件回调
    KeyEventCallback m_keyEventCallback;
};

} // namespace mc::client
