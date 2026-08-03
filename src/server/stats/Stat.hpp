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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/stats/StatType.hpp"
#include <cstddef>
#include <functional>
#include <string>

namespace mc {
namespace server {
namespace stats {

/**
 * @brief 统计项
 *
 * 表示一个具体的统计条目，包含统计ID和当前值。
 * 统计值可以是整数（如击杀次数）或距离/时间等。
 */
class Stat {
public:
    /**
     * @brief 统计值类型
     *
     * 大多数统计使用整数计数，但某些自定义统计
     * 可能代表距离、时间等
     */
    using ValueType = i64;

    /**
     * @brief 构造统计项
     *
     * @param type 统计类型
     * @param id 统计ID（方块、物品、实体或自定义统计ID）
     */
    Stat(StatType type, ResourceLocation id) noexcept;

    /**
     * @brief 获取统计类型
     */
    [[nodiscard]] StatType getType() const noexcept { return m_type; }

    /**
     * @brief 获取统计ID
     */
    [[nodiscard]] const ResourceLocation& getId() const noexcept { return m_id; }

    /**
     * @brief 获取完整的统计资源位置
     *
     * 格式：minecraft.{type}:{id}
     */
    [[nodiscard]] ResourceLocation getFullLocation() const;

    /**
     * @brief 获取当前统计值
     */
    [[nodiscard]] ValueType getValue() const noexcept { return m_value; }

    /**
     * @brief 设置统计值
     *
     * @param value 新值
     */
    void setValue(ValueType value) noexcept;

    /**
     * @brief 增加统计值
     *
     * @param delta 增量（默认为1）
     */
    void increment(ValueType delta = 1) noexcept;

    /**
     * @brief 重置统计值
     */
    void reset() noexcept { m_value = 0; }

    /**
     * @brief 判断两个统计项是否相等
     *
     * 基于类型和ID比较，不比较值
     */
    [[nodiscard]] bool operator==(const Stat& other) const noexcept
    {
        return m_type == other.m_type && m_id == other.m_id;
    }

    /**
     * @brief 判断两个统计项是否不等
     */
    [[nodiscard]] bool operator!=(const Stat& other) const noexcept { return !(*this == other); }

    /**
     * @brief 获取哈希值（用于 unordered_map/unordered_set）
     */
    [[nodiscard]] size_t hash() const noexcept
    {
        size_t h1 = std::hash<u8>{}(static_cast<u8>(m_type));
        size_t h2 = m_id.hash();
        return h1 ^ (h2 << 1);
    }

private:
    StatType m_type;
    ResourceLocation m_id;
    ValueType m_value = 0;
};

} // namespace stats
} // namespace server
} // namespace mc

namespace std {

/**
 * @brief Stat 的 std::hash 特化
 */
template <>
struct hash<mc::server::stats::Stat> {
    size_t operator()(const mc::server::stats::Stat& stat) const noexcept { return stat.hash(); }
};

} // namespace std
