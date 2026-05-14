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
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

#include "common/perfetto/PerfettoManager.hpp"

#ifdef _WIN32
// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
// clang-format on
#pragma comment(lib, "dbghelp.lib")
#elif defined(__linux__) || defined(__APPLE__)
#include <cxxabi.h>
#include <execinfo.h>
#include <unistd.h>
#endif

namespace mc::assert {

// ============================================================================
// AssertManager
// ============================================================================

AssertManager& AssertManager::instance()
{
    static AssertManager instance;
    return instance;
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
    std::ostringstream oss;
    oss << "Stack trace:\n";

#ifdef _WIN32
    // Windows 堆栈跟踪
    constexpr i32 MAX_FRAMES = 64;
    void* stack[MAX_FRAMES];
    USHORT frames = CaptureStackBackTrace(2, MAX_FRAMES, stack, nullptr);

    HANDLE process = GetCurrentProcess();
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + 256));
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement;

    SymInitialize(process, nullptr, TRUE);

    for (USHORT i = 0; i < frames; ++i) {
        DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

        if (SymFromAddr(process, address, nullptr, symbol)) {
            oss << "  [" << std::setw(2) << i << "] " << symbol->Name;

            if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
                oss << " at " << line.FileName << ":" << line.LineNumber;
            }

            oss << "\n";
        }
    }

    SymCleanup(process);
    free(symbol);

#elif defined(__linux__) || defined(__APPLE__)
    // Linux/macOS 堆栈跟踪
    constexpr i32 MAX_FRAMES = 64;
    void* buffer[MAX_FRAMES];
    int frames = backtrace(buffer, MAX_FRAMES);
    char** symbols = backtrace_symbols(buffer, frames);

    if (symbols) {
        for (int i = 2; i < frames; ++i) {
            oss << "  [" << std::setw(2) << (i - 2) << "] ";

            // 尝试解码 C++ 符号
            std::string sym(symbols[i]);

#ifdef __GNUC__
            // 尝试使用 abi::__cxa_demangle 解码
            size_t start = sym.find('(');
            size_t end = sym.find('+', start);

            if (start != std::string::npos && end != std::string::npos) {
                std::string mangled = sym.substr(start + 1, end - start - 1);
                int status = 0;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);

                if (status == 0 && demangled) {
                    oss << demangled;
                    free(demangled);
                } else {
                    oss << sym;
                }
            } else {
                oss << sym;
            }
#else
            oss << sym;
#endif
            oss << "\n";
        }
        free(symbols);
    }
#endif

    return oss.str();
}

// ============================================================================
// 默认断言处理器
// ============================================================================

[[noreturn]] void defaultAssertHandler(const AssertFailure& failure)
{
    std::cerr << "\n";
    std::cerr << "========================================\n";
    std::cerr << "            ASSERTION FAILED            \n";
    std::cerr << "========================================\n";
    std::cerr << "\n";
    std::cerr << "Expression: " << failure.expression << "\n";

    if (!failure.message.empty()) {
        std::cerr << "Message:    " << failure.message << "\n";
    }

    std::cerr << "\n";
    std::cerr << "Location:   " << failure.file << ":" << failure.line << "\n";
    std::cerr << "Function:   " << failure.function << "\n";

    if (!failure.stackTrace.empty()) {
        std::cerr << "\n" << failure.stackTrace << "\n";
    }

    std::cerr << "========================================\n";
    std::cerr << std::flush;

    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    std::cout << "Perfetto tracing stopped due to assertion failure" << std::endl;

    std::abort();
}

// ============================================================================
// 日志断言处理器
// ============================================================================

[[noreturn]] void logAssertHandler(const AssertFailure& failure)
{
    // 如果 spdlog 可用，使用它；否则回退到 stderr
    // 这里简单使用 stderr，实际项目中可以集成 spdlog
    std::cerr << "[ASSERT] Assertion failed: " << failure.expression;

    if (!failure.message.empty()) {
        std::cerr << " - " << failure.message;
    }

    std::cerr << " at " << failure.file << ":" << failure.line << " in " << failure.function << std::endl;

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

} // namespace detail

} // namespace mc::assert
