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

#include "ChunkStatus.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

namespace ChunkStatuses {

// 阶段定义（按顺序，与 MC 1.16.5 一致）
//
// taskRange 参数说明：
// - 0: 不需要邻居区块
// - 8: 需要周围8个区块（FEATURES阶段）
// - 1: 只需要相邻区块（LIGHT阶段）
//
// heightmaps 参数说明：
// - PRE_FEATURES: WORLD_SURFACE_WG | OCEAN_FLOOR_WG（生成前高度图）
// - POST_FEATURES: WORLD_SURFACE | OCEAN_FLOOR | MOTION_BLOCKING | MOTION_BLOCKING_NO_LEAVES（生成后高度图）

// EMPTY: 空区块，刚创建
const ChunkStatus EMPTY("empty",
    EMPTY_ORDINAL,
    nullptr, // 父状态为 nullptr，表示起始状态
    -1,      // taskRange = -1 表示不执行任务
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

// STRUCTURE_STARTS: 结构起点生成
// 生成结构（村庄、神殿等）的起点位置
const ChunkStatus STRUCTURE_STARTS(
    "structure_starts", STRUCTURE_STARTS_ORDINAL, &EMPTY, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// STRUCTURE_REFERENCES: 结构引用计算
// 计算结构之间的引用关系
const ChunkStatus STRUCTURE_REFERENCES("structure_references",
    STRUCTURE_REFERENCES_ORDINAL,
    &STRUCTURE_STARTS,
    8, // 需要邻居区块来确定结构引用
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

// BIOMES: 生物群系生成
// 为区块生成生物群系数据
const ChunkStatus BIOMES(
    "biomes", BIOMES_ORDINAL, &STRUCTURE_REFERENCES, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// NOISE: 噪声地形生成
// 使用噪声生成基础地形
const ChunkStatus NOISE("noise",
    NOISE_ORDINAL,
    &BIOMES,
    8, // 需要邻居区块以获取平滑的生物群系过渡
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

// SURFACE: 地表生成
// 生成地表方块（草地、沙子等）
const ChunkStatus SURFACE("surface", SURFACE_ORDINAL, &NOISE, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// CARVERS: 空气雕刻
// 生成洞穴和峡谷（空气填充）
const ChunkStatus CARVERS("carvers", CARVERS_ORDINAL, &SURFACE, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// LIQUID_CARVERS: 液体雕刻
// 生成水下洞穴（水/熔岩填充）
const ChunkStatus LIQUID_CARVERS("liquid_carvers",
    LIQUID_CARVERS_ORDINAL,
    &CARVERS,
    0,
    HeightmapFlag::POST_FEATURES, // 切换到生成后高度图
    ChunkType::PROTOCHUNK);

// FEATURES: 特性放置
// 放置树木、矿石、花草等地物
const ChunkStatus FEATURES("features",
    FEATURES_ORDINAL,
    &LIQUID_CARVERS,
    8, // 需要邻居区块以正确连接特性
    HeightmapFlag::POST_FEATURES,
    ChunkType::PROTOCHUNK);

// LIGHT: 光照计算
// 计算区块光照
const ChunkStatus LIGHT("light",
    LIGHT_ORDINAL,
    &FEATURES,
    1, // 需要相邻区块的光照数据
    HeightmapFlag::POST_FEATURES,
    ChunkType::PROTOCHUNK);

// SPAWN: 生物生成点计算
// 计算生物的合法生成点
const ChunkStatus SPAWN("spawn", SPAWN_ORDINAL, &LIGHT, 0, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// HEIGHTMAPS: 高度图更新
// 更新最终高度图
const ChunkStatus HEIGHTMAPS(
    "heightmaps", HEIGHTMAPS_ORDINAL, &SPAWN, 0, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// FULL: 完整区块
// 区块完全生成完成，可以加载到世界
const ChunkStatus FULL("full",
    FULL_ORDINAL,
    &HEIGHTMAPS,
    0,
    HeightmapFlag::POST_FEATURES,
    ChunkType::LEVELCHUNK // 标记为可加载区块
);

} // namespace ChunkStatuses

// ============================================================================
// 状态范围映射
// ============================================================================

namespace {

// 根据 MC 的 STATUS_BY_RANGE 映射
// 用于将距离值映射到对应的状态
const std::vector<const ChunkStatus*> STATUS_BY_RANGE = {&ChunkStatuses::FULL,
    &ChunkStatuses::FEATURES,
    &ChunkStatuses::LIQUID_CARVERS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS,
    &ChunkStatuses::STRUCTURE_STARTS};

// 预计算的状态到距离映射
std::vector<i32> computeRangeByStatus()
{
    std::vector<i32> rangeByStatus(ChunkStatuses::COUNT);
    i32 rangeIndex = 0;

    for (i32 j = ChunkStatuses::COUNT - 1; j >= 0; --j) {
        while (rangeIndex + 1 < static_cast<i32>(STATUS_BY_RANGE.size()) &&
            j <= STATUS_BY_RANGE[rangeIndex + 1]->ordinal()) {
            ++rangeIndex;
        }
        rangeByStatus[static_cast<size_t>(j)] = rangeIndex;
    }

    return rangeByStatus;
}

} // anonymous namespace

// ============================================================================
// 构造函数
// ============================================================================

ChunkStatus::ChunkStatus(const std::string& name,
    i32 ordinal,
    const ChunkStatus* parent,
    i32 taskRange,
    HeightmapFlag heightmaps,
    ChunkType type)
    : m_name(name)
    , m_ordinal(ordinal)
    , m_parent(parent ? parent : this)
    , m_taskRange(taskRange)
    , m_heightmaps(heightmaps)
    , m_type(type)
{}

// ============================================================================
// 静态方法
// ============================================================================

const std::vector<ChunkStatus>& ChunkStatus::getAll()
{
    static const std::vector<ChunkStatus> allStatuses = {ChunkStatuses::EMPTY,
        ChunkStatuses::STRUCTURE_STARTS,
        ChunkStatuses::STRUCTURE_REFERENCES,
        ChunkStatuses::BIOMES,
        ChunkStatuses::NOISE,
        ChunkStatuses::SURFACE,
        ChunkStatuses::CARVERS,
        ChunkStatuses::LIQUID_CARVERS,
        ChunkStatuses::FEATURES,
        ChunkStatuses::LIGHT,
        ChunkStatuses::SPAWN,
        ChunkStatuses::HEIGHTMAPS,
        ChunkStatuses::FULL};
    return allStatuses;
}

const ChunkStatus* ChunkStatus::byName(const std::string& name)
{
    const auto& all = getAll();
    for (const auto& status : all) {
        if (status.name() == name) {
            return &status;
        }
    }
    return nullptr;
}

const ChunkStatus* ChunkStatus::byOrdinal(i32 ordinal)
{
    const auto& all = getAll();
    if (ordinal >= 0 && ordinal < static_cast<i32>(all.size())) {
        return &all[static_cast<size_t>(ordinal)];
    }
    return nullptr;
}

const ChunkStatus& ChunkStatus::getStatus(i32 distance)
{
    if (distance >= static_cast<i32>(STATUS_BY_RANGE.size())) {
        return ChunkStatuses::EMPTY;
    }
    if (distance < 0) {
        return ChunkStatuses::FULL;
    }
    return *STATUS_BY_RANGE[static_cast<size_t>(distance)];
}

i32 ChunkStatus::getDistance(const ChunkStatus& status)
{
    static const std::vector<i32> rangeByStatus = computeRangeByStatus();
    const i32 ordinal = status.ordinal();
    if (ordinal >= 0 && ordinal < static_cast<i32>(rangeByStatus.size())) {
        return rangeByStatus[static_cast<size_t>(ordinal)];
    }
    return 0;
}

i32 ChunkStatus::maxDistance()
{
    return static_cast<i32>(STATUS_BY_RANGE.size());
}

} // namespace mc
