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
#include "common/world/chunk/base/ChunkPos.hpp"
#include "common/world/chunk/load/ChunkLoadLevel.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 显式 Ticket 类型 - 定义非玩家来源的区块加载请求
// ============================================================================

/**
 * @brief 空类型，用于不需要值的显式 ticket 类型
 *
 * 某些显式 ticket 类型（如 START, DRAGON）不需要关联具体的值，
 * 使用 Unit 作为模板参数。
 */
struct Unit {
    // 为 Unit 类型提供比较运算符
    bool operator<(const Unit&) const noexcept { return false; }
    bool operator==(const Unit&) const noexcept { return true; }
};

/**
 * @brief 显式区块加载 ticket 类型
 *
 * 定义会直接存入 `ChunkTicketSet` 的显式加载请求类型。
 * 用于强制加载、传送门、光照等非玩家来源的区块加载请求。
 *
 * @note 玩家加载来源不再表示为 ticket，而是由 `ChunkLoadTicketManager`
 *       作为独立 player source 聚合后注入 `ChunkDistanceGraph`。
 *
 * @tparam T 显式 ticket 关联的值类型（如 ChunkPos、u32、Unit）
 *
 * @example
 * @code
 * // 创建永久票据类型
 * auto forcedType = ChunkLoadTicketType<ChunkPos>::create("forced");
 *
 * // 创建带生命周期的票据类型
 * auto portalType = ChunkLoadTicketType<ChunkPos>::create("portal", 300); // 300 tick
 * @endcode
 */
template <typename T>
class ChunkLoadTicketType {
public:
    using ValueType = T;
    using Comparator = std::function<bool(const T&, const T&)>;

    /**
     * @brief 获取显式 ticket 类型名称
     * @return 类型名称字符串
     */
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }

    /**
     * @brief 获取生命周期（tick 数）
     * @return 生命周期，0 表示永不过期
     */
    [[nodiscard]] u32 lifespan() const noexcept { return m_lifespan; }

    /**
     * @brief 获取比较器
     * @return 用于比较同类型票据的比较器
     */
    [[nodiscard]] const Comparator& comparator() const noexcept { return m_comparator; }

    /**
     * @brief 创建不带生命周期的显式 ticket 类型
     * @param name 类型名称（必须唯一）
     * @param comp 比较器函数
     * @return 票据类型实例
     *
     * @note 不带生命周期的显式 ticket 会一直存在直到被显式移除
     */
    static ChunkLoadTicketType<T> create(const std::string& name, Comparator comp = defaultCompare)
    {
        return ChunkLoadTicketType<T>(name, comp, 0);
    }

    /**
     * @brief 创建带生命周期的显式 ticket 类型
     * @param name 类型名称（必须唯一）
     * @param lifespan 生命周期（tick 数），过期后自动移除
     * @param comp 比较器函数
     * @return 票据类型实例
     *
     * @note 生命周期 ticket 常用于临时加载（如传送门）
     */
    static ChunkLoadTicketType<T> create(const std::string& name, u32 lifespan, Comparator comp = defaultCompare)
    {
        return ChunkLoadTicketType<T>(name, comp, lifespan);
    }

    bool operator==(const ChunkLoadTicketType& other) const noexcept { return m_name == other.m_name; }

    bool operator!=(const ChunkLoadTicketType& other) const noexcept { return m_name != other.m_name; }

private:
    std::string m_name;
    Comparator m_comparator;
    u32 m_lifespan;

    ChunkLoadTicketType(const std::string& name, Comparator comp, u32 lifespan)
        : m_name(name)
        , m_comparator(std::move(comp))
        , m_lifespan(lifespan)
    {}

    static bool defaultCompare(const T& a, const T& b) { return a < b; }
};

// ============================================================================
// 预定义显式 ticket 类型
// ============================================================================

namespace TicketTypes {
/**
 * @brief 强制加载票据
 *
 * 通过 /forceload 命令或 API 添加。
 * 永久加载区块，不会因玩家离开而卸载。
 */
extern const ChunkLoadTicketType<ChunkPos> FORCED;

/**
 * @brief 传送门加载票据
 *
 * 玩家使用传送门时临时加载目标区域的区块。
 * 生命周期：300 tick（约 15 秒）
 */
extern const ChunkLoadTicketType<ChunkPos> PORTAL;

/**
 * @brief 传送后加载票据
 *
 * 传送完成后的临时加载，确保区块不会立即卸载。
 * 生命周期：5 tick
 */
extern const ChunkLoadTicketType<u32> POST_TELEPORT;

/**
 * @brief 未知/临时加载票据
 */
extern const ChunkLoadTicketType<ChunkPos> UNKNOWN;

/**
 * @brief 世界启动票据
 *
 * 用于加载世界出生点附近的区块。
 */
extern const ChunkLoadTicketType<Unit> START;

/**
 * @brief 末影龙战斗票据
 *
 * 加载末地中央岛屿的区块。
 */
extern const ChunkLoadTicketType<Unit> DRAGON;

/**
 * @brief 光照计算票据
 *
 * 用于光照计算的区块加载。
 */
extern const ChunkLoadTicketType<ChunkPos> LIGHT;

} // namespace TicketTypes

// ============================================================================
// 区块加载票据
// ============================================================================

