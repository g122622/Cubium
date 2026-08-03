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

#pragma once

#include "common/core/Types.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

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
     * @return true 表示事件已消费，阻止后续 action 触发；false 继续正常流程
     */
    using KeyEventCallback = std::function<bool(i32 key, i32 action, i32 mods)>;

    /**
     * @brief 设置键盘事件回调
     * @param callback 回调函数
     */
    void setKeyEventCallback(KeyEventCallback callback) { m_keyEventCallback = std::move(callback); }

    /**
     * @brief 清除键盘事件回调
     */
    void clearKeyEventCallback() { m_keyEventCallback = nullptr; }

    /**
     * @brief 获取当前修饰键状态
     * @return GLFW修饰键位掩码 (GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT, GLFW_MOD_SUPER)
     *
     * 返回最近一次键盘或鼠标事件中的修饰键状态。
     * 用于在轮询模式下获取点击时的修饰键状态。
     */
    [[nodiscard]] i32 currentMods() const noexcept { return m_currentMods; }

    // 按键绑定
    using ActionCallback = std::function<void()>;

    void bindKeyAction(i32 key, const std::string& action);
    void bindActionCallback(const std::string& action, ActionCallback callback);

private:
    static void _keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void _mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void _mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void _scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void _charCallback(GLFWwindow* window, unsigned int codepoint);

    void _handleKey(i32 key, i32 action, i32 mods);
    void _handleMouseButton(i32 button, i32 action, i32 mods);
    void _handleMouseMove(f64 x, f64 y);
    void _handleScroll(f64 x, f64 y);
    void _handleCharInput(u32 codepoint);

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

    // 当前修饰键状态（由GLFW回调更新）
    i32 m_currentMods = 0;

    // 鼠标锁定
    bool m_mouseLocked = false;

    // 按键绑定
    std::unordered_map<i32, std::string> m_keyBindings;
    std::unordered_map<std::string, ActionCallback> m_actionCallbacks;

    // 字符输入回调
    CharCallback m_charCallback;

    // 键盘事件回调
    KeyEventCallback m_keyEventCallback;
};

} // namespace mc::client
