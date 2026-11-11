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

#include "common/util/assert/AssertMacros.hpp"

#include <functional>
#include <memory>
#include <optional>
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

    [[nodiscard]] ErrorCode code() const noexcept
    {
        return m_code;
    }

    [[nodiscard]] const std::string& message() const noexcept
    {
        return m_message;
    }

    [[nodiscard]] const std::string& source() const noexcept
    {
        return m_source;
    }

    [[nodiscard]] bool success() const noexcept
    {
        return m_code == ErrorCode::Success;
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return m_code != ErrorCode::Success;
    }

    [[nodiscard]] std::string toString() const
    {
        if (m_source.empty()) {
            return formatError();
        }
        return m_source + ": " + formatError();
    }

    // 静态工厂方法
    static Error ok()
    {
        return Error(ErrorCode::Success);
    }

    static Error unknown(std::string_view message = "")
    {
        return Error(ErrorCode::Unknown, message);
    }

    static Error invalidArgument(std::string_view message = "")
    {
        return Error(ErrorCode::InvalidArgument, message);
    }

    static Error notFound(std::string_view message = "")
    {
        return Error(ErrorCode::NotFound, message);
    }

    static Error fileNotFound(std::string_view path = "")
    {
        return Error(ErrorCode::FileNotFound, path);
    }

    static Error connectionFailed(std::string_view message = "")
    {
        return Error(ErrorCode::ConnectionFailed, message);
    }

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
 * Result<i32> divide(i32 a, i32 b) {
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
template <typename T, typename E = Error>
class Result {
public:
    // 构造函数
    Result() = delete;

    template <typename U,
        typename = std::enable_if_t<std::is_constructible_v<T, U&&> &&
            !std::is_same_v<std::decay_t<U>, Result> && !std::is_same_v<std::decay_t<U>, E> &&
            !std::is_convertible_v<U, E>>>
    Result(U&& value) // NOLINT: 允许隐式转换
        : m_value(std::in_place, std::forward<U>(value))
        , m_success(true)
    {}

    Result(E error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    /**
     * @brief 由「可转换为 E 的错误类型」构造失败结果
     *
     * 用于跨子系统传播：命令子系统返回 `Result<T, CommandError>`，内部调用返回
     * `Result<U>`(E=Error) 的函数时，`MC_TRY` 会把 `Error` 隐式转成 `CommandError`。
     */
    template <typename E2,
        typename = void,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<E2>, E> && std::is_convertible_v<E2, E>>>
    Result(E2&& error) // NOLINT: 允许隐式转换
        : m_error(std::forward<E2>(error))
        , m_success(false)
    {}

    // 拷贝和移动
    Result(const Result&) = delete;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = delete;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept
    {
        return m_success;
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return !success();
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return success();
    }

    // 获取值
    /// 取失败结果的 value() 属于调用方违约（必须先判 success()），此处断言崩溃并带上
    /// 原始错误信息，避免静默返回默认值或让异常越过 Result 边界。
    [[nodiscard]] T& value() &
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return *m_value;
    }

    [[nodiscard]] const T& value() const&
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return *m_value;
    }

    [[nodiscard]] T&& value() &&
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return std::move(*m_value);
    }

    // 获取值或默认值
    [[nodiscard]] T valueOr(T defaultValue) const&
    {
        return success() ? value() : std::move(defaultValue);
    }

    [[nodiscard]] T valueOr(T defaultValue) &&
    {
        return success() ? std::move(value()) : std::move(defaultValue);
    }

    // 获取错误
    [[nodiscard]] const E& error() const noexcept
    {
        return m_success ? m_successError : m_error;
    }

    // 转换操作
    template <typename U>
    [[nodiscard]] Result<U, E> map(std::function<U(const T&)> f) const&
    {
        if (success()) {
            return f(value());
        }
        return error();
    }

    template <typename U>
    [[nodiscard]] Result<U, E> map(std::function<U(T)> f) &&
    {
        if (success()) {
            return f(std::move(value()));
        }
        return error();
    }

