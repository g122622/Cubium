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

#include "Types.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <string_view>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 错误类型
// ============================================================================

/**
 * @brief 错误代码枚举
 */
enum class ErrorCode : i32 {
    Success = 0,

    // 通用错误
    Unknown = -1,
    InvalidArgument = -2,
    NullPointer = -3,
    OutOfRange = -4,
    Overflow = -5,
    OutOfBounds = -6,
    InvalidState = -7,
    InvalidData = -8,
    NotInitialized = -9,

    // 资源错误
    NotFound = -100,
    AlreadyExists = -101,
    ResourceExhausted = -102,
    OutOfMemory = -103,

    // 文件错误
    FileNotFound = -200,
    FileOpenFailed = -201,
    FileReadFailed = -202,
    FileWriteFailed = -203,
    FileCorrupted = -204,
    DecompressionFailed = -205,
    CompressionFailed = -206,

    // 网络错误
    ConnectionFailed = -300,
    ConnectionClosed = -301,
    ConnectionTimeout = -302,
    InvalidPacket = -303,
    ProtocolError = -304,

    // 游戏错误
    InvalidBlock = -400,
    InvalidItem = -401,
    InvalidEntity = -402,
    InvalidPlayer = -403,
    InvalidWorld = -404,

    // 渲染错误
    InitializationFailed = -600,
    OperationFailed = -601,
    CapacityExceeded = -602,
    Unsupported = -603,

    // 权限错误
    PermissionDenied = -500,
    Unauthorized = -501,

    // 资源包错误
    ResourcePackNotFound = -700,
    ResourcePackInvalid = -701,
    ResourceNotFound = -702,
    ResourceParseError = -703,
    TextureLoadFailed = -704,
    TextureAtlasFull = -705,
    ModelNotFound = -706,
    BlockStateNotFound = -707,

    // 存档错误 (1000-1099)
    WorldNotFound = -1000,
    WorldCorrupted = -1001,
    WorldLocked = -1002,
    WorldIncompatible = -1003,
    ChunkNotFound = -1004,
    ChunkCorrupted = -1005,
    ChunkSaveFailed = -1006,
    ChunkLoadFailed = -1007,
    SnapshotNotFound = -1008,
    SnapshotCorrupted = -1009,
    SnapshotCreateFailed = -1010,
    SnapshotRestoreFailed = -1011,
    ImportFailed = -1012,
    ExportFailed = -1013,
    RocksDBError = -1014,
    VersionTooNew = -1015,
    ChecksumMismatch = -1016
};

/**
 * @brief 错误信息类
 */
class Error {
public:
    Error() = default;

    Error(ErrorCode code, std::string_view message = "", std::string_view source = "")
        : m_code(code)
        , m_message(message)
        , m_source(source)
    {
        // auto str = toString();
        // spdlog::error("[Error] {}", str);
    }

    Error(ErrorCode code, const char* message, const char* source = "")
        : m_code(code)
        , m_message(message)
        , m_source(source)
    {
        // auto str = toString();
        // spdlog::error("[Error] {}", str);
    }

    Error(ErrorCode code, std::string message, std::string source = "")
        : m_code(code)
        , m_message(std::move(message))
        , m_source(std::move(source))
    {
        // auto str = toString();
        // spdlog::error("[Error] {}", str);
    }

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] const std::string& source() const noexcept { return m_source; }

    [[nodiscard]] bool success() const noexcept { return m_code == ErrorCode::Success; }

    [[nodiscard]] bool failed() const noexcept { return m_code != ErrorCode::Success; }

    [[nodiscard]] std::string toString() const
    {
        if (m_source.empty()) {
            return formatError();
        }
        return m_source + ": " + formatError();
    }

    // 静态工厂方法
    static Error ok() { return Error(ErrorCode::Success); }

    static Error unknown(std::string_view message = "") { return Error(ErrorCode::Unknown, message); }

    static Error invalidArgument(std::string_view message = "") { return Error(ErrorCode::InvalidArgument, message); }

    static Error notFound(std::string_view message = "") { return Error(ErrorCode::NotFound, message); }

    static Error fileNotFound(std::string_view path = "") { return Error(ErrorCode::FileNotFound, path); }

    static Error connectionFailed(std::string_view message = "") { return Error(ErrorCode::ConnectionFailed, message); }

private:
    [[nodiscard]] std::string formatError() const
    {
        if (m_message.empty()) {
            return std::string("[Error ") + std::to_string(static_cast<i32>(m_code)) + "]";
        }
        return std::string("[Error ") + std::to_string(static_cast<i32>(m_code)) + "] " + m_message;
    }

    ErrorCode m_code = ErrorCode::Success;
    std::string m_message;
    std::string m_source;
};

// ============================================================================
// Result 类型
// ============================================================================

/**
 * @brief 结果类型 - 用于错误处理
 * @tparam T 成功时的值类型
 *
 * 使用示例:
 * @code
 * Result<int> divide(int a, int b) {
 *     if (b == 0) {
 *         return Error(ErrorCode::InvalidArgument, "Division by zero");
 *     }
 *     return a / b;
 * }
 *
 * auto result = divide(10, 2);
 * if (result.success()) {
 *     std::cout << "Result: " << result.value() << std::endl;
 * } else {
 *     std::cerr << result.error().toString() << std::endl;
 * }
 * @endcode
 */
