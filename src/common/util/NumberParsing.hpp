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
#include "common/util/assert/AssertAll.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <string>
#include <string_view>
#include <system_error>

namespace mc::util {

/**
 * @brief 不抛异常的文本数值解析
 *
 * `std::stoi` / `std::stol` / `std::stof` 系列在非法输入或越界时**抛出异常**，
 * 在 `-fno-exceptions` 下会直接终止进程；它们也不适合作为「外部输入校验」的基础设施。
 * 本组函数基于 `std::from_chars`：不做 locale 相关转换、不分配、不抛异常，
 * 非法或越界一律以 `Result` 的 `ErrorCode::InvalidData` 返回。
 *
 * 通用约定：
 * - 接受前后空白（与 JMH/MC 的文本读取习惯一致，避免 `" 42 "` 这类输入被误判为非法）
 * - 必须整串都被消费（除首尾空白外不允许残留字符），避免 `"42abc"` 被静默当成 42
 * - 十进制；`parseIntegerBase` 支持任意进制（用于 `#RRGGBB`、UUID 分段等）
 */

/**
 * @brief 去掉字符串两端的 ASCII 空白
 */
[[nodiscard]] std::string_view trimWhitespace(std::string_view text) noexcept;

/**
 * @brief 解析任意进制的无符号整数
 *
 * @param text 输入文本（可含首尾空白）
 * @param base 进制，2..36
 * @return 成功返回数值，非法/越界/进制非法返回错误
 */
[[nodiscard]] Result<u64> parseUint64Base(std::string_view text, i32 base);

/**
 * @brief 解析十进制无符号 64 位整数
 */
[[nodiscard]] Result<u64> parseUint64(std::string_view text);

/**
 * @brief 解析十进制无符号 32 位整数
 */
[[nodiscard]] Result<u32> parseUint32(std::string_view text);

/**
 * @brief 解析十进制有符号 64 位整数
 */
[[nodiscard]] Result<i64> parseInt64(std::string_view text);

/**
 * @brief 解析十进制有符号 32 位整数
 *
 * 越界（如 `"99999999999"`）返回错误，绝不静默截断。
 */
[[nodiscard]] Result<i32> parseInt32(std::string_view text);

/**
 * @brief 解析十进制有符号 16 位整数
 */
[[nodiscard]] Result<i16> parseInt16(std::string_view text);

/**
 * @brief 解析十进制有符号 8 位整数
 */
[[nodiscard]] Result<i8> parseInt8(std::string_view text);

/**
 * @brief 解析单精度浮点数
 *
 * 支持 `1.5`、`-2e10`、`inf`、`nan` 等 `from_chars` 认可的格式；整串必须被消费。
 */
[[nodiscard]] Result<f32> parseFloat32(std::string_view text);

/**
 * @brief 解析双精度浮点数
 */
[[nodiscard]] Result<f64> parseFloat64(std::string_view text);

/**
 * @brief 把双精度浮点数渲染成 Java `Double.toString` 风格的文本。
 *
 * 命令反馈里原版用 `String.valueOf(double)` 渲染参数（如 `3` 显示成 `3.0`、
 * `0.1` 显示成 `0.1`）。这里取最短往返表示，并在没有小数点/指数时补 `.0`；
 * 特殊值按 Java 的拼写输出 `NaN` / `Infinity` / `-Infinity`。
 *
 * 注：Java 在 1e-3 以下/1e7 以上改用科学计数法，本函数只在指数很大时才是科学计数法；
 * 命令反馈中的数值远小于该量级，差异可忽略（项目允许浮点格式上的细微差异）。
 */
[[nodiscard]] inline std::string javaDoubleToString(f64 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value < 0 ? "-Infinity" : "Infinity";
    }

    std::array<char, 64> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    MC_ASSERT_RELEASE_MSG(result.ec == std::errc{}, "to_chars failed for double value");
    std::string text(buffer.data(), result.ptr);
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }
    return text;
}

/**
 * @brief 把单精度浮点数渲染成 Java `Float.toString` 风格的文本。
 *
 * 语义与 {@link javaDoubleToString} 相同，只是以 float 精度生成最短往返表示。
 */
[[nodiscard]] inline std::string javaFloatToString(f32 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value < 0 ? "-Infinity" : "Infinity";
    }

    std::array<char, 64> buffer{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    MC_ASSERT_RELEASE_MSG(result.ec == std::errc{}, "to_chars failed for float value");
    std::string text(buffer.data(), result.ptr);
    if (text.find_first_of(".eE") == std::string::npos) {
        text += ".0";
    }
    return text;
}

} // namespace mc::util