private:
    E m_error{};
    std::optional<T> m_value;
    bool m_success = false;
    static inline E m_successError{};
};

// ============================================================================
// Result<void> 特化
// ============================================================================

template <typename E>
class Result<void, E> {
public:
    Result()
        : m_success(true)
    {}

    Result(E error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    template <typename E2,
        typename = void,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<E2>, E> && std::is_convertible_v<E2, E>>>
    Result(E2&& error) // NOLINT: 允许隐式转换
        : m_error(std::forward<E2>(error))
        , m_success(false)
    {}

    /**
     * @brief 由另一种错误类型的 `Result<void>` 转换
     *
     * 使得返回 `Result<void, 自定义错误>` 的函数里可以直接 `return Result<void>::ok();`
     * 或透传内层失败结果；错误类型按 `E2 -> E` 隐式转换。
     */
    template <typename E2,
        typename = void,
        typename = std::enable_if_t<!std::is_same_v<E2, E> && std::is_convertible_v<const E2&, E>>>
    Result(Result<void, E2>&& other) // NOLINT: 允许隐式转换
        : m_error(other.success() ? E{} : E(other.error()))
        , m_success(other.success())
    {}

    template <typename E2,
        typename = void,
        typename = std::enable_if_t<!std::is_same_v<E2, E> && std::is_convertible_v<const E2&, E>>>
    Result(const Result<void, E2>& other) // NOLINT: 允许隐式转换
        : m_error(other.success() ? E{} : E(other.error()))
        , m_success(other.success())
    {}

    // 拷贝和移动
    Result(const Result&) = default;
    Result(Result&&) noexcept = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) noexcept = default;

    // 查询状态
    [[nodiscard]] bool success() const noexcept
    {
        return m_success;
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return !m_success;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return m_success;
    }

    // 获取错误
    [[nodiscard]] const E& error() const noexcept
    {
        return m_error;
    }

    // 静态工厂方法
    static Result ok()
    {
        return Result();
    }

private:
    E m_error{};
    bool m_success;
};

// ============================================================================
// Result<std::unique_ptr<T>> 特化
// ============================================================================

template <typename T, typename Deleter, typename E>
class Result<std::unique_ptr<T, Deleter>, E> {
public:
    Result() = delete;

    Result(std::unique_ptr<T, Deleter>&& value) // NOLINT: 允许隐式转换
        : m_value(value.release())
        , m_deleter(std::move(value.get_deleter()))
        , m_success(true)
    {}

    /**
     * @brief 由派生类指针的 unique_ptr 构造成功结果
     *
     * 使 `Result<std::unique_ptr<Base>>` 能直接接住 `std::make_unique<Derived>()`，
     * 不必在每个返回点手写基类指针转换。派生类指针转基类指针失败时该构造函数不参与重载。
     */
    template <typename Derived,
        typename DerivedDeleter,
        typename = std::enable_if_t<std::is_convertible_v<Derived*, T*> && !std::is_same_v<Derived, T> &&
            std::is_constructible_v<Deleter, DerivedDeleter&&>>>
    Result(std::unique_ptr<Derived, DerivedDeleter>&& value) // NOLINT: 允许隐式转换
        : m_value(static_cast<T*>(value.release()))
        , m_deleter(value.get_deleter())
        , m_success(true)
    {}

    Result(E error) // NOLINT: 允许隐式转换
        : m_error(std::move(error))
        , m_success(false)
    {}

