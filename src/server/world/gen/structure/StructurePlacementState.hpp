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
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/world/gen/structure/StructureSet.hpp"
#include "server/world/gen/structure/placement/ConcentricRingsStructurePlacement.hpp"
#include "server/world/gen/structure/placement/StructurePlacement.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::world::gen::structure {

/**
 * @brief 一个维度内的结构放置状态
 *
 * 对应原版 `ChunkGeneratorStructureState`：把「哪些结构集在这个维度里可能出现」与
 * 「每个结构集在哪些区块里落点」这两件事从放置规则对象里剥出来，集中持有。
 *
 * 之所以需要它，是因为放置规则自身只能回答「给定世界种子，这个区块是不是落点」，
 * 而原版的判定里有一层**跨结构集的约束**——排斥区（掠夺者前哨站必须远离村庄）。
 * 那个判定要问「另一个结构集在附近有没有落点」，也就是需要看到整张结构集表，
 * 单个 `StructurePlacement` 看不到。
 *
 * 同心环（要塞）也是同理：它的落点不是逐区块算出来的，而是整张表按序生成再查表；
 * 那张表属于维度的状态，而不属于任何单个放置规则。
 */
class StructurePlacementState {
public:
    /**
     * @brief 构造一个维度的放置状态
     *
     * 构造期按原版的快速预过滤剔除不可能出现的结构集：结构集里所有结构的群系标签与
     * 本维度群系源的可能群系**完全不相交**时，该结构集在本维度永远不会生成，直接不参与。
     *
     * @param biomeSource 本维度的群系源（提供可能群系，并与生成器共用同一个群系源）
     * @param worldSeed 世界种子
     */
    StructurePlacementState(const biome::IBiomeSource& biomeSource, i64 worldSeed);

    /**
     * @brief 登记一个结构集（由注册表逐个喂入）
     *
     * 与构造期的预过滤一致：群系不相交的结构集被静默跳过。
     * 结构集必须比本状态活得更久——这里只持有指针。
     */
    void addStructureSet(const StructureSet& structureSet);

    /** @brief 通过预过滤的结构集（按登记顺序）。 */
    [[nodiscard]] const std::vector<const StructureSet*>& structureSets() const noexcept
    {
        return m_structureSets;
    }

    [[nodiscard]] i64 worldSeed() const noexcept
    {
        return m_worldSeed;
    }

    /**
     * @brief 结构集是否在指定区块落点
     *
     * 对应原版 `StructurePlacement#isStructureChunk`：
     * 落点碎片判定 → 频率缩减 → 排斥区。同心环直接查预生成的环形位置表。
     */
    [[nodiscard]] bool isStructureChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 某结构集在以 (chunkX, chunkZ) 为中心、半径 chunkCount 的方块区块范围内是否有落点
     *
     * 排斥区判定用的就是它。搜索按 X 主序、Z 次序（与原版双层循环一致），
     * 任何一处命中即返回 true。
     */
    [[nodiscard]] bool hasStructureChunkInRange(
        const StructureSet& structureSet, i32 chunkX, i32 chunkZ, i32 chunkCount) const;

    // === 单步判定（供诊断与差分逐层定位） ===

    /** 落点碎片判定：这个区块是不是该放置规则的候选区块（不含频率与排斥区）。 */
    [[nodiscard]] bool isPlacementChunk(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const;

    /** 频率缩减：候选区块里真正生成的比例。 */
    [[nodiscard]] bool passesFrequencyReduction(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const;

    /** 排斥区：附近的另一个结构集是否占据了这块空间。 */
    [[nodiscard]] bool passesExclusionZone(const StructureSet& structureSet, i32 chunkX, i32 chunkZ) const;

private:
    /**
     * @brief 同心环位置表（惰性生成并按放置规则缓存）
     *
     * 键是指向放置规则的指针——放置规则由结构集拥有，结构集由注册表拥有，
     * 在状态存活期间地址稳定。
     */
    [[nodiscard]] const std::vector<world::chunk::ChunkPos>& ringPositions(
        const placement::ConcentricRingsStructurePlacement& concentric) const;

    const biome::IBiomeSource& m_biomeSource;
    i64 m_worldSeed;
    std::vector<const StructureSet*> m_structureSets;
    /// 结构集 → 是否含同心环放置（判定时省一次 dynamic_cast）
    std::unordered_map<const StructureSet*, const placement::ConcentricRingsStructurePlacement*> m_concentric;
};

} // namespace mc::world::gen::structure