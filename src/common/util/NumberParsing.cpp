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

#include "NumberParsing.hpp"

#include <charconv>
#include <limits>
#include <string>

namespace mc::util {

namespace {

/// 解析失败时统一的错误构造，附带原始文本便于定位是哪个输入有问题
Error _invalidNumber(std::string_view text)
{
    return Error(ErrorCode::InvalidData, "invalid number literal: '" + std::string(text) + "'");
}

/// 解析越界时统一的错误构造
Error _numberOutOfRange(std::string_view text)
{
    return Error(ErrorCode::InvalidData, "number literal out of range: '" + std::string(text) + "'");
}

} // namespace

std::string_view trimWhitespace(std::string_view text) noexcept
{
    auto isSpace = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    };
    while (!text.empty() && isSpace(text.front())) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(text.back())) {
        text.remove_suffix(1);
    }
    return text;
}

namespace {

/**
 * @brief 去掉整数文本的前导 '+'（`std::from_chars` 对整数不接受 '+'）
 *
 * Java 的 `Integer.parseInt` / `Long.parseLong` 从 7 起接受前导 '+'，项目对齐 Java 语义，
 * 因此这里统一剥掉。只剥「后面还有字符」的那个 '+'，避免把单独的 "+" 变成空串后误判。
 */
std::string_view stripLeadingPlus(std::string_view text) noexcept
{
    if (text.size() > 1 && text.front() == '+') {
        text.remove_prefix(1);
    }
    return text;
}

} // namespace

Result<u64> parseUint64Base(std::string_view text, i32 base)
{
    if (base < 2 || base > 36) {
        return Error(ErrorCode::InvalidArgument, "parseUint64Base: base must be within [2, 36]");
    }

    const std::string_view trimmed = stripLeadingPlus(trimWhitespace(text));
    if (trimmed.empty()) {
        return _invalidNumber(text);
    }

    u64 value = 0;
    const auto [end, error] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value, base);
    if (error == std::errc::result_out_of_range) {
        return _numberOutOfRange(text);
    }
    if (error != std::errc() || end != trimmed.data() + trimmed.size()) {
        return _invalidNumber(text);
    }
    return value;
}

Result<u64> parseUint64(std::string_view text)
{
    return parseUint64Base(text, 10);
}

Result<u32> parseUint32(std::string_view text)
{
    auto result = parseUint64(text);
    if (result.failed()) {
        return result.error();
    }
    const u64 value = result.value();
    if (value > std::numeric_limits<u32>::max()) {
        return _numberOutOfRange(text);
    }
    return static_cast<u32>(value);
}

Result<i64> parseInt64(std::string_view text)
{
    const std::string_view trimmed = stripLeadingPlus(trimWhitespace(text));
    if (trimmed.empty()) {
        return _invalidNumber(text);
    }

    i64 value = 0;
    const auto [end, error] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value, 10);
    if (error == std::errc::result_out_of_range) {
        return _numberOutOfRange(text);
    }
    if (error != std::errc() || end != trimmed.data() + trimmed.size()) {
        return _invalidNumber(text);
    }
    return value;
}

Result<i32> parseInt32(std::string_view text)
{
    auto result = parseInt64(text);
    if (result.failed()) {
        return result.error();
    }
    const i64 value = result.value();
    if (value < std::numeric_limits<i32>::min() || value > std::numeric_limits<i32>::max()) {
        return _numberOutOfRange(text);
    }
    return static_cast<i32>(value);
}

Result<i16> parseInt16(std::string_view text)
{
    auto result = parseInt64(text);
    if (result.failed()) {
        return result.error();
    }
    const i64 value = result.value();
    if (value < std::numeric_limits<i16>::min() || value > std::numeric_limits<i16>::max()) {
        return _numberOutOfRange(text);
    }
    return static_cast<i16>(value);
}

Result<i8> parseInt8(std::string_view text)
{
    auto result = parseInt64(text);
    if (result.failed()) {
        return result.error();
    }
    const i64 value = result.value();
    if (value < std::numeric_limits<i8>::min() || value > std::numeric_limits<i8>::max()) {
        return _numberOutOfRange(text);
    }
    return static_cast<i8>(value);
}

Result<f32> parseFloat32(std::string_view text)
{
    const std::string_view trimmed = trimWhitespace(text);
    if (trimmed.empty()) {
        return _invalidNumber(text);
    }

    f32 value = 0.0f;
    const auto [end, error] =
        std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value, std::chars_format::general);
    if (error == std::errc::result_out_of_range) {
        return _numberOutOfRange(text);
    }
    if (error != std::errc() || end != trimmed.data() + trimmed.size()) {
        return _invalidNumber(text);
    }
    return value;
}

Result<f64> parseFloat64(std::string_view text)
{
    const std::string_view trimmed = trimWhitespace(text);
    if (trimmed.empty()) {
        return _invalidNumber(text);
    }

    f64 value = 0.0;
    const auto [end, error] =
        std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value, std::chars_format::general);
    if (error == std::errc::result_out_of_range) {
        return _numberOutOfRange(text);
    }
    if (error != std::errc() || end != trimmed.data() + trimmed.size()) {
        return _invalidNumber(text);
    }
    return value;
}

} // namespace mc::util
