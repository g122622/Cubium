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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

#include <cstddef>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// ============================================================================
// 前向声明
// ============================================================================

class SettingsBase;

// ============================================================================
// 设置相关类型定义
// ============================================================================

/**
 * @brief 设置值类型
 *
 * 支持的设置值类型，使用 std::variant 存储。
 */
using SettingsValue = std::variant<bool, i32, f32, std::string>;

/**
 * @brief 设置变更回调模板
 *
 * 当设置值变更时调用的回调函数类型。
 * @tparam T 设置值的类型
 */
template <typename T>
using SettingsCallback = std::function<void(T newValue)>;

// ============================================================================
// IOption - 选项基类接口
// ============================================================================

/**
 * @brief 设置选项基类接口
 *
 * 所有设置选项的抽象基类，定义了选项的基本操作接口。
 * 每个选项具有唯一的键名，支持序列化/反序列化和变更通知。
 */
class IOption {
public:
    virtual ~IOption() = default;

    /**
     * @brief 获取选项的唯一键名
     * @return 键名字符串，如 "renderDistance"
     */
    [[nodiscard]] virtual std::string getKey() const = 0;

    /**
     * @brief 获取当前值
     * @return 当前值的 variant 形式
     */
    [[nodiscard]] virtual SettingsValue getValue() const = 0;

    /**
     * @brief 设置值
     * @param value 新值（variant 形式）
     * @return 是否设置成功，失败表示类型不匹配
     */
    virtual bool setValue(const SettingsValue& value) = 0;

    /**
     * @brief 序列化到 JSON 对象
     * @param j JSON 对象引用
     */
    virtual void serialize(nlohmann::json& j) const = 0;

    /**
     * @brief 从 JSON 对象反序列化
     * @param j JSON 对象引用
     */
    virtual void deserialize(const nlohmann::json& j) = 0;

    /**
     * @brief 重置为默认值
     */
    virtual void reset() = 0;

    /**
     * @brief 检查值是否为默认值
     */
    [[nodiscard]] virtual bool isDefault() const = 0;
};

// ============================================================================
// BooleanOption - 布尔选项
// ============================================================================

/**
 * @brief 布尔类型设置选项
 *
 * 用于表示开关类型的设置，如 "fullscreen"、"vsync" 等。
 *
 * 使用示例:
 * @code
 * BooleanOption fullscreen{false};  // 默认关闭
 * fullscreen.set(true);
 * if (fullscreen.get()) { ... }
 * fullscreen.onChange([](bool value) {
 *     spdlog::info("Fullscreen changed to: {}", value);
 * });
 * @endcode
 */
class BooleanOption : public IOption {
public:
    /**
     * @brief 构造布尔选项
     * @param key 选项键名
     * @param defaultValue 默认值
     */
    BooleanOption(std::string key, bool defaultValue = false)
        : m_key(std::move(key))
        , m_value(defaultValue)
        , m_default(defaultValue)
    {}

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override { return m_value; }

    bool setValue(const SettingsValue& value) override
    {
        if (std::holds_alternative<bool>(value)) {
            set(std::get<bool>(value));
            return true;
        }
        return false;
    }

    void serialize(nlohmann::json& j) const override { j[m_key] = m_value; }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key) && j[m_key].is_boolean()) {
            set(j[m_key].get<bool>());
        }
    }

    void reset() override { set(m_default); }

    [[nodiscard]] bool isDefault() const override { return m_value == m_default; }

    // 便捷方法

    /**
     * @brief 获取当前布尔值
     */
    [[nodiscard]] bool get() const { return m_value; }

    /**
     * @brief 设置布尔值
     * @param value 新值
     */
    void set(bool value)
    {
        if (m_value != value) {
            m_value = value;
            if (m_callback) {
                m_callback(value);
            }
        }
    }

    /**
     * @brief 隐式转换为布尔值
     */
    [[nodiscard]] operator bool() const { return m_value; }

    /**
     * @brief 设置变更回调
     * @param callback 变更时调用的函数
     */
    void onChange(SettingsCallback<bool> callback) { m_callback = std::move(callback); }

