/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted copies of the following:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"

#include <memory>
#include <unordered_map>

namespace mc::world::chunk {
class ChunkPrimer;
}

namespace mc::server {

// ============================================================================
// 区块状态快照
// ============================================================================

/**
 * @brief 不可变的区块生成状态快照
 *
 * 对齐 Moonrise 的 `GenerationChunkHolder.getChunkIfPresentUnchecked` 返回的不可变 ChunkAccess。
 * WorldGenRegion 通过 ChunkSnapshot 访问邻居区块数据，保证生成执行期间邻居数据不变。
 *
 * 持有以下数据（从 ChunkPrimer 深拷贝，发布后不可变）：
 * - `shared_ptr<const ChunkData>`：方块状态数据（深拷贝自 ChunkPrimer 的 m_data）
 * - `unordered_map<HeightmapType, Heightmap>`：高度图（深拷贝自 ChunkPrimer 的 m_heightmaps）
 * - `BiomeContainer`：生物群系（深拷贝自 ChunkPrimer 的 m_biomes）
 * - `const ChunkStatus*`：该快照对应的生成状态
 *
 * 深拷贝原因：ChunkPrimer 非线程安全，生成任务在执行线程持有 primer 并修改它。
 * 快照在任务完成时创建一次，之后只读，可安全地被其他线程（WorldGenRegion）读取。
 *
 * 高度图/生物群系单独存储：ChunkPrimer 的高度图和生物群系存在 primer 自身（不在 m_data 上），
 * 而 getTopBlockY/getBiomeAtBlock 从 primer 自身读取，所以快照必须独立持有这两者。
 * ChunkData 的 m_heightmaps 未同步（primer 的高度图在 primer 上），因此快照用自己的 m_heightmaps。
 *
 * 初版采用深拷贝（决策 10），后续可优化为 COW（Copy-on-Write）或 shared_ptr 共享。
 */
class ChunkSnapshot {
public:
    /**
     * @brief 从 ChunkPrimer 创建不可变快照
     *
     * 深拷贝 primer 的方块状态、高度图、生物群系。调用后 primer 数据不受影响（非破坏性）。
     *
     * @param primer 源 ChunkPrimer（必须已初始化 m_data）
     * @param status 该快照对应的生成状态
     * @return 不可变快照
     */
    [[nodiscard]] static ChunkSnapshot fromPrimer(
        const mc::world::chunk::ChunkPrimer& primer, const mc::world::chunk::ChunkStatus& status);

    ChunkSnapshot() = default;
    ~ChunkSnapshot() = default;

    // 不可变快照：允许移动构造/赋值（shared_ptr 移动），禁止拷贝（深拷贝昂贵）
    ChunkSnapshot(ChunkSnapshot&&) noexcept = default;
    ChunkSnapshot& operator=(ChunkSnapshot&&) noexcept = default;
    ChunkSnapshot(const ChunkSnapshot&) = delete;
    ChunkSnapshot& operator=(const ChunkSnapshot&) = delete;

    /** @brief 区块 X 坐标 */
    [[nodiscard]] mc::ChunkCoord x() const noexcept { return m_x; }

    /** @brief 区块 Z 坐标 */
    [[nodiscard]] mc::ChunkCoord z() const noexcept { return m_z; }

    /** @brief 该快照对应的生成状态 */
    [[nodiscard]] const mc::world::chunk::ChunkStatus& status() const noexcept { return *m_status; }

    /** @brief 方块状态数据（不可变） */
    [[nodiscard]] const mc::world::chunk::ChunkData& data() const noexcept { return *m_data; }

    /** @brief 方块状态数据的 shared_ptr（用于共享所有权） */
    [[nodiscard]] const std::shared_ptr<const mc::world::chunk::ChunkData>& dataPtr() const noexcept { return m_data; }

    /**
     * @brief 获取指定类型的高度图
     *
     * 若该类型高度图不存在，返回 nullptr。
     */
    [[nodiscard]] const mc::world::chunk::Heightmap* heightmap(mc::world::chunk::HeightmapType type) const;

    /** @brief 是否包含指定类型的高度图 */
    [[nodiscard]] bool hasHeightmap(mc::world::chunk::HeightmapType type) const
    {
        return m_heightmaps.find(type) != m_heightmaps.end();
    }

    /** @brief 所有高度图（只读） */
    [[nodiscard]] const std::unordered_map<mc::world::chunk::HeightmapType, mc::world::chunk::Heightmap>&
    heightmaps() const noexcept
    {
        return m_heightmaps;
    }

    /** @brief 生物群系（只读） */
    [[nodiscard]] const mc::world::chunk::BiomeContainer& biomes() const noexcept { return m_biomes; }

    /** @brief 快照是否有效（已通过 fromPrimer 初始化） */
    [[nodiscard]] bool isValid() const noexcept { return m_data != nullptr && m_status != nullptr; }

private:
    ChunkSnapshot(mc::ChunkCoord x,
        mc::ChunkCoord z,
        const mc::world::chunk::ChunkStatus* status,
        std::shared_ptr<const mc::world::chunk::ChunkData> data,
        std::unordered_map<mc::world::chunk::HeightmapType, mc::world::chunk::Heightmap> heightmaps,
        mc::world::chunk::BiomeContainer biomes)
        : m_x(x)
        , m_z(z)
        , m_status(status)
        , m_data(std::move(data))
        , m_heightmaps(std::move(heightmaps))
        , m_biomes(std::move(biomes))
    {}

    mc::ChunkCoord m_x = 0;
    mc::ChunkCoord m_z = 0;
    const mc::world::chunk::ChunkStatus* m_status = nullptr;
    std::shared_ptr<const mc::world::chunk::ChunkData> m_data;
    std::unordered_map<mc::world::chunk::HeightmapType, mc::world::chunk::Heightmap> m_heightmaps;
    mc::world::chunk::BiomeContainer m_biomes;
};

} // namespace mc::server
