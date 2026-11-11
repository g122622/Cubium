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

#include <iosfwd>
#include <string_view>
#include <nlohmann/json.hpp>

namespace mc::util::json {

/**
 * @brief nlohmann-json 的非抛出访问层
 *
 * nlohmann 的 `json::parse` 在文本非法时抛 `parse_error`，`.get<T>()` / `.at()` 在类型或
 * 键不匹配时抛 `type_error` / `out_of_range`。这些异常一旦从数据包加载路径逃逸就会终止
 * 进程（`-fno-exceptions` 下 nlohmann 直接 abort），而数据包内容属于**外部不可信输入**。
 *
 * 因此数据包/网络等外部输入统一经本层读取：解析与取值失败都以 `Result` 返回，调用方
 * 记录 warn 并跳过该条目，绝不让一条坏数据打死整服。
 *
 * 约定：带 `field` 参数的重载要求该键存在且类型匹配；不带 `field` 的版本作用于一个已经
 * 定位好的 JSON 值。
 */

/// JSON 解析失败
[[nodiscard]] Result<nlohmann::json> parse(std::string_view text);

/**
 * @brief 从输入流解析 JSON（非抛出）
 *
 * 内部先读尽整个流再走 `parse`：nlohmann 的 `operator>>` 在文本非法时抛
 * `parse_error`，而文件/网络流的内容属于外部输入。
 */
[[nodiscard]] Result<nlohmann::json> parse(std::istream& input);

/**
 * @brief 取十进制整数（JSON number，必须为整数）
 */
[[nodiscard]] Result<i32> getInt32(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<i32> getInt32(const nlohmann::json& value);
[[nodiscard]] Result<i64> getInt64(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<i64> getInt64(const nlohmann::json& value);

/**
 * @brief 取浮点数（接受 JSON number，整数与浮点均可）
 */
[[nodiscard]] Result<f32> getFloat32(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<f32> getFloat32(const nlohmann::json& value);
[[nodiscard]] Result<f64> getFloat64(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<f64> getFloat64(const nlohmann::json& value);

/**
 * @brief 取布尔值（必须是 JSON bool）
 */
[[nodiscard]] Result<bool> getBool(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<bool> getBool(const nlohmann::json& value);

/**
 * @brief 取字符串（必须是 JSON string）
 */
[[nodiscard]] Result<std::string> getString(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<std::string> getString(const nlohmann::json& value);

/**
 * @brief 取对象 / 数组的常量引用（必须类型匹配）
 */
[[nodiscard]] Result<const nlohmann::json&> getObject(const nlohmann::json& value, std::string_view field);
[[nodiscard]] Result<const nlohmann::json&> getArray(const nlohmann::json& value, std::string_view field);

/**
 * @brief 检查字段是否存在（键存在即真，不校验类型）
 */
[[nodiscard]] bool hasField(const nlohmann::json& value, std::string_view field);

/**
 * @brief 取字段，不存在时返回 nullptr（不抛异常）
 *
 * 用于「可选字段」场景：调用方拿到指针后自行判空，而不是先 `contains` 再 `at`。
 */
[[nodiscard]] const nlohmann::json* findField(const nlohmann::json& value, std::string_view field);

} // namespace mc::util::json