private:
    std::string m_key;
    bool m_value;
    bool m_default;
    SettingsCallback<bool> m_callback;
};

// ============================================================================
// RangeOption - 整数范围选项
// ============================================================================

/**
 * @brief 整数范围设置选项
 *
 * 用于表示有范围限制的整数设置，如 "renderDistance"、"fpsLimit" 等。
 * 支持最小值、最大值和默认值约束。
 *
 * 使用示例:
 * @code
 * RangeOption renderDistance{"renderDistance", 2, 32, 12};  // 2-32，默认12
 * renderDistance.set(16);
 * int distance = renderDistance.get();
 * renderDistance.onChange([](int value) {
 *     spdlog::info("Render distance changed to: {}", value);
 * });
 * @endcode
 */
class RangeOption : public IOption {
public:
    /**
     * @brief 构造整数范围选项
     * @param key 选项键名
     * @param min 最小值
     * @param max 最大值
     * @param defaultValue 默认值
     */
    RangeOption(std::string key, i32 min, i32 max, i32 defaultValue)
        : m_key(std::move(key))
        , m_min(min)
        , m_max(max)
        , m_value(clamp(defaultValue))
        , m_default(clamp(defaultValue))
    {}

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override { return m_value; }

    bool setValue(const SettingsValue& value) override
    {
        if (std::holds_alternative<i32>(value)) {
            set(std::get<i32>(value));
            return true;
        }
        // 尝试从浮点数转换
        if (std::holds_alternative<f32>(value)) {
            set(static_cast<i32>(std::get<f32>(value)));
            return true;
        }
        return false;
    }

    void serialize(nlohmann::json& j) const override { j[m_key] = m_value; }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key)) {
            if (j[m_key].is_number_integer()) {
                set(j[m_key].get<i32>());
            } else if (j[m_key].is_number_unsigned()) {
                set(static_cast<i32>(j[m_key].get<u32>()));
            }
        }
    }

    void reset() override { set(m_default); }

    [[nodiscard]] bool isDefault() const override { return m_value == m_default; }

    // 便捷方法

    /**
     * @brief 获取当前整数值
     */
    [[nodiscard]] i32 get() const { return m_value; }

    /**
     * @brief 设置整数值
     * @param value 新值（自动 clamp 到有效范围）
     */
    void set(i32 value)
    {
        i32 newValue = clamp(value);
        if (m_value != newValue) {
            m_value = newValue;
            if (m_callback) {
                m_callback(newValue);
            }
        }
    }

    /**
     * @brief 隐式转换为整数
     */
    [[nodiscard]] operator i32() const { return m_value; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] i32 min() const { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] i32 max() const { return m_max; }

    /**
     * @brief 获取默认值
     */
    [[nodiscard]] i32 defaultValue() const { return m_default; }

    /**
     * @brief 设置变更回调
     */
    void onChange(SettingsCallback<i32> callback) { m_callback = std::move(callback); }

private:
    std::string m_key;
    i32 m_min;
    i32 m_max;
    i32 m_value;
    i32 m_default;
    SettingsCallback<i32> m_callback;

    [[nodiscard]] i32 clamp(i32 value) const
    {
        if (value < m_min) return m_min;
        if (value > m_max) return m_max;
        return value;
    }
};

// ============================================================================
// FloatOption - 浮点数选项
// ============================================================================

/**
 * @brief 浮点数设置选项
 *
 * 用于表示有范围限制的浮点数设置，如 "mouseSensitivity"、"fov" 等。
 * 支持最小值、最大值和默认值约束。
 *
 * 使用示例:
 * @code
 * FloatOption sensitivity{"mouseSensitivity", 0.0f, 1.0f, 0.5f};
 * sensitivity.set(0.7f);
 * float sens = sensitivity.get();
 * @endcode
 */
class FloatOption : public IOption {
public:
    /**
     * @brief 构造浮点数选项
     * @param key 选项键名
     * @param min 最小值
     * @param max 最大值
     * @param defaultValue 默认值
     */
    FloatOption(std::string key, f32 min, f32 max, f32 defaultValue)
        : m_key(std::move(key))
        , m_min(min)
        , m_max(max)
        , m_value(clamp(defaultValue))
        , m_default(clamp(defaultValue))
    {}

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override { return m_value; }

