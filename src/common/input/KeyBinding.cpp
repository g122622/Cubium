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

#include "KeyBinding.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc {

// 静态成员初始化
std::map<std::string, KeyBinding*> KeyBinding::s_bindings;
std::map<std::string, std::vector<KeyBinding*>> KeyBinding::s_categoryBindings;
KeyBinding::StateCallback KeyBinding::s_stateCallback;

KeyBinding::KeyBinding(std::string id, i32 defaultKey, std::string category)
    : m_id(std::move(id))
    , m_defaultKey(defaultKey)
    , m_currentKey(defaultKey)
    , m_category(std::move(category))
{
    _registerBinding();
}

KeyBinding::~KeyBinding()
{
    _unregisterBinding();
}

KeyBinding::KeyBinding(KeyBinding&& other) noexcept
    : m_id(std::move(other.m_id))
    , m_defaultKey(other.m_defaultKey)
    , m_currentKey(other.m_currentKey)
    , m_category(std::move(other.m_category))
    , m_pressed(other.m_pressed)
    , m_justPressed(other.m_justPressed)
    , m_justReleased(other.m_justReleased)
{
    // 更新静态注册表中的指针
    _registerBinding();
    other._unregisterBinding();
}

KeyBinding& KeyBinding::operator=(KeyBinding&& other) noexcept
{
    if (this != &other) {
        _unregisterBinding();

        m_id = std::move(other.m_id);
        m_defaultKey = other.m_defaultKey;
        m_currentKey = other.m_currentKey;
        m_category = std::move(other.m_category);
        m_pressed = other.m_pressed;
        m_justPressed = other.m_justPressed;
        m_justReleased = other.m_justReleased;

        _registerBinding();
        other._unregisterBinding();
    }
    return *this;
}

void KeyBinding::setKey(i32 key)
{
    m_currentKey = key;
}

void KeyBinding::resetToDefault()
{
    setKey(m_defaultKey);
}

bool KeyBinding::isDefault() const noexcept
{
    return m_currentKey == m_defaultKey;
}

bool KeyBinding::isPressed() const noexcept
{
    return m_pressed;
}

bool KeyBinding::isJustPressed() const noexcept
{
    return m_justPressed;
}

bool KeyBinding::isJustReleased() const noexcept
{
    return m_justReleased;
}

KeyBinding* KeyBinding::find(const std::string& id)
{
    auto it = s_bindings.find(id);
    return (it != s_bindings.end()) ? it->second : nullptr;
}

std::vector<KeyBinding*> KeyBinding::getByCategory(const std::string& category)
{
    auto it = s_categoryBindings.find(category);
    if (it != s_categoryBindings.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> KeyBinding::getCategories()
{
    std::vector<std::string> categories;
    categories.reserve(s_categoryBindings.size());
    for (const auto& [category, bindings] : s_categoryBindings) {
        categories.push_back(category);
    }
    return categories;
}

void KeyBinding::updateAll(const std::vector<i32>& pressedKeys,
    const std::vector<i32>& justPressedKeys,
    const std::vector<i32>& justReleasedKeys)
{
    // 构建快速查找的 set
    auto isPressed = [&pressedKeys](i32 key) {
        return std::find(pressedKeys.begin(), pressedKeys.end(), key) != pressedKeys.end();
    };
    auto isJustPressed = [&justPressedKeys](i32 key) {
        return std::find(justPressedKeys.begin(), justPressedKeys.end(), key) != justPressedKeys.end();
    };
    auto isJustReleased = [&justReleasedKeys](i32 key) {
        return std::find(justReleasedKeys.begin(), justReleasedKeys.end(), key) != justReleasedKeys.end();
    };

    // 更新所有绑定状态
    for (auto& [id, binding] : s_bindings) {
        i32 key = binding->m_currentKey;
        bool wasPressed = binding->m_pressed;

        binding->m_pressed = isPressed(key);
        binding->m_justPressed = isJustPressed(key);
        binding->m_justReleased = isJustReleased(key);

        // 触发状态回调
        if (s_stateCallback &&
            (binding->m_pressed != wasPressed || binding->m_justPressed || binding->m_justReleased)) {
            s_stateCallback(*binding, binding->m_pressed);
        }
    }
}

void KeyBinding::resetAllToDefault()
{
    for (auto& [id, binding] : s_bindings) {
        binding->resetToDefault();
    }
    spdlog::info("All key bindings reset to default");
}

void KeyBinding::setStateCallback(StateCallback callback)
{
    s_stateCallback = std::move(callback);
}

void KeyBinding::serializeAll(nlohmann::json& j)
{
    for (const auto& [id, binding] : s_bindings) {
        // 只保存非默认值
        if (!binding->isDefault()) {
            j[binding->m_id] = binding->m_currentKey;
        }
    }
}

void KeyBinding::deserializeAll(const nlohmann::json& j)
{
    for (auto& [id, binding] : s_bindings) {
        if (j.contains(id) && j[id].is_number_integer()) {
            binding->setKey(j[id].get<i32>());
        }
    }
}

void KeyBinding::_registerBinding()
{
    if (m_id.empty()) return;

    // 检查是否已存在
    if (s_bindings.find(m_id) != s_bindings.end()) {
        spdlog::warn("Key binding '{}' already registered, replacing", m_id);
    }

    s_bindings[m_id] = this;
    s_categoryBindings[m_category].push_back(this);
}

void KeyBinding::_unregisterBinding()
{
    if (m_id.empty()) return;

    // 从绑定表移除
    s_bindings.erase(m_id);

    // 从分类表移除
    auto it = s_categoryBindings.find(m_category);
    if (it != s_categoryBindings.end()) {
        auto& bindings = it->second;
        bindings.erase(std::remove(bindings.begin(), bindings.end(), this), bindings.end());
        if (bindings.empty()) {
            s_categoryBindings.erase(it);
        }
    }
}

} // namespace mc
