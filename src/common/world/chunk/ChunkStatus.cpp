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

// 阶段定义（按顺序）
//
// taskRange 参数说明：
// -1: 不执行任务（EMPTY）
//  0: 不需要邻居区块
//  1: 需要直接相邻区块
//  8: 需要较大范围邻居区块
//
// heightmaps 参数说明：
// - PRE_FEATURES: WORLD_SURFACE_WG | OCEAN_FLOOR_WG（生成时高度图，EMPTY~SURFACE 阶段）
// - POST_FEATURES: WORLD_SURFACE | OCEAN_FLOOR | MOTION_BLOCKING | MOTION_BLOCKING_NO_LEAVES
//   （最终高度图，CARVERS~FULL 阶段，不含 LIGHT_BLOCKING）

// EMPTY: 空区块，刚创建
const ChunkStatus EMPTY("empty",
    EMPTY_ORDINAL,
    nullptr, // 父状态为 nullptr，表示起始状态
    -1,      // taskRange = -1 表示不执行任务
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

// STRUCTURE_STARTS: 结构起点生成
const ChunkStatus STRUCTURE_STARTS(
    "structure_starts", STRUCTURE_STARTS_ORDINAL, &EMPTY, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// STRUCTURE_REFERENCES: 结构引用计算
const ChunkStatus STRUCTURE_REFERENCES("structure_references",
    STRUCTURE_REFERENCES_ORDINAL,
    &STRUCTURE_STARTS,
    8,
    HeightmapFlag::PRE_FEATURES,
    ChunkType::PROTOCHUNK);

// BIOMES: 生物群系生成
const ChunkStatus BIOMES(
    "biomes", BIOMES_ORDINAL, &STRUCTURE_REFERENCES, 0, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// NOISE: 噪声地形生成
const ChunkStatus NOISE("noise", NOISE_ORDINAL, &BIOMES, 8, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// SURFACE: 地表生成
// 需要邻居生物群系数据来正确生成地表
const ChunkStatus SURFACE("surface", SURFACE_ORDINAL, &NOISE, 1, HeightmapFlag::PRE_FEATURES, ChunkType::PROTOCHUNK);

// CARVERS: 雕刻（洞穴、峡谷）
// CARVERS 阶段切换到 POST_FEATURES 高度图
const ChunkStatus CARVERS("carvers", CARVERS_ORDINAL, &SURFACE, 0, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// FEATURES: 特性放置
// directDependencies: [STRUCTURE_STARTS(8), CARVERS(1)]
const ChunkStatus FEATURES(
    "features", FEATURES_ORDINAL, &CARVERS, 8, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// INITIALIZE_LIGHT: 初始化光源
// 将区块中已有光源注册到光照引擎
const ChunkStatus INITIALIZE_LIGHT(
    "initialize_light", INITIALIZE_LIGHT_ORDINAL, &FEATURES, 0, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// LIGHT: 光照传播计算
// directDependencies: [INITIALIZE_LIGHT(1)]
const ChunkStatus LIGHT(
    "light", LIGHT_ORDINAL, &INITIALIZE_LIGHT, 1, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// SPAWN: 生物生成点计算
// directDependencies: [BIOMES(1)]
const ChunkStatus SPAWN("spawn", SPAWN_ORDINAL, &LIGHT, 1, HeightmapFlag::POST_FEATURES, ChunkType::PROTOCHUNK);

// FULL: 完整区块
const ChunkStatus FULL("full", FULL_ORDINAL, &SPAWN, 0, HeightmapFlag::POST_FEATURES, ChunkType::LEVELCHUNK);

} // namespace ChunkStatuses

// ============================================================================
// 状态范围映射
// ============================================================================

namespace {

// 用于将距离值映射到对应的状态
// distance 0: FULL
// distance 1: FEATURES (需要 CARVERS 完成)
// distance 2: CARVERS (需要 STRUCTURE_STARTS 完成)
// distance 3+: STRUCTURE_STARTS
const std::vector<const ChunkStatus*> STATUS_BY_RANGE = {&ChunkStatuses::FULL,
    &ChunkStatuses::FEATURES,
    &ChunkStatuses::CARVERS,
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
        ChunkStatuses::FEATURES,
        ChunkStatuses::INITIALIZE_LIGHT,
        ChunkStatuses::LIGHT,
        ChunkStatuses::SPAWN,
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