    bool setValue(const SettingsValue& value) override
    {
        if (std::holds_alternative<f32>(value)) {
            set(std::get<f32>(value));
            return true;
        }
        // 尝试从整数转换
        if (std::holds_alternative<i32>(value)) {
            set(static_cast<f32>(std::get<i32>(value)));
            return true;
        }
        return false;
    }

    void serialize(nlohmann::json& j) const override { j[m_key] = m_value; }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key) && j[m_key].is_number()) {
            set(j[m_key].get<f32>());
        }
    }

    void reset() override { set(m_default); }

    [[nodiscard]] bool isDefault() const override
    {
        // 使用 epsilon 比较浮点数
        constexpr f32 epsilon = 0.0001f;
        return std::abs(m_value - m_default) < epsilon;
    }

    // 便捷方法

    /**
     * @brief 获取当前浮点值
     */
    [[nodiscard]] f32 get() const { return m_value; }

    /**
     * @brief 设置浮点值
     * @param value 新值（自动 clamp 到有效范围）
     */
    void set(f32 value)
    {
        f32 newValue = clamp(value);
        // 使用 epsilon 比较
        constexpr f32 epsilon = 0.0001f;
        if (std::abs(m_value - newValue) > epsilon) {
            m_value = newValue;
            if (m_callback) {
                m_callback(newValue);
            }
        }
    }

    /**
     * @brief 隐式转换为浮点数
     */
    [[nodiscard]] operator f32() const { return m_value; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] f32 min() const { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] f32 max() const { return m_max; }

    /**
     * @brief 获取默认值
     */
    [[nodiscard]] f32 defaultValue() const { return m_default; }

    /**
     * @brief 设置变更回调
     */
    void onChange(SettingsCallback<f32> callback) { m_callback = std::move(callback); }

private:
    std::string m_key;
    f32 m_min;
    f32 m_max;
    f32 m_value;
    f32 m_default;
    SettingsCallback<f32> m_callback;

    [[nodiscard]] f32 clamp(f32 value) const
    {
        if (value < m_min) return m_min;
        if (value > m_max) return m_max;
        return value;
    }
};

// ============================================================================
// EnumOption - 枚举选项
// ============================================================================

/**
 * @brief 枚举类型设置选项
 *
 * 用于表示从预定义值列表中选择的设置，如 "graphicsMode"、"cloudMode" 等。
 * 模板参数 T 为底层整数类型（如 u8, i32）。
 *
 * 使用示例:
 * @code
 * enum class GraphicsMode : u8 { Fast, Fancy };
 *
 * EnumOption<u8> graphics{"graphicsMode", {0, 1}, 1, {"fast", "fancy"}};
 * graphics.set(0);
 * graphics.setByName("fancy");
 * const char* name = graphics.getName();  // "fancy"
 * @endcode
 */
template <typename T>
class EnumOption : public IOption {
    static_assert(std::is_integral_v<T>, "T must be an integral type");

public:
    /**
     * @brief 构造枚举选项
     * @param key 选项键名
     * @param values 允许的值列表
     * @param defaultValue 默认值
     * @param names 值对应的名称列表（必须与 values 一一对应）
     */
    EnumOption(std::string key, std::vector<T> values, T defaultValue, std::vector<std::string> names)
        : m_key(std::move(key))
        , m_values(std::move(values))
        , m_names(std::move(names))
        , m_value(defaultValue)
        , m_default(defaultValue)
    {
        // 构建名称到值的映射
        for (size_t i = 0; i < m_values.size() && i < m_names.size(); ++i) {
            m_nameToValue[m_names[i]] = m_values[i];
        }
    }

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override { return static_cast<i32>(m_value); }

    bool setValue(const SettingsValue& value) override
    {
        if (std::holds_alternative<i32>(value)) {
            set(static_cast<T>(std::get<i32>(value)));
            return true;
        }
        return false;
    }

