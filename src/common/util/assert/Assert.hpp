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

#include "../../core/Types.hpp"
#include <exception>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace mc::assert {

/**
 * @brief 断言级别
 */
enum class AssertLevel : u8 {
    Debug,   // 仅在 Debug 模式启用
    Release, // 始终启用
    Fatal    // 始终启用，失败时终止程序
};

/**
 * @brief 断言失败信息
 */
struct AssertFailure {
    std::string expression; // 断言表达式
    std::string message;    // 自定义消息
    std::string file;       // 文件名
    i32 line;               // 行号
    std::string function;   // 函数名
    AssertLevel level;      // 断言级别
    std::string stackTrace; // 堆栈跟踪（可选）
};

/**
 * @brief 断言处理器接口
 *
 * 自定义断言失败时的行为，例如：
 * - 记录日志
 * - 抛出异常
 * - 触发调试器断点
 * - 显示对话框
 */
using AssertHandler = std::function<void(const AssertFailure&)>;

/**
 * @brief 断言配置
 */
struct AssertConfig {
    AssertHandler handler;          // 自定义处理器
    bool captureStackTrace = true;  // 是否捕获堆栈跟踪（默认开启，与 CrashHandler 的崩溃栈输出对齐）
    bool breakOnFailure = false;    // 是否触发调试器断点
    bool throwException = false;    // 是否抛出异常
    bool continueExecution = false; // 是否继续执行（仅 Debug 级别）
};

/**
 * @brief 断言管理器
 */
class AssertManager {
public:
    /**
     * @brief 获取单例实例
     */
    static AssertManager& instance();

    /**
     * @brief 设置配置
     */
    void setConfig(const AssertConfig& config);

    /**
     * @brief 获取当前配置
     */
    [[nodiscard]] const AssertConfig& config() const { return m_config; }

    /**
     * @brief 设置自定义处理器
     */
    void setHandler(AssertHandler handler);

    /**
     * @brief 处理断言失败
     *
     * @param expression 断言表达式
     * @param message 自定义消息
     * @param file 文件名
     * @param line 行号
     * @param function 函数名
     * @param level 断言级别
     */
    [[noreturn]] void handleFailure(const char* expression,
        const char* message,
        const char* file,
        i32 line,
        const char* function,
        AssertLevel level);

    /**
     * @brief 处理可恢复的断言失败（仅 Debug 级别）
     *
     * @return 是否继续执行
     */
    bool handleRecoverableFailure(
        const char* expression, const char* message, const char* file, i32 line, const char* function);

    /**
     * @brief 捕获堆栈跟踪
     */
    [[nodiscard]] std::string captureStackTrace() const;

private:
    AssertManager();
    ~AssertManager() = default;

    // 读取 "0/1" 形式的布尔环境变量（值为 1/true/TRUE 视为真）
    bool readEnvFlag(const char* name) const;

    AssertConfig m_config;
};

/**
 * @brief 默认断言处理器
 *
 * 输出失败信息（含调用栈）到 stderr 并终止程序
 */
[[noreturn]] void defaultAssertHandler(const AssertFailure& failure);

/**
 * @brief 异常断言处理器
 *
 * 抛出 AssertException
 */
[[noreturn]] void throwAssertHandler(const AssertFailure& failure);

/**
 * @brief 断言异常
 */
class AssertException : public std::exception {
public:
    explicit AssertException(const AssertFailure& failure);
    [[nodiscard]] const char* what() const noexcept override { return m_what.c_str(); }
    [[nodiscard]] const AssertFailure& failure() const noexcept { return m_failure; }

private:
    AssertFailure m_failure;
    std::string m_what;
};

// ============================================================================
// 断言帮助函数
// ============================================================================

namespace detail {

/**
 * @brief 格式化断言失败消息
 */
std::string formatFailureMessage(
    const char* expression, const char* message, const char* file, i32 line, const char* function);

/**
 * @brief 格式化默认断言处理器输出块
 */
std::string formatFailureBlock(const AssertFailure& failure);

/**
 * @brief 值格式化帮助器
 */
template <typename T>
std::string formatValue(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/**
 * @brief 字符串值格式化（带引号）
 */
inline std::string formatValue(const char* value)
{
    if (value == nullptr) {
        return "nullptr";
    }
    return "\"" + std::string(value) + "\"";
}

inline std::string formatValue(const std::string& value)
{
    return "\"" + value + "\"";
}

/**
 * @brief 指针格式化
 */
template <typename T>
std::string formatValue(T* value)
{
    if (value == nullptr) {
        return "nullptr";
    }
    std::ostringstream oss;
    oss << static_cast<const void*>(value);
    return oss.str();
}

/**
 * @brief 布尔值格式化
 */
inline std::string formatValue(bool value)
{
    return value ? "true" : "false";
}

/**
 * @brief 格式化比较断言消息
 */
template <typename T, typename U>
std::string formatComparisonMessage(
    const char* op, const char* aName, const T& aValue, const char* bName, const U& bValue)
{
    std::ostringstream oss;
    oss << op << "\n"
        << "  " << aName << " = " << formatValue(aValue) << "\n"
        << "  " << bName << " = " << formatValue(bValue);
    return oss.str();
}

} // namespace detail

} // namespace mc::assert
