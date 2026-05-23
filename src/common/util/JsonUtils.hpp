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

#include "core/Result.hpp"

#include <nlohmann/json.hpp>
#include <sstream>
#include <string_view>

namespace mc::json {

/**
 * @brief 安全解析 JSON 字符串（不抛出异常）
 *
 * 使用 nlohmann::json::parse 的 allow_exceptions=false 模式，
 * 解析失败时返回 Error 而不是抛出异常。
 *
 * @param str JSON 字符串
 * @return Result<nlohmann::json> 解析结果，失败时包含 JsonParseError
 *
 * @example
 * @code
 * auto result = mc::json::parse(jsonString);
 * if (result.failed()) {
 *     spdlog::error("JSON parse failed: {}", result.error().message());
 *     return result.error();
 * }
 * const auto& json = result.value();
 * @endcode
 */
[[nodiscard]] inline Result<nlohmann::json> parse(std::string_view str) noexcept
{
    auto j = nlohmann::json::parse(str, nullptr, false);
    if (j.is_discarded()) {
        return Error(ErrorCode::JsonParseError, "Failed to parse JSON string");
    }
    return j;
}

/**
 * @brief 安全解析 JSON 输入流（不抛出异常）
 *
 * @param stream JSON 输入流
 * @return Result<nlohmann::json> 解析结果，失败时包含 JsonParseError
 */
[[nodiscard]] inline Result<nlohmann::json> parse(std::istream& stream) noexcept
{
    try {
        nlohmann::json j;
        stream >> j;
        if (stream.fail() && !stream.eof()) {
            return Error(ErrorCode::JsonParseError, "Failed to parse JSON stream");
        }
        return j;
    }
    catch (...) {
        return Error(ErrorCode::JsonParseError, "Failed to parse JSON stream");
    }
}

/**
 * @brief 安全解析 JSON 字符串并返回详细信息（不抛出异常）
 *
 * @param str JSON 字符串
 * @param context 上下文信息（用于错误消息）
 * @return Result<nlohmann::json> 解析结果，失败时包含带有上下文的错误信息
 */
[[nodiscard]] inline Result<nlohmann::json> parseWithContext(std::string_view str, std::string_view context) noexcept
{
    auto j = nlohmann::json::parse(str, nullptr, false);
    if (j.is_discarded()) {
        return Error(ErrorCode::JsonParseError,
            std::string("Failed to parse JSON") + (context.empty() ? "" : " for " + std::string(context)));
    }
    return j;
}

/**
 * @brief 验证 JSON 字符串是否有效
 *
 * @param str JSON 字符串
 * @return bool 是否为有效的 JSON
 */
[[nodiscard]] inline bool isValid(std::string_view str) noexcept
{
    auto j = nlohmann::json::parse(str, nullptr, false);
    return !j.is_discarded();
}

/**
 * @brief 安全获取 JSON 对象中的字段（不抛出异常）
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<nlohmann::json> 字段值，不存在时返回 NotFound 错误
 */
[[nodiscard]] inline Result<nlohmann::json> getField(const nlohmann::json& json, std::string_view key) noexcept
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "JSON value is not an object");
    }
    auto it = json.find(key);
    if (it == json.end()) {
        return Error(ErrorCode::NotFound, std::string("Field not found: ") + std::string(key));
    }
    return *it;
}

/**
 * @brief 安全获取 JSON 对象中的可选字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return std::optional<nlohmann::json> 字段值，不存在时返回 nullopt
 */
[[nodiscard]] inline std::optional<nlohmann::json> getOptionalField(const nlohmann::json& json, std::string_view key) noexcept
{
    if (!json.is_object()) {
        return std::nullopt;
    }
    auto it = json.find(key);
    if (it == json.end()) {
        return std::nullopt;
    }
    return *it;
}

/**
 * @brief 安全获取 JSON 对象中的字符串字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<std::string> 字符串值，失败时返回错误
 */
[[nodiscard]] inline Result<std::string> getString(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_string()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not a string: ") + std::string(key));
    }
    return field.get<std::string>();
}

/**
 * @brief 安全获取 JSON 对象中的整数字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<i64> 整数值，失败时返回错误
 */
[[nodiscard]] inline Result<i64> getInt(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_number_integer()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not an integer: ") + std::string(key));
    }
    return field.get<i64>();
}

/**
 * @brief 安全获取 JSON 对象中的浮点数字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<f64> 浮点数值，失败时返回错误
 */
[[nodiscard]] inline Result<f64> getFloat(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_number()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not a number: ") + std::string(key));
    }
    return field.get<f64>();
}

/**
 * @brief 安全获取 JSON 对象中的布尔字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<bool> 布尔值，失败时返回错误
 */
[[nodiscard]] inline Result<bool> getBool(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_boolean()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not a boolean: ") + std::string(key));
    }
    return field.get<bool>();
}

/**
 * @brief 安全获取 JSON 对象中的数组字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<nlohmann::json> 数组值，失败时返回错误
 */
[[nodiscard]] inline Result<nlohmann::json> getArray(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_array()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not an array: ") + std::string(key));
    }
    return field;
}

/**
 * @brief 安全获取 JSON 对象中的对象字段
 *
 * @param json JSON 对象
 * @param key 字段名
 * @return Result<nlohmann::json> 对象值，失败时返回错误
 */
[[nodiscard]] inline Result<nlohmann::json> getObject(const nlohmann::json& json, std::string_view key) noexcept
{
    auto fieldResult = getField(json, key);
    if (fieldResult.failed()) {
        return fieldResult.error();
    }
    const auto& field = fieldResult.value();
    if (!field.is_object()) {
        return Error(ErrorCode::InvalidData, std::string("Field is not an object: ") + std::string(key));
    }
    return field;
}

} // namespace mc::json
