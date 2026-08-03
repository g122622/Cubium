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
 * The above copyright notice shall be included in all copies or substantial portions
 * of the Software.
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

#include "../settings/FlatLevelGeneratorSettings.hpp"
#include "IChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/NoiseColumn.hpp"
#include "common/world/gen/feature/FeatureSorter.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureCheck.hpp"
#include "common/world/gen/structure/StructureManager.hpp"
#include "common/world/gen/structure/StructureSet.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace mc {

/**
 * @brief 平坦世界区块生成器
 *
 * 生成平坦层次的世界地形。
 *
 * 特点：
 * - 使用 FlatLevelGeneratorSettings 定义层配置
 * - generateNoise: 逐层填充方块（基岩、泥土、草方块等）
 * - generateStructureStarts: 根据 structureOverrides 生成结构起点
 * - generateStructureReferences: 扫描邻居区块建立结构引用
 * - buildSurface/applyCarvers/spawnInitialMobs: 空操作
 * - placeFeatures: 根据 decoration/addLakes 标志放置特性和填充层，
 *   同时按装饰阶段放置结构方块
 * - 所有位置返回固定生物群系
 *
 * MC 默认配置：
 * - 1x Bedrock + 2x Dirt + 1x Grass Block
 * - 生物群系: Plains
 * - 海平面: -63（低于世界底部，无水）
 * - structureOverrides: minecraft:villages, minecraft:strongholds
 */
class FlatChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造平坦世界区块生成器
     * @param seed 世界种子
     * @param settings 平坦世界生成设置
     */
    FlatChunkGenerator(u64 seed, FlatLevelGeneratorSettings settings);

    ~FlatChunkGenerator() override;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(
        WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;
    [[nodiscard]] i32 getGroundHeight() const override;

    [[nodiscard]] world::biome::IBiomeSource* getBiomeSource() override { return m_biomeSource.get(); }
    [[nodiscard]] const world::biome::IBiomeSource* getBiomeSource() const override { return m_biomeSource.get(); }

    // === 结构缓存 ===

    [[nodiscard]] world::gen::structure::StructureCheck* structureCheck() override
    {
        return m_structureManager ? &m_structureManager->structureCheck() : nullptr;
    }
    [[nodiscard]] const world::gen::structure::StructureCheck* structureCheck() const override
    {
        return m_structureManager ? &m_structureManager->structureCheck() : nullptr;
    }

    /**
     * @brief 清理结构生成缓存
     *
     * 释放 StructureCheck 中的缓存数据。
     * 在维度卸载时由 ServerDimension::shutdown() 显式调用。
     */
    void clearStructureCache() override;

    [[nodiscard]] i32 getGenDepth() const override { return world::CHUNK_HEIGHT; }
    [[nodiscard]] i32 getMinY() const override { return 0; }
    i32 seaLevel() const override { return -63; }

    /**
     * @brief 获取指定位置的基础列方块状态
     *
     * 返回展开层列表中的方块状态，null 替换为空气。
     */
    [[nodiscard]] NoiseColumn getBaseColumn(i32 x, i32 z) const override;

private:
    /**
     * @brief 在 TOP_LAYER_MODIFICATION 阶段填充非运动阻挡层
     *
     * 对 FlatLevelGeneratorSettings 中标记为 nullptr 的层位置（如水层），
     * 在湖泊等特性放置之后，将空气方块替换为原始方块状态。
     */
    void _placeFillLayers(WorldGenRegion& region, const BlockPos& chunkOrigin);

    /**
     * @brief 初始化结构与放置器注册表
     *
     * 初始化 StructureRegistry、StructureSetRegistry，并创建 StructureManager。
     * 线程安全，仅初始化一次。
     */
    void _initGenerationRegistries();

    /**
     * @brief 检查结构集是否与平坦世界生物群系兼容
     *
     * 参考 MC 1.21.11 ChunkGeneratorStructureState.hasBiomesForStructureSet()：
     * 如果结构集中的任何结构的有效生物群系列表与 FixedBiomeSource 的唯一生物群系
     * 有交集，则认为该结构集与当前生物群系源兼容。
     *
     * @param structureSet 结构集合
     * @return true 如果结构集与平坦世界生物群系兼容
     */
    [[nodiscard]] bool _hasBiomesForStructureSet(const world::gen::structure::StructureSet& structureSet) const;

    FlatLevelGeneratorSettings m_flatSettings;
    std::unique_ptr<world::biome::IBiomeSource> m_biomeSource;

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 特性拓扑排序（MC 1.21 FeatureSorter）===
    /// 懒初始化：首次调用 placeFeatures 时构建
    std::vector<FeatureSorter::StepFeatureData> m_featuresPerStep;
    std::once_flag m_featuresPerStepFlag;

    // === 结构注册表初始化标志 ===
    std::once_flag m_generationRegistriesFlag;
};

} // namespace mc
