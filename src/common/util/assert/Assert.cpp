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

#include "Assert.hpp"
#include "CrashHandler.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

#include "common/core/Types.hpp"
#include "common/profiler/ProfilerManager.hpp"

#ifdef _WIN32
// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
// clang-format on
#endif

namespace mc::assert {

namespace {

std::mutex& getAssertOutputMutex()
{
    static std::mutex s_assertOutputMutex;
    return s_assertOutputMutex;
}

} // namespace

// ============================================================================
// AssertManager
// ============================================================================

AssertManager& AssertManager::instance()
{
    static AssertManager instance;
    return instance;
}

AssertManager::AssertManager()
{
    // 断言失败默认捕获调用栈，便于在没有调试器附加时定位失败根因
    // （与 CrashHandler 的崩溃栈输出对齐）。如需关闭（例如压测热路径），
    // 设置环境变量 MC_ASSERT_NO_STACK=1 即可全局禁用。
    m_config.captureStackTrace = !readEnvFlag("MC_ASSERT_NO_STACK");
}

bool AssertManager::readEnvFlag(const char* name) const
{
    if (name == nullptr) {
        return false;
    }
#ifdef _WIN32
    // 使用 GetEnvironmentVariableA 避免 UCRT getenv 的弃用警告（C4996/-Wdeprecated-declarations）
    char buffer[8] = {};
    const DWORD len = GetEnvironmentVariableA(name, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len == 0 || len >= sizeof(buffer)) {
        // 0 表示变量不存在或为空；>= buffer 表示值过长，非预期布尔标记
        return false;
    }
    const char* value = buffer;
#else
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return false;
    }
#endif
    return std::strcmp(value, "1") == 0 || std::strcmp(value, "true") == 0 || std::strcmp(value, "TRUE") == 0;
}

void AssertManager::setConfig(const AssertConfig& config)
{
    m_config = config;
}

void AssertManager::setHandler(AssertHandler handler)
{
    m_config.handler = std::move(handler);
}

void AssertManager::handleFailure(
    const char* expression, const char* message, const char* file, i32 line, const char* function, AssertLevel level)
{
    AssertFailure failure;
    failure.expression = expression ? expression : "";
    failure.message = message ? message : "";
    failure.file = file ? file : "";
    failure.line = line;
    failure.function = function ? function : "";
    failure.level = level;

    if (m_config.captureStackTrace) {
        failure.stackTrace = captureStackTrace();
    }

    // 触发调试器断点
    if (m_config.breakOnFailure) {
#ifdef _WIN32
        DebugBreak();
#else
        __builtin_trap();
#endif
    }

    // 使用自定义处理器
    if (m_config.handler) {
        m_config.handler(failure);
    } else {
        defaultAssertHandler(failure);
    }

    // 不应该到达这里
    std::abort();
}

bool AssertManager::handleRecoverableFailure(
    const char* expression, const char* message, const char* file, i32 line, const char* function)
{
    AssertFailure failure;
    failure.expression = expression ? expression : "";
    failure.message = message ? message : "";
    failure.file = file ? file : "";
    failure.line = line;
    failure.function = function ? function : "";
    failure.level = AssertLevel::Debug;

    if (m_config.captureStackTrace) {
        failure.stackTrace = captureStackTrace();
    }

    if (m_config.breakOnFailure) {
#ifdef _WIN32
        DebugBreak();
#else
        __builtin_trap();
#endif
    }

    if (m_config.handler) {
        m_config.handler(failure);
    } else {
        defaultAssertHandler(failure);
    }

    return m_config.continueExecution;
}

std::string AssertManager::captureStackTrace() const
{
    // 委托给 CrashHandler 的统一实现：使用 StackWalk64 + 一次性符号初始化，
    // 并尝试枚举每帧局部变量，质量优于 CaptureStackBackTrace 裸调用。
    // 不跳过任何帧（skipFrames=0），完整保留断言处理链与调用方栈帧。
    return "Stack trace:\n" + CrashHandler::captureStackTrace(0);
}

// ============================================================================
// 默认断言处理器
// ============================================================================

[[noreturn]] void defaultAssertHandler(const AssertFailure& failure)
{
    std::lock_guard<std::mutex> lock(getAssertOutputMutex());

    std::cerr << detail::formatFailureBlock(failure) << std::flush;

    auto& profilerManager = mc::profiler::ProfilerManager::instance();
    profilerManager.stopTracing();
    profilerManager.shutdown();
    std::cout << "Perfetto tracing stopped due to assertion failure" << std::endl;

    std::abort();
}

// ============================================================================
// 异常断言处理器
// ============================================================================

[[noreturn]] void throwAssertHandler(const AssertFailure& failure)
{
    throw AssertException(failure);
}

// ============================================================================
// AssertException
// ============================================================================

AssertException::AssertException(const AssertFailure& failure)
    : m_failure(failure)
{
    std::ostringstream oss;
    oss << "Assertion failed: " << failure.expression;

    if (!failure.message.empty()) {
        oss << " - " << failure.message;
    }

    oss << " at " << failure.file << ":" << failure.line << " in " << failure.function;

    m_what = oss.str();
}

// ============================================================================
// 帮助函数
// ============================================================================

namespace detail {

std::string formatFailureMessage(
    const char* expression, const char* message, const char* file, i32 line, const char* function)
{
    std::ostringstream oss;
    oss << "Assertion failed: " << (expression ? expression : "???");

    if (message && message[0] != '\0') {
        oss << "\n  Message: " << message;
    }

    oss << "\n  at " << (file ? file : "???") << ":" << line;
    oss << "\n  in " << (function ? function : "???");

    return oss.str();
}

std::string formatFailureBlock(const AssertFailure& failure)
{
    std::ostringstream oss;
    oss << "\n";
    oss << "========================================\n";
    oss << "            ASSERTION FAILED            \n";
    oss << "========================================\n";
    oss << "\n";
    oss << "Expression: " << failure.expression << "\n";

    if (!failure.message.empty()) {
        oss << "Message:    " << failure.message << "\n";
    }

    oss << "\n";
    oss << "Location:   " << failure.file << ":" << failure.line << "\n";
    oss << "Function:   " << failure.function << "\n";

    if (!failure.stackTrace.empty()) {
        oss << "\n" << failure.stackTrace;
        if (!failure.stackTrace.empty() && failure.stackTrace.back() != '\n') {
            oss << "\n";
        }
        oss << "\n";
    }

    oss << "========================================\n";
    return oss.str();
}

} // namespace detail

} // namespace mc::assert