    void serialize(nlohmann::json& j) const override { j[m_key] = getName(); }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key)) {
            if (j[m_key].is_string()) {
                setByName(j[m_key].template get<std::string>());
            } else if (j[m_key].is_number()) {
                set(static_cast<T>(j[m_key].template get<i32>()));
            }
        }
    }

    void reset() override { set(m_default); }

    [[nodiscard]] bool isDefault() const override { return m_value == m_default; }

    // 便捷方法

    /**
     * @brief 获取当前枚举值
     */
    [[nodiscard]] T get() const { return m_value; }

    /**
     * @brief 设置枚举值
     * @param value 新值（必须是允许值列表中的值）
     */
    void set(T value)
    {
        // 验证值是否有效
        bool valid = false;
        for (const auto& v : m_values) {
            if (v == value) {
                valid = true;
                break;
            }
        }
        if (!valid) return;

        if (m_value != value) {
            m_value = value;
            if (m_callback) {
                m_callback(value);
            }
        }
    }

    /**
     * @brief 通过名称设置值
     * @param name 值名称
     * @return 是否设置成功
     */
    bool setByName(const std::string& name)
    {
        auto it = m_nameToValue.find(name);
        if (it != m_nameToValue.end()) {
            set(it->second);
            return true;
        }
        return false;
    }

    /**
     * @brief 获取当前值的名称
     * @return 值名称，如果找不到则返回空字符串
     */
    [[nodiscard]] std::string getName() const
    {
        for (size_t i = 0; i < m_values.size() && i < m_names.size(); ++i) {
            if (m_values[i] == m_value) {
                return m_names[i];
            }
        }
        return "";
    }

    /**
     * @brief 隐式转换为枚举值
     */
    [[nodiscard]] operator T() const { return m_value; }

    /**
     * @brief 获取所有允许的值
     */
    [[nodiscard]] const std::vector<T>& values() const { return m_values; }

    /**
     * @brief 获取所有值名称
     */
    [[nodiscard]] const std::vector<std::string>& names() const { return m_names; }

    /**
     * @brief 设置变更回调
     */
    void onChange(SettingsCallback<T> callback) { m_callback = std::move(callback); }

private:
    std::string m_key;
    std::vector<T> m_values;
    std::vector<std::string> m_names;
    std::map<std::string, T> m_nameToValue;
    T m_value;
    T m_default;
    SettingsCallback<T> m_callback;
};

// ============================================================================
// StringOption - 字符串选项
// ============================================================================

/**
 * @brief 字符串类型设置选项
 *
 * 用于表示文本类型的设置，如 "username"、"serverAddress" 等。
 */
class StringOption : public IOption {
public:
    /**
     * @brief 构造字符串选项
     * @param key 选项键名
     * @param defaultValue 默认值
     */
    StringOption(std::string key, std::string defaultValue = "")
        : m_key(std::move(key))
        , m_value(std::move(defaultValue))
        , m_default(m_value)
    {}

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override { return m_value; }

    bool setValue(const SettingsValue& value) override
    {
        if (std::holds_alternative<std::string>(value)) {
            set(std::get<std::string>(value));
            return true;
        }
        return false;
    }

    void serialize(nlohmann::json& j) const override { j[m_key] = m_value; }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key) && j[m_key].is_string()) {
            set(j[m_key].get<std::string>());
        }
    }

    void reset() override { set(m_default); }

    [[nodiscard]] bool isDefault() const override { return m_value == m_default; }

    // 便捷方法

    /**
     * @brief 获取当前字符串值
     */
    [[nodiscard]] const std::string& get() const { return m_value; }

    /**
     * @brief 设置字符串值
     */
    void set(std::string value)
    {
        if (m_value != value) {
            m_value = std::move(value);
            if (m_callback) {
                m_callback(m_value);
            }
        }
    }

    /**
     * @brief 隐式转换为字符串视图
     */
    [[nodiscard]] operator std::string_view() const { return m_value; }

    /**
     * @brief 获取默认值
     */
    [[nodiscard]] const std::string& defaultValue() const { return m_default; }

    /**
     * @brief 设置变更回调
     */
    void onChange(SettingsCallback<std::string> callback) { m_callback = std::move(callback); }

private:
    std::string m_key;
    std::string m_value;
    std::string m_default;
    SettingsCallback<std::string> m_callback;
};

} // namespace mc
