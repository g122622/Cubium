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

#include "../../core/Types.hpp"
#include <array>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace mc {

// 前向声明
class IChunk;
class ChunkPrimer;

// 前向声明 server 命名空间中的类
namespace server {
class ServerWorld;
}

// ============================================================================
// 区块类型
// ============================================================================

/**
 * @brief 区块类型枚举
 *
 * PROTOCHUNK: 原型区块（生成中的区块）
 * LEVELCHUNK: 完整区块（可加载的区块）
 */
enum class ChunkType : u8 { PROTOCHUNK, LEVELCHUNK };

// ============================================================================
// 高度图类型标志
// ============================================================================

/**
 * @brief 高度图类型标志
 *
 * 用于指定区块生成阶段需要更新的高度图类型
 */
enum class HeightmapFlag : u32 {
    NONE = 0,
    WORLD_SURFACE_WG = 1 << 0,          // 世界表面（生成时）
    OCEAN_FLOOR_WG = 1 << 1,            // 海底（生成时）
    WORLD_SURFACE = 1 << 2,             // 世界表面
    OCEAN_FLOOR = 1 << 3,               // 海底
    MOTION_BLOCKING = 1 << 4,           // 阻挡运动
    MOTION_BLOCKING_NO_LEAVES = 1 << 5, // 阻挡运动（不含树叶）
    LIGHT_BLOCKING = 1 << 6,            // 光照阻挡

    // 预定义组合
    PRE_FEATURES = WORLD_SURFACE_WG | OCEAN_FLOOR_WG,
    POST_FEATURES = WORLD_SURFACE | OCEAN_FLOOR | MOTION_BLOCKING | MOTION_BLOCKING_NO_LEAVES | LIGHT_BLOCKING
};

// 位运算操作符
inline constexpr HeightmapFlag operator|(HeightmapFlag a, HeightmapFlag b)
{
    return static_cast<HeightmapFlag>(static_cast<u32>(a) | static_cast<u32>(b));
}

inline constexpr HeightmapFlag operator&(HeightmapFlag a, HeightmapFlag b)
{
    return static_cast<HeightmapFlag>(static_cast<u32>(a) & static_cast<u32>(b));
}

inline constexpr HeightmapFlag& operator|=(HeightmapFlag& a, HeightmapFlag b)
{
    a = a | b;
    return a;
}

inline constexpr bool hasFlag(HeightmapFlag flags, HeightmapFlag flag)
{
    return (static_cast<u32>(flags) & static_cast<u32>(flag)) != 0;
}

// ============================================================================
// 区块生成阶段
// ============================================================================

/**
 * @brief 区块生成阶段
 *
 * 定义区块生成的各个阶段，每个阶段有特定的生成任务。
 *
 * 生成流程：
 * EMPTY → STRUCTURE_STARTS → STRUCTURE_REFERENCES → BIOMES → NOISE →
 * SURFACE → CARVERS → LIQUID_CARVERS → FEATURES → LIGHT → SPAWN →
 * HEIGHTMAPS → FULL
 *
 * 参考 MC 1.16.5 ChunkStatus，但简化了异步任务模型
 */
class ChunkStatus {
public:
    // === 任务类型定义 ===

    /**
     * @brief 选择性生成任务（可能需要邻居区块）
     * @param world 世界引用
     * @param generator 区块生成器
     * @param chunk 要处理的区块
     * @param neighbors 邻居区块数组（可能为空）
     */
    using SelectiveTask = std::function<void(server::ServerWorld& world,
        class IChunkGenerator& generator,
        ChunkPrimer& chunk,
        const std::array<IChunk*, 8>& neighbors)>;

    /**
     * @brief 简单生成任务（不需要邻居区块）
     * @param world 世界引用
     * @param generator 区块生成器
     * @param chunk 要处理的区块
     */
    using SimpleTask =
        std::function<void(server::ServerWorld& world, class IChunkGenerator& generator, ChunkPrimer& chunk)>;

    // === 构造函数 ===

    ChunkStatus() = default;

    /**
     * @brief 构造区块状态
     * @param name 状态名称
     * @param ordinal 序号
     * @param parent 父状态
     * @param taskRange 需要的邻居区块范围（0表示不需要邻居）
     * @param heightmaps 此阶段更新的高度图
     * @param type 区块类型
     */
    ChunkStatus(const std::string& name,
        i32 ordinal,
        const ChunkStatus* parent,
        i32 taskRange,
        HeightmapFlag heightmaps,
        ChunkType type);

    // === 属性访问 ===