template <typename T>
class Result {
public:
    // 构造函数
    Result() = delete;

    template <typename U,
        typename = std::enable_if_t<std::is_constructible_v<T, U&&> && !std::is_same_v<std::decay_t<U>, Result> &&
            !std::is_same_v<std::decay_t<U>, Error>>>
    Result(U&& value) // NOLINT: 允许隐式转换
        : m_value(std::in_place, std::forward<U>(value))
        , m_success(true)
    {}

    Result(Error error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    // 拷贝和移动
    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept { return m_success; }

    [[nodiscard]] bool failed() const noexcept { return !success(); }

    [[nodiscard]] explicit operator bool() const noexcept { return success(); }

    // 获取值
    [[nodiscard]] T& value() &
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return *m_value;
    }

    [[nodiscard]] const T& value() const&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return *m_value;
    }

    [[nodiscard]] T&& value() &&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return std::move(*m_value);
    }

    // 获取值或默认值
    [[nodiscard]] T valueOr(T defaultValue) const& { return success() ? value() : std::move(defaultValue); }

    [[nodiscard]] T valueOr(T defaultValue) && { return success() ? std::move(value()) : std::move(defaultValue); }

    // 获取错误
    [[nodiscard]] const Error& error() const noexcept { return m_success ? m_successError : m_error; }

    // 转换操作
    template <typename U>
    [[nodiscard]] Result<U> map(std::function<U(const T&)> f) const&
    {
        if (success()) {
            return f(value());
        }
        return error();
    }

    template <typename U>
    [[nodiscard]] Result<U> map(std::function<U(T)> f) &&
    {
        if (success()) {
            return f(std::move(value()));
        }
        return error();
    }

private:
    Error m_error;
    std::optional<T> m_value;
    bool m_success = false;
    static inline Error m_successError{ErrorCode::Success};
};

// ============================================================================
// Result<void> 特化
// ============================================================================

template <>
class Result<void> {
public:
    Result()
        : m_success(true)
    {}

    Result(Error error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    // 拷贝和移动
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept { return m_success; }
    [[nodiscard]] bool failed() const noexcept { return !m_success; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_success; }

    // 获取错误
    [[nodiscard]] const Error& error() const noexcept { return m_error; }

    // 静态工厂方法
    static Result ok() { return Result(); }

private:
    Error m_error;
    bool m_success;
};

// ============================================================================
// Result<std::unique_ptr<T>> 特化
// ============================================================================

template <typename T, typename Deleter>
class Result<std::unique_ptr<T, Deleter>> {
public:
    Result() = delete;

    Result(std::unique_ptr<T, Deleter>&& value) // NOLINT: 允许隐式转换
        : m_value(value.release())
        , m_deleter(std::move(value.get_deleter()))
        , m_success(true)
    {}

    Result(Error error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) noexcept = default;

    [[nodiscard]] bool success() const noexcept { return m_success; }
    [[nodiscard]] bool failed() const noexcept { return !m_success; }
    [[nodiscard]] explicit operator bool() const noexcept { return m_success; }

    [[nodiscard]] std::unique_ptr<T, Deleter> value() &
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return takeValue();
    }

    [[nodiscard]] std::unique_ptr<T, Deleter> value() const&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return const_cast<Result*>(this)->takeValue();
    }

    [[nodiscard]] std::unique_ptr<T, Deleter> value() &&
    {
        if (failed()) {
            throw std::runtime_error("Result contains error: " + error().toString());
        }
        return takeValue();
    }

    [[nodiscard]] const Error& error() const noexcept { return m_success ? m_successError : m_error; }

private:
    [[nodiscard]] std::unique_ptr<T, Deleter> takeValue()
    {
        T* value = m_value;
        m_value = nullptr;
        return std::unique_ptr<T, Deleter>(value, m_deleter);
    }

    Error m_error;
    T* m_value = nullptr;
    Deleter m_deleter{};
    bool m_success = false;
    static inline Error m_successError{ErrorCode::Success};
};

// ============================================================================
// 辅助宏
// ============================================================================

/**
 * @brief TRY宏 - 简化错误传播
 *
 * 使用示例:
 * @code
 * Result<int> foo() { ... }
 *
 * Result<void> bar() {
 *     TRY(auto value, foo());
 *     // 使用 value
 *     return Result<void>::ok();
 * }
 * @endcode
 */
#define MC_TRY(expr)                \
    do {                            \
        auto _result = (expr);      \
        if (_result.failed()) {     \
            return _result.error(); \
        }                           \
    } while (0)

#define MC_TRY_ASSIGN(var, expr)          \
    do {                                  \
        auto _result = (expr);            \
        if (_result.failed()) {           \
            return _result.error();       \
        }                                 \
        var = std::move(_result.value()); \
    } while (0)

} // namespace mc
