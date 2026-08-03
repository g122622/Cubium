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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF ANY KIND, WHETHER
 * EXPRESS OR IMPLIED, INCLUDING STATUTORY OR OTHERWISE, IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::nbt::tags {
struct tag;
struct compound_tag;
struct list_tag;
} // namespace mc::nbt::tags

namespace mc::nbt {

/**
 * @brief 将 NBT 标签转换为 nlohmann::json
 *
 * 转换规则：
 * - 数值类型（Byte/Short/Int/Long/Float/Double）直接转换为 JSON 数值
 * - 字符串直接转换为 JSON 字符串
 * - ByteArray/IntArray/LongArray 转换为 JSON 数组
 * - List 转换为 JSON 数组
 * - Compound 转换为 JSON 对象
 * - End 转换为 null
 *
 * @param tag NBT 标签
 * @return 转换后的 JSON 值
 */
[[nodiscard]] nlohmann::json nbtToJson(const tags::tag& tag);

/**
 * @brief 将 JSON 值转换为 NBT compound_tag
 *
 * 转换规则：
 * - JSON 对象 -> compound_tag
 * - JSON 数组 -> 根据元素类型推断 list_tag 子类型
 * - JSON 字符串 -> 包装为 compound_tag（键为空字符串）或直接返回 string_tag
 * - JSON 数值 -> 根据值大小推断最合适的整数类型
 * - JSON null -> 空 compound_tag
 *
 * @param json JSON 值
 * @return 转换后的 compound_tag，如果转换失败返回 nullptr
 */
[[nodiscard]] std::unique_ptr<tags::compound_tag> jsonToNbt(const nlohmann::json& json);

/**
 * @brief 将 Mojangson 格式字符串解析为 compound_tag
 *
 * 支持标准 Mojangson 语法，如 {key:value, key2:42b}
 *
 * @param mojangsonString Mojangson 格式字符串
 * @return 解析后的 compound_tag，如果解析失败返回 nullptr
 */
[[nodiscard]] std::unique_ptr<tags::compound_tag> parseMojangson(const std::string& mojangsonString);

} // namespace mc::nbt