    [[nodiscard]] const std::string& name() const { return m_name; }
    [[nodiscard]] i32 ordinal() const { return m_ordinal; }
    [[nodiscard]] const ChunkStatus* parent() const { return m_parent; }
    [[nodiscard]] i32 taskRange() const { return m_taskRange; }
    [[nodiscard]] HeightmapFlag heightmaps() const { return m_heightmaps; }
    [[nodiscard]] ChunkType type() const { return m_type; }

    // === 比较操作 ===

    /**
     * @brief 检查此状态是否达到或超过指定状态
     */
    [[nodiscard]] bool isAtLeast(const ChunkStatus& status) const { return m_ordinal >= status.m_ordinal; }

    /**
     * @brief 检查此状态是否低于指定状态
     */
    [[nodiscard]] bool isBefore(const ChunkStatus& status) const { return m_ordinal < status.m_ordinal; }

    // === 静态方法 ===

    /**
     * @brief 获取所有状态列表（按顺序）
     */
    [[nodiscard]] static const std::vector<ChunkStatus>& getAll();

    /**
     * @brief 根据名称获取状态
     */
    [[nodiscard]] static const ChunkStatus* byName(const std::string& name);

    /**
     * @brief 根据序号获取状态
     */
    [[nodiscard]] static const ChunkStatus* byOrdinal(i32 ordinal);

    /**
     * @brief 获取状态数量
     */
    [[nodiscard]] static i32 count() { return static_cast<i32>(getAll().size()); }

    /**
     * @brief 根据距离获取状态
     * @param distance 距离值
     */
    [[nodiscard]] static const ChunkStatus& getStatus(i32 distance);

    /**
     * @brief 获取状态的距离值
     */
    [[nodiscard]] static i32 getDistance(const ChunkStatus& status);

    /**
     * @brief 获取最大距离
     */
    [[nodiscard]] static i32 maxDistance();

    // === 比较运算符 ===

    bool operator==(const ChunkStatus& other) const { return m_ordinal == other.m_ordinal; }

    bool operator!=(const ChunkStatus& other) const { return m_ordinal != other.m_ordinal; }

    bool operator<(const ChunkStatus& other) const { return m_ordinal < other.m_ordinal; }

    bool operator<=(const ChunkStatus& other) const { return m_ordinal <= other.m_ordinal; }

    bool operator>(const ChunkStatus& other) const { return m_ordinal > other.m_ordinal; }

    bool operator>=(const ChunkStatus& other) const { return m_ordinal >= other.m_ordinal; }

private:
    std::string m_name;
    i32 m_ordinal = 0;
    const ChunkStatus* m_parent = nullptr;
    i32 m_taskRange = 0;
    HeightmapFlag m_heightmaps = HeightmapFlag::NONE;
    ChunkType m_type = ChunkType::PROTOCHUNK;
};

// ============================================================================
// 区块状态常量
// ============================================================================

namespace ChunkStatuses {

// 生成阶段序号常量（与 MC 1.16.5 一致）
constexpr i32 EMPTY_ORDINAL = 0;
constexpr i32 STRUCTURE_STARTS_ORDINAL = 1;
constexpr i32 STRUCTURE_REFERENCES_ORDINAL = 2;
constexpr i32 BIOMES_ORDINAL = 3;
constexpr i32 NOISE_ORDINAL = 4;
constexpr i32 SURFACE_ORDINAL = 5;
constexpr i32 CARVERS_ORDINAL = 6;
constexpr i32 LIQUID_CARVERS_ORDINAL = 7;
constexpr i32 FEATURES_ORDINAL = 8;
constexpr i32 LIGHT_ORDINAL = 9;
constexpr i32 SPAWN_ORDINAL = 10;
constexpr i32 HEIGHTMAPS_ORDINAL = 11;
constexpr i32 FULL_ORDINAL = 12;

// 总阶段数
constexpr i32 COUNT = 13;

} // namespace ChunkStatuses

// ============================================================================
// 全局静态阶段定义
// ============================================================================

namespace ChunkStatuses {

// 在 ChunkStatus.cpp 中定义
extern const ChunkStatus EMPTY;
extern const ChunkStatus STRUCTURE_STARTS;
extern const ChunkStatus STRUCTURE_REFERENCES;
extern const ChunkStatus BIOMES;
extern const ChunkStatus NOISE;
extern const ChunkStatus SURFACE;
extern const ChunkStatus CARVERS;
extern const ChunkStatus LIQUID_CARVERS;
extern const ChunkStatus FEATURES;
extern const ChunkStatus LIGHT;
extern const ChunkStatus SPAWN;
extern const ChunkStatus HEIGHTMAPS;
extern const ChunkStatus FULL;

} // namespace ChunkStatuses

} // namespace mc
