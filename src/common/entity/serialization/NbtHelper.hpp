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
#include "common/util/nbt/Nbt.hpp"
#include <optional>
#include <string>
#include <vector>

namespace mc {
namespace entity {
namespace attribute {
class AttributeMap;
}
} // namespace entity
} // namespace mc

namespace mc::entity::serialization {

/**
 * @brief NBT 辅助工具函数
 *
 * 提供实体序列化/反序列化所需的通用 NBT 读写操作。
 * 从 PlayerSaveData.cpp 提取并扩展为公共工具。
 */
namespace nbt_helper {

// ========== 安全读取函数 ==========

/** @brief 安全读取 i8 值，键不存在返回 std::nullopt */
[[nodiscard]] std::optional<i8> tryGetByte(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 i16 值 */
[[nodiscard]] std::optional<i16> tryGetShort(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 i32 值 */
[[nodiscard]] std::optional<i32> tryGetInt(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 i64 值 */
[[nodiscard]] std::optional<i64> tryGetLong(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 f32 值 */
[[nodiscard]] std::optional<f32> tryGetFloat(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 f64 值 */
[[nodiscard]] std::optional<f64> tryGetDouble(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取字符串值 */
[[nodiscard]] std::optional<std::string> tryGetString(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 bool 值（底层为 byte） */
[[nodiscard]] std::optional<bool> tryGetBool(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 compound_tag 指针，不存在返回 nullptr */
[[nodiscard]] const nbt::tags::compound_tag* tryGetCompound(const nbt::tags::compound_tag& tag, const std::string& key);

/** @brief 安全读取 list_tag 指针，不存在返回 nullptr */
[[nodiscard]] const nbt::tags::list_tag* tryGetList(const nbt::tags::compound_tag& tag, const std::string& key);

// ========== MC 格式列表读写 ==========

/**
 * @brief 写入 double 列表（MC 格式：Pos、Motion 等）
 *
 * @param tag 目标 compound_tag
 * @param key 键名
 * @param values double 值数组
 */
void putDoubleList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<f64>& values);

/**
 * @brief 写入 float 列表（MC 格式：Rotation 等）
 *
 * @param tag 目标 compound_tag
 * @param key 键名
 * @param values float 值数组
 */
void putFloatList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<f32>& values);

/**
 * @brief 读取 double 列表
 *
 * @param tag 源 compound_tag
 * @param key 键名
 * @return double 列表，键不存在或类型不匹配返回空
 */
[[nodiscard]] std::vector<f64> getDoubleList(const nbt::tags::compound_tag& tag, const std::string& key);

/**
 * @brief 读取 float 列表
 *
 * @param tag 源 compound_tag
 * @param key 键名
 * @return float 列表，键不存在或类型不匹配返回空
 */
[[nodiscard]] std::vector<f32> getFloatList(const nbt::tags::compound_tag& tag, const std::string& key);

/**
 * @brief 写入 int 列表（MC 格式：LastDeathLocation.pos 等）
 *
 * @param tag 目标 compound_tag
 * @param key 键名
 * @param values i32 值数组
 */
void putIntList(nbt::tags::compound_tag& tag, const std::string& key, const std::vector<i32>& values);

/**
 * @brief 读取 int 列表
 *
 * @param tag 源 compound_tag
 * @param key 键名
 * @return i32 列表，键不存在或类型不匹配返回空
 */
[[nodiscard]] std::vector<i32> getIntList(const nbt::tags::compound_tag& tag, const std::string& key);

// ========== UUID 读写 ==========

/**
 * @brief 写入 UUID（MC 格式：UUIDMost + UUIDLeast 两个 i64）
 *
 * @param tag 目标 compound_tag
 * @param uuid UUID 字符串（32位十六进制）
 */
void putUuid(nbt::tags::compound_tag& tag, const std::string& uuid);

/**
 * @brief 读取 UUID
 *
 * @param tag 源 compound_tag
 * @return UUID 字符串，不存在返回空字符串
 */
[[nodiscard]] std::string getUuid(const nbt::tags::compound_tag& tag);

// ========== AttributeMap 序列化 ==========

/**
 * @brief 将 AttributeMap 序列化为 NBT compound_tag 列表
 *
 * MC 1.16.5 格式:
 * [
 *   {
 *     "Name": "generic.max_health",
 *     "Base": 20.0,
 *     "Modifiers": [
 *       { "UUIDMost": ..., "UUIDLeast": ..., "Name": "...", "Operation": 0, "Amount": 1.0 }
 *     ]
 *   }
 * ]
 *
 * @param tag 目标 compound_tag
 * @param key 键名（通常为 "Attributes"）
 * @param attrMap 属性映射表
 */
void writeAttributeMap(
    nbt::tags::compound_tag& tag, const std::string& key, const entity::attribute::AttributeMap& attrMap);

/**
 * @brief 从 NBT compound_tag 列表反序列化 AttributeMap
 *
 * @param tag 源 compound_tag
 * @param key 键名（通常为 "Attributes"）
 * @param attrMap 目标属性映射表
 */
void readAttributeMap(
    const nbt::tags::compound_tag& tag, const std::string& key, entity::attribute::AttributeMap& attrMap);

} // namespace nbt_helper

} // namespace mc::entity::serialization
