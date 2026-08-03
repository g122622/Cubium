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

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本执行标志
 */
enum class EvalFlags : u32 {
    None = 0,
    Strict = 1 << 0, // 严格模式
    Module = 1 << 1, // ES6模块
    Global = 1 << 2, // 全局作用域
};

inline EvalFlags operator|(EvalFlags a, EvalFlags b)
{
    return static_cast<EvalFlags>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline bool operator&(EvalFlags a, EvalFlags b)
{
    return (static_cast<u32>(a) & static_cast<u32>(b)) != 0;
}

/**
 * @brief 脚本值类型
 *
 * 表示从脚本引擎返回的值的类型
 */
enum class ScriptValueType {
    Undefined,
    Null,
    Boolean,
    Number,
    String,
    Object,
    Array,
    Function,
};

/**
 * @brief 脚本值
 *
 * 表示从脚本引擎返回的值，支持多种类型
 */
class ScriptValue {
public:
    ScriptValue()
        : m_type(ScriptValueType::Undefined)
    {}
    explicit ScriptValue(std::nullptr_t)
        : m_type(ScriptValueType::Null)
    {}
    explicit ScriptValue(bool val)
        : m_type(ScriptValueType::Boolean)
        , m_data(val)
    {}
    explicit ScriptValue(f64 val)
        : m_type(ScriptValueType::Number)
        , m_data(val)
    {}
    explicit ScriptValue(i32 val)
        : m_type(ScriptValueType::Number)
        , m_data(static_cast<f64>(val))
    {}
    explicit ScriptValue(std::string val)
        : m_type(ScriptValueType::String)
        , m_data(std::move(val))
    {}

    [[nodiscard]] ScriptValueType type() const { return m_type; }
    [[nodiscard]] bool isUndefined() const { return m_type == ScriptValueType::Undefined; }
    [[nodiscard]] bool isNull() const { return m_type == ScriptValueType::Null; }
    [[nodiscard]] bool isBoolean() const { return m_type == ScriptValueType::Boolean; }
    [[nodiscard]] bool isNumber() const { return m_type == ScriptValueType::Number; }
    [[nodiscard]] bool isString() const { return m_type == ScriptValueType::String; }
    [[nodiscard]] bool isObject() const { return m_type == ScriptValueType::Object; }
    [[nodiscard]] bool isArray() const { return m_type == ScriptValueType::Array; }

    [[nodiscard]] bool asBoolean() const { return std::get<bool>(m_data); }
    [[nodiscard]] f64 asNumber() const { return std::get<f64>(m_data); }
    [[nodiscard]] const std::string& asString() const { return std::get<std::string>(m_data); }

private:
    ScriptValueType m_type;
    std::variant<std::monostate, bool, f64, std::string> m_data;
};

/**
 * @brief 脚本执行结果
 */
class ScriptResult {
public:
    static ScriptResult ok(ScriptValue value = ScriptValue()) { return ScriptResult(std::move(value), true, ""); }

    static ScriptResult error(std::string message) { return ScriptResult(ScriptValue(), false, std::move(message)); }

    [[nodiscard]] bool success() const { return m_success; }
    [[nodiscard]] const std::string& errorMessage() const { return m_errorMessage; }
    [[nodiscard]] const ScriptValue& value() const { return m_value; }

    [[nodiscard]] ScriptValue takeValue() { return std::move(m_value); }

private:
    ScriptResult(ScriptValue value, bool success, std::string errorMessage)
        : m_value(std::move(value))
        , m_success(success)
        , m_errorMessage(std::move(errorMessage))
    {}

    ScriptValue m_value;
    bool m_success;
    std::string m_errorMessage;
};

} // namespace mc::mod::bedrock::addon
