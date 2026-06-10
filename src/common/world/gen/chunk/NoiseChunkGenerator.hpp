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
#include "../../biome/BiomeSource.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
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
#include "../surface/SurfaceRules.hpp"
#include "IChunkGenerator.hpp"
#include <memory>
#include <mutex>
#include <vector>

namespace mc {

/**
 * @brief 噪声区块生成器
 *
 * 使用多层噪声生成地形，是主世界和下界的标准地形生成器。
 * MC 1.18+ 使用 BiomeSource（3D 多噪声）替代旧版 BiomeProvider。
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
     * @param seed 世界种子
     * @param settings 维度设置
     * @param biomeSource 生物群系源
     */
    NoiseChunkGenerator(u64 seed, DimensionSettings settings, std::unique_ptr<world::biome::BiomeSource> biomeSource);

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

    // === 生物群系源 ===

    [[nodiscard]] world::biome::BiomeSource* getBiomeSource() override { return m_biomeSource.get(); }
    [[nodiscard]] const world::biome::BiomeSource* getBiomeSource() const override { return m_biomeSource.get(); }

    // === 噪声参数 ===

    [[nodiscard]] const NoiseSettings& noiseSettings() const { return m_settings.noise; }

    // === 结构地形平滑 ===

private:
    // === MC 1.21 密度函数管线 ===
    std::unique_ptr<world::gen::RandomState> m_randomState; ///< 随机状态（持有 NoiseRouter、SurfaceSystem 等）
    i32 m_cellWidth = 4;                                    ///< X/Z 方向 cell 宽度（主世界=4, 末地=8）
    i32 m_cellHeight = 8;                                   ///< Y 方向 cell 高度（主世界=8, 末地=4）

    // === 生物群系 ===
    std::unique_ptr<world::biome::BiomeSource> m_biomeSource;

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 特征拓扑排序（MC 1.21 FeatureSorter）===
    /// 懒初始化：首次调用 placeFeatures 时构建
    std::vector<FeatureSorter::StepFeatureData> m_featuresPerStep;
    std::once_flag m_featuresPerStepFlag;

    // === 核心生成方法 ===

    [[nodiscard]] world::gen::density::Beardifier _buildBeardifier(ChunkPrimer& chunk) const;

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