/**
 * @brief 显式区块加载 ticket
 *
 * 代表一个会直接进入 `ChunkTicketSet` 的显式加载请求，
 * 包含类型、级别和关联值。参考 Minecraft 的 Ticket 类。
 *
 * 玩家 source 不使用此类型建模。
 *
 * 票据级别说明：
 * - Level 越小，优先级越高
 * - Level <= 31：完全加载（实体可以 tick）
 * - Level == 32：方块 tick 区块
 * - Level == 33：完全加载区块（Full）
 * - Level == 34：边界区块（加载但无 tick）
 * - Level 35-45：生成中间状态
 * - Level >= 46：未加载
 *
 * @note 显式 ticket 是不可变的，创建后无法修改
 */
class ChunkLoadTicket {
public:
    ChunkLoadTicket() = default;

    /**
     * @brief 构造显式 ticket
     * @tparam T 值类型
     * @param type 显式 ticket 类型
     * @param level ticket 级别
     * @param value 关联值
     */
    template <typename T>
    ChunkLoadTicket(const ChunkLoadTicketType<T>& type, i32 level, const T& value)
        : m_typeName(type.name())
        , m_level(level)
        , m_timestamp(0)
        , m_lifespan(type.lifespan())
    {
        if constexpr (std::is_same_v<T, ChunkPos>) {
            m_chunkValue = value;
            m_hasChunkValue = true;
        } else if constexpr (std::is_same_v<T, u32>) {
            m_intValue = value;
            m_hasIntValue = true;
        }
    }

    /**
     * @brief 比较优先级
     * @param other 另一个票据
     * @return true 表示 this 优先级低于 other（在优先队列中排后面）
     *
     * @note 级别越小优先级越高
     */
    bool operator<(const ChunkLoadTicket& other) const noexcept
    {
        if (m_level != other.m_level) {
            return m_level > other.m_level; // 级别大的排后面
        }
        return m_typeName > other.m_typeName;
    }

    bool operator==(const ChunkLoadTicket& other) const noexcept
    {
        return m_typeName == other.m_typeName && m_level == other.m_level && m_chunkValue == other.m_chunkValue &&
            m_intValue == other.m_intValue;
    }

    bool operator!=(const ChunkLoadTicket& other) const noexcept { return !(*this == other); }

    /** @brief 获取 ticket 级别 */
    [[nodiscard]] i32 level() const noexcept { return m_level; }

    /** @brief 获取 ticket 类型名称 */
    [[nodiscard]] const std::string& typeName() const noexcept { return m_typeName; }

    /** @brief 获取区块值 */
    [[nodiscard]] ChunkPos chunkValue() const noexcept { return m_chunkValue; }
    [[nodiscard]] bool hasChunkValue() const noexcept { return m_hasChunkValue; }

    /** @brief 获取整数值 */
    [[nodiscard]] u32 intValue() const noexcept { return m_intValue; }
    [[nodiscard]] bool hasIntValue() const noexcept { return m_hasIntValue; }

    /** @brief 设置时间戳（用于过期检查） */
    void setTimestamp(u64 timestamp) noexcept { m_timestamp = timestamp; }

    /**
     * @brief 检查是否过期
     * @param currentTime 当前时间
     * @return true 表示已过期
     *
     * @note 生命周期为 0 的票据永不过期
     */
    [[nodiscard]] bool isExpired(u64 currentTime) const noexcept
    {
        if (m_lifespan == 0) return false;
        return currentTime - m_timestamp > m_lifespan;
    }

private:
    std::string m_typeName;
    i32 m_level = static_cast<i32>(ChunkLoadLevel::MaxLevel); // 默认为未加载级别
    u64 m_timestamp = 0;
    u32 m_lifespan = 0;

    ChunkPos m_chunkValue{0, 0};
    u32 m_intValue = 0;
    bool m_hasChunkValue = false;
    bool m_hasIntValue = false;
};

// ============================================================================
// 显式 ticket 集合 - 每个区块可以有多个显式 ticket
// ============================================================================

/**
 * @brief 区块显式 ticket 集合
 *
 * 管理单个区块的所有显式 ticket，自动计算最小级别。
 *
 * @note 一个区块可以有多个显式 ticket，最终 ticket 级别由最小 level 决定
 */
class ChunkTicketSet {
public:
    /**
     * @brief 添加票据
     * @param ticket 要添加的票据
     *
     * @note 如果相同票据已存在，不会重复添加
     */
    void addTicket(ChunkLoadTicket ticket);

    /**
     * @brief 移除票据
     * @param ticket 要移除的票据
     * @return true 如果成功移除
     */
    bool removeTicket(const ChunkLoadTicket& ticket);

    /**
     * @brief 获取最小级别（最高优先级）
     * @return 最小级别，如果集合为空返回 MaxLevel
     */
    [[nodiscard]] i32 getMinLevel() const noexcept;

    /** @brief 是否为空 */
    [[nodiscard]] bool empty() const { return m_tickets.empty(); }

    /**
     * @brief 清理过期票据
     * @param currentTime 当前时间
     */
    void removeExpired(u64 currentTime);

    /** @brief 票据数量 */
    [[nodiscard]] size_t size() const { return m_tickets.size(); }

    /** @brief 获取所有票据 */
    [[nodiscard]] const std::vector<ChunkLoadTicket>& tickets() const { return m_tickets; }

private:
    std::vector<ChunkLoadTicket> m_tickets;
};

} // namespace mc::world::chunk
