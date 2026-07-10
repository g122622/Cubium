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

#include "InputManager.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client {

namespace {

std::unordered_map<GLFWwindow*, InputManager*> g_inputManagers;

InputManager* getInputManager(GLFWwindow* window)
{
    auto it = g_inputManagers.find(window);
    if (it != g_inputManagers.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace

InputManager::~InputManager()
{
    if (m_window != nullptr) {
        g_inputManagers.erase(m_window);
    }
}

void InputManager::initialize(GLFWwindow* window)
{
    m_window = window;
    g_inputManagers[window] = this;

    // 设置回调
    glfwSetKeyCallback(window, _keyCallback);
    glfwSetCursorPosCallback(window, _mouseCallback);
    glfwSetMouseButtonCallback(window, _mouseButtonCallback);
    glfwSetScrollCallback(window, _scrollCallback);
    glfwSetCharCallback(window, _charCallback);

    // 初始化鼠标位置
    glfwGetCursorPos(window, &m_mouseX, &m_mouseY);
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;

    spdlog::info("InputManager initialized");
}

void InputManager::update()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "InputManager::update");

    if (m_window) {
        std::unordered_set<i32> currentMouseButtonsPressed;
        for (i32 button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
            if (glfwGetMouseButton(m_window, button) == GLFW_PRESS) {
                currentMouseButtonsPressed.insert(button);
            }
        }

        for (i32 button : currentMouseButtonsPressed) {
            if (m_previousMouseButtonsPressed.count(button) == 0) {
                m_mouseButtonsJustPressed.insert(button);
                m_mouseButtonsJustReleased.erase(button);
            }
        }

        for (i32 button : m_previousMouseButtonsPressed) {
            if (currentMouseButtonsPressed.count(button) == 0) {
                m_mouseButtonsJustReleased.insert(button);
                m_mouseButtonsJustPressed.erase(button);
            }
        }

        m_mouseButtonsPressed = std::move(currentMouseButtonsPressed);
        m_previousMouseButtonsPressed = m_mouseButtonsPressed;
    }

    // 计算鼠标增量
    m_mouseDeltaX = m_mouseX - m_lastMouseX;
    m_mouseDeltaY = m_mouseY - m_lastMouseY;
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;
}

void InputManager::endFrame()
{
    m_keysJustPressed.clear();
    m_keysJustReleased.clear();
    m_mouseButtonsJustPressed.clear();
    m_mouseButtonsJustReleased.clear();
    m_scrollDeltaX = 0.0;
    m_scrollDeltaY = 0.0;
}

bool InputManager::isKeyPressed(i32 key) const
{
    return m_keysPressed.count(key) > 0;
}

bool InputManager::isKeyJustPressed(i32 key) const
{
    return m_keysJustPressed.count(key) > 0;
}

bool InputManager::isKeyJustReleased(i32 key) const
{
    return m_keysJustReleased.count(key) > 0;
}

bool InputManager::isMouseButtonPressed(i32 button) const
{
    return m_mouseButtonsPressed.count(button) > 0;
}

bool InputManager::isMouseButtonJustPressed(i32 button) const
{
    return m_mouseButtonsJustPressed.count(button) > 0;
}

bool InputManager::isMouseButtonJustReleased(i32 button) const
{
    return m_mouseButtonsJustReleased.count(button) > 0;
}

void InputManager::setMouseLocked(bool locked)
{
    m_mouseLocked = locked;
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        // 切换模式后重置鼠标状态，避免跳跃
        glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
    }
}

void InputManager::bindKeyAction(i32 key, const std::string& action)
{
    m_keyBindings[key] = action;
}

void InputManager::bindActionCallback(const std::string& action, ActionCallback callback)
{
    m_actionCallbacks[action] = std::move(callback);
}

void InputManager::_keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
{
    auto* input = getInputManager(window);
    if (input && key >= 0) {
        input->_handleKey(key, action, mods);
    }
}

void InputManager::_mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* input = getInputManager(window);
    if (input) {
        input->_handleMouseMove(xpos, ypos);
    }
}

void InputManager::_mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* input = getInputManager(window);
    if (input && button >= 0) {
        input->_handleMouseButton(button, action, mods);
    }
}

void InputManager::_scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* input = getInputManager(window);
    if (input) {
        input->_handleScroll(xoffset, yoffset);
    }
}

void InputManager::_handleKey(i32 key, i32 action, i32 mods)
{
    m_currentMods = mods;

    // 先触发键盘事件回调（用于UI输入处理），如果回调消费了事件则不再触发 action
    if (m_keyEventCallback) {
        if (m_keyEventCallback(key, action, mods)) {
            return;
        }
    }

    if (action == GLFW_PRESS) {
        m_keysPressed.insert(key);
        m_keysJustPressed.insert(key);
        m_keysJustReleased.erase(key);

        // 触发按键绑定
        auto it = m_keyBindings.find(key);
        if (it != m_keyBindings.end()) {
            auto callbackIt = m_actionCallbacks.find(it->second);
            if (callbackIt != m_actionCallbacks.end() && callbackIt->second) {
                callbackIt->second();
            }
        }
    } else if (action == GLFW_RELEASE) {
        m_keysPressed.erase(key);
        m_keysJustPressed.erase(key);
        m_keysJustReleased.insert(key);
    }
}

void InputManager::_handleMouseButton(i32 button, i32 action, i32 mods)
{
    m_currentMods = mods;

    if (action == GLFW_PRESS) {
        m_mouseButtonsPressed.insert(button);
        m_mouseButtonsJustPressed.insert(button);
        m_mouseButtonsJustReleased.erase(button);
    } else if (action == GLFW_RELEASE) {
        m_mouseButtonsPressed.erase(button);
        m_mouseButtonsJustPressed.erase(button);
        m_mouseButtonsJustReleased.insert(button);
    }
}

void InputManager::_handleMouseMove(f64 x, f64 y)
{
    m_mouseX = x;
    m_mouseY = y;
}

void InputManager::_handleScroll(f64 x, f64 y)
{
    m_scrollDeltaX = x;
    m_scrollDeltaY = y;
}

void InputManager::_charCallback(GLFWwindow* window, unsigned int codepoint)
{
    auto* input = getInputManager(window);
    if (input) {
        input->_handleCharInput(codepoint);
    }
}

void InputManager::_handleCharInput(u32 codepoint)
{
    if (m_charCallback) {
        m_charCallback(codepoint);
    }
}

} // namespace mc::client
