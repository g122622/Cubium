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

#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../biome/BiomeManager.hpp"
#include "../../biome/BiomeSource.hpp"
#include "../RandomState.hpp"
#include "../carver/CarverConfiguration.hpp"
#include "../carver/CarvingContext.hpp"
#include "../density/Beardifier.hpp"
#include "../density/NoiseChunk.hpp"
#include "../density/NoiseRouter.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../feature/DecorationStage.hpp"
#include "../feature/FeatureSorter.hpp"
#include "../jigsaw/JigsawJunction.hpp"
#include "../settings/NoiseSettings.hpp"
#include "../structure/StructureManager.hpp"
#include "../structure/StructureSet.hpp"
#include "../surface/SurfaceRules.hpp"
#include "IChunkGenerator.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/aquifer/FluidStatus.hpp"
#include "common/world/gen/chunk/NoiseColumn.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/structure/StructureCheck.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace mc {

/**
 * @brief 噪声区块生成器
 *
 * 使用多层噪声生成地形，是主世界和下界的标准地形生成器。
 * MC 1.18+ 使用 IBiomeSource（3D 多噪声）替代旧版 BiomeProvider。
 *
 * 使用方法：
 * @code
 * DimensionSettings settings = DimensionSettings::overworld();
 * NoiseChunkGenerator generator(seed, std::move(settings), std::move(biomeSource));
 *
 * ChunkPrimer primer(chunkX, chunkZ);
 * generator.generateBiomes(region, primer);
 * generator.generateNoise(region, primer);
 * generator.buildSurface(region, primer);
 * @endcode
 */
class NoiseChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造噪声区块生成器（带生物群系源）
     * @param settings 维度设置
     * @param biomeSource 生物群系源
     * @param randomState 世界随机状态（与生物群系源共享同一缓存，持有 NoiseRouter/SurfaceSystem 等）
     */
    NoiseChunkGenerator(DimensionSettings settings,
        std::unique_ptr<world::biome::IBiomeSource> biomeSource,
        std::shared_ptr<world::gen::RandomState> randomState);

    ~NoiseChunkGenerator() override;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(
        WorldGenRegion& region, ChunkPrimer& chunk, std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;
    [[nodiscard]] i32 getGroundHeight() const override { return m_settings.seaLevel + 1; }
    [[nodiscard]] NoiseColumn getBaseColumn(i32 x, i32 z) const override;
    [[nodiscard]] i32 getGenDepth() const override { return m_settings.noise.height; }
    [[nodiscard]] i32 getMinY() const override { return m_settings.noise.minY; }

    // === 生物群系源 ===

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
     * 释放 StructureCheck 中的缓存数据（m_loadedChunks 和 m_featureChecks）。
     * 在维度卸载时由 ServerDimension::shutdown() 显式调用，
     * 而非等到生成器析构时才清理，对齐 MC 1.21.11 中 ServerLevel 卸载时
     * 立即清理 StructureCheck 的行为。
     */
    void clearStructureCache() override;

    // === 随机状态访问 ===

    /**
     * @brief 获取世界生成随机状态
     *
     * MC 1.21.11: ServerChunkCache.randomState()
     * 持有 NoiseRouter、Climate::Sampler、SurfaceSystem 等。
     * 出生点查找通过 sampler().findSpawnPosition() 完成。
     *
     * @return 随机状态共享指针
     */
    [[nodiscard]] const std::shared_ptr<world::gen::RandomState>& randomState() const { return m_randomState; }

    // === 结构地形平滑 ===

private:
    // === MC 1.21 密度函数管线 ===
    std::shared_ptr<world::gen::RandomState>
        m_randomState;    ///< 随机状态（与生物群系源共享，持有 NoiseRouter、SurfaceSystem 等）
    i32 m_cellWidth = 4;  ///< X/Z 方向 cell 宽度（主世界=4, 末地=8）
    i32 m_cellHeight = 8; ///< Y 方向 cell 高度（主世界=8, 末地=4）

    // === 生物群系 ===
    std::unique_ptr<world::biome::IBiomeSource> m_biomeSource;

    // === MC 1.21 生物群系管理器（Voronoi 缩放查询）===
    std::unique_ptr<world::biome::BiomeManager> m_biomeManager;

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 特征拓扑排序（MC 1.21 FeatureSorter）===
    /// 懒初始化：首次调用 placeFeatures 时构建
    std::vector<FeatureSorter::StepFeatureData> m_featuresPerStep;
    std::once_flag m_featuresPerStepFlag;

    // === MC 1.21 全局流体选择器（缓存，避免每次创建）===
    world::gen::aquifer::FluidPicker m_globalFluidPicker;

    /**
     * @brief 检查结构集中的任何结构是否可能与当前维度的生物群系兼容
     *
     * 对齐 MC 1.21.11 ChunkGeneratorStructureState.hasBiomesForStructureSet()，
     * 检查 BiomeSource 的 possibleBiomes 是否与结构集中任意结构的 biomeTag 有交集。
     * 如果没有交集，则该结构集在当前维度中永远不可能生成，可以跳过。
     *
     * @param structureSet 结构集
     * @return 是否可能生成
     */
    [[nodiscard]] bool _hasBiomesForStructureSet(const world::gen::structure::StructureSet& structureSet) const;

    // === 核心生成方法 ===

    [[nodiscard]] world::gen::density::Beardifier _buildBeardifier(WorldGenRegion& region, ChunkPrimer& chunk) const;

    /**
     * @brief 初始化结构与放置器注册表
     *
     * @warning 该方法会触发全局注册逻辑，调用方需保证幂等语义。
     */
    void _initGenerationRegistries();

    /**
     * @brief 初始化 MC 1.21 密度函数管线
     * 创建 NoiseRouter 并配置 cell 大小参数。
     */
    void _initDensityFunctionPipeline();

    /**
     * @brief 使用 MC 1.21 NoiseChunk 生成噪声地形
     * 基于 cell 的三线性插值管线，替代旧的 _fillNoiseColumn 方法。
     */
    void _generateNoiseWithDensityFunction(WorldGenRegion& region, ChunkPrimer& chunk);
};

} // namespace mc