    template <typename E2,
        typename = void,
        typename = std::enable_if_t<!std::is_same_v<std::decay_t<E2>, E> && std::is_convertible_v<E2, E>>>
    Result(E2&& error) // NOLINT: 允许隐式转换
        : m_error(std::forward<E2>(error))
        , m_success(false)
    {}

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    // 移动语义须**手动实现**：本类以裸指针 m_value 持有对象，默认移动只会复制指针而
    // 不置空源对象，一旦有了析构函数就会双重释放。这里显式转移所有权并把源置空。
    Result(Result&& other) noexcept
        : m_error(std::move(other.m_error))
        , m_value(other.m_value)
        , m_deleter(std::move(other.m_deleter))
        , m_success(other.m_success)
    {
        other.m_value = nullptr;
    }

    Result& operator=(Result&& other) noexcept
    {
        if (this != &other) {
            if (m_value != nullptr) {
                m_deleter(m_value);
            }
            m_error = std::move(other.m_error);
            m_value = other.m_value;
            m_deleter = std::move(other.m_deleter);
            m_success = other.m_success;
            other.m_value = nullptr;
        }
        return *this;
    }

    /**
     * @brief 析构：释放尚未被 value() 取走的对象
     *
     * 【为什么必须有】value() 是**转移所有权**语义（takeValue 把 m_value 置空并把裸指针
     * 交给 unique_ptr）。若调用方只判 success() 而不取 value()（如 nbt_io::skipCompound
     * 只借 readCompound 推进游标、丢弃解析结果），没有析构函数就会**泄漏**整个对象。
     * fuzz 实测：fuzz_java_codec 因此报 LeakSanitizer「64 byte(s) leaked」，泄漏栈为
     * compound_tag::read ← skipCompound ← readComponentNbt。
     */
    ~Result()
    {
        if (m_value != nullptr) {
            m_deleter(m_value);
        }
    }

    [[nodiscard]] bool success() const noexcept
    {
        return m_success;
    }

    [[nodiscard]] bool failed() const noexcept
    {
        return !m_success;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return m_success;
    }

    /// 取失败结果的 value() 属于调用方违约（必须先判 success()），此处断言崩溃并带上
    /// 原始错误信息，避免静默返回空指针或让异常越过 Result 边界。
    [[nodiscard]] std::unique_ptr<T, Deleter> value() &
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return takeValue();
    }

    [[nodiscard]] std::unique_ptr<T, Deleter> value() const&
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return const_cast<Result*>(this)->takeValue();
    }

    [[nodiscard]] std::unique_ptr<T, Deleter> value() &&
    {
        MC_ASSERT_RELEASE_MSG(!failed(), error().toString().c_str());
        return takeValue();
    }

    [[nodiscard]] const E& error() const noexcept
    {
        return m_success ? m_successError : m_error;
    }

private:
    [[nodiscard]] std::unique_ptr<T, Deleter> takeValue()
    {
        T* value = m_value;
        m_value = nullptr;
        return std::unique_ptr<T, Deleter>(value, m_deleter);
    }

    E m_error{};
    T* m_value = nullptr;
    Deleter m_deleter{};
    bool m_success = false;
    static inline E m_successError{};
};

// ============================================================================
// 辅助宏
// ============================================================================

/**
 * @brief TRY宏 - 简化错误传播
 *
 * 使用示例:
 * @code
 * Result<i32> foo() { ... }
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
        var = std::move(_result).value(); \
    } while (0)

/**
 * @brief 声明变量并以表达式的值初始化；失败则向上传播错误
 *
 * 与 MC_TRY_ASSIGN 的区别：本宏**声明**变量，因此不需要先写一遍类型与初值。
 *
 * @warning 展开为两条语句（`auto` 声明 + `if`），只能写在语句序列里，
 *          不能放进 `if (...) MC_TRY_DECLARE(...);` 这类单语句体。
 */
#define MC_TRY_DECLARE(var, expr)             \
    auto _mc_try_result_##var = (expr);      \
    if (_mc_try_result_##var.failed()) {     \
        return _mc_try_result_##var.error(); \
    }                                        \
    auto var = std::move(_mc_try_result_##var).value()

} // namespace mc
