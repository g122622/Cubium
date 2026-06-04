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
#include "../../chunk/ChunkPrimer.hpp"
#include "../carver/CanyonCarver.hpp"
#include "../carver/CaveCarver.hpp"
#include "../carver/UnderwaterCarver.hpp"
#include "../carver/WorldCarver.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../feature/DecorationStage.hpp"
#include "../jigsaw/JigsawJunction.hpp"
#include "../noise/OctavesNoiseGenerator.hpp"
#include "../settings/NoiseSettings.hpp"
#include "../structure/StructureManager.hpp"
#include "../surface/SurfaceBuilders.hpp"
#include "IChunkGenerator.hpp"
#include <array>
#include <memory>
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
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) override;
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
    [[nodiscard]] i32 noiseSizeX() const { return m_noiseSizeX; }
    [[nodiscard]] i32 noiseSizeY() const { return m_noiseSizeY; }
    [[nodiscard]] i32 noiseSizeZ() const { return m_noiseSizeZ; }

    // === 结构地形平滑 ===

    /**
     * @brief 计算结构片段对地形密度的影响
     *
     * 使用 24x24x24 高斯核计算平滑影响值。
     *
     * @param dx X 方向距离（方块坐标差）
     * @param dy Y 方向距离（方块坐标差）
     * @param dz Z 方向距离（方块坐标差）
     * @return 地形密度偏移值
     */
    [[nodiscard]] static f64 calculateStructureDensityOffset(i32 dx, i32 dy, i32 dz);

private:
    /**
     * @brief fillNoiseColumn 的 5x5 生物群系滑窗缓存
     *
     * 该缓存由单次生成流程在栈上持有，避免将可变状态放在生成器实例中，
     * 从而提升并发生成时的线程安全性。
     */
    struct BiomeWindowCache {
        bool valid = false;
        i32 centerNoiseX = 0;
        i32 centerNoiseZ = 0;
        std::array<BiomeId, 25> window{};
    };

    // === 噪声生成器 ===
    std::unique_ptr<OctavesNoiseGenerator> m_mainDensityNoise;         // 主密度噪声 (16倍频)
    std::unique_ptr<OctavesNoiseGenerator> m_secondaryDensityNoise;    // 次密度噪声 (16倍频)
    std::unique_ptr<OctavesNoiseGenerator> m_weightNoise;              // 权重噪声 (8倍频)
    std::unique_ptr<INoiseGenerator> m_surfaceDepthNoise;              // 地表深度噪声（Perlin 或 Octaves）
    std::unique_ptr<OctavesNoiseGenerator> m_randomDensityOffsetNoise; // 随机密度偏移噪声

    // === 生物群系 ===
    std::unique_ptr<world::biome::BiomeSource> m_biomeSource;

    // === 洞穴雕刻器 ===
    std::unique_ptr<CaveCarver> m_caveCarver;
    std::unique_ptr<CanyonCarver> m_canyonCarver;
    ProbabilityConfig m_caveConfig;
    ProbabilityConfig m_canyonConfig;

    // === 水下雕刻器 ===
    std::unique_ptr<world::gen::carver::UnderwaterCaveCarver> m_underwaterCaveCarver;
    std::unique_ptr<world::gen::carver::UnderwaterCanyonCarver> m_underwaterCanyonCarver;
    // 使用与普通洞穴/峡谷相同的概率配置

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 缓存的噪声参数 ===
    i32 m_noiseSizeX;
    i32 m_noiseSizeY;
    i32 m_noiseSizeZ;
    i32 m_verticalNoiseGranularity;
    i32 m_horizontalNoiseGranularity;

    // === 5x5 权重查找表 ===
    std::array<f32, 25> m_biomeWeights;

    // === 核心生成方法 ===

    /**
     * @brief 填充噪声列
     *
     * 计算噪声柱的高度值，这是地形生成的核心算法。
     */
    void _fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ, BiomeWindowCache& biomeWindowCache) const;

    /**
     * @brief 计算噪声密度
     *
     * 计算 3D 噪声采样值。
     */
    [[nodiscard]] f32 _calculateNoiseDensity(
        i32 noiseX, i32 noiseY, i32 noiseZ, f32 xzScale, f32 yScale, f32 xzFactor, f32 yFactor) const;

    /**
     * @brief 计算随机密度偏移
     *
     * 用于增加地形的随机变化。
     */
    [[nodiscard]] f32 _calculateRandomDensityOffset(i32 noiseX, i32 noiseZ) const;

    /**
     * @brief 判断密度值对应的方块
     * @return 方块状态指针，nullptr 表示空气
     */
    [[nodiscard]] const BlockState* _getBlockForDensity(f32 density, i32 y) const;

    // === JigsawJunction 地形平滑 ===

    /**
     * @brief 收集区块附近的结构片段和 JigsawJunction
     *
     * 收集 12 格范围内的所有 JigsawJunction 用于地形平滑。
     *
     * @param chunk 区块
     * @param outPieces 输出：结构片段列表
     * @param outJunctions 输出：JigsawJunction 列表
     */
    void _collectStructureData(ChunkPrimer& chunk,
        std::vector<const world::gen::structure::StructurePiece*>& outPieces,
        std::vector<world::gen::jigsaw::JigsawJunction>& outJunctions) const;

    /**
     * @brief 初始化高斯查找表
     *
     * 预计算 24x24x24 高斯核用于地形平滑。
     */
    static void _initGaussianLUT();

    void _initNoiseGenerators();
    void _initBiomeWeights();

    /**
     * @brief 初始化洞穴/峡谷雕刻器与概率配置
     *
     * @note 该方法必须在所有构造路径中调用，避免生成阶段出现行为分叉。
     */
    void _initCarvers();

    /**
     * @brief 初始化结构与放置器注册表
     *
     * @warning 该方法会触发全局注册逻辑，调用方需保证幂等语义。
     */
    void _initGenerationRegistries();

    // === 地表生成 ===

    /**
     * @brief 采样地表深度噪声
     *
     * 该方法统一封装 Perlin/Octaves 两种地表噪声路径。
     */
    [[nodiscard]] f32 _sampleSurfaceDepthNoise(i32 worldX, i32 worldZ, i32 localX) const;

    /**
     * @brief 生成顶部/底部基岩层
     *
     * 使用维度设置中的基岩锚点控制上下基岩。
     */
    void _applyBedrock(ChunkPrimer& chunk, math::Random& random) const;

    void _buildSurfaceForColumn(
        ChunkPrimer& chunk, math::Random& random, i32 x, i32 z, i32 startHeight, f32 surfaceNoise, BiomeId biome);

    // === 24x24x24 高斯查找表 ===
    // 预计算的高斯核，用于结构边界地形平滑
    // 索引公式: x * 24 * 24 + y * 24 + z (偏移 +12)
    static std::array<f32, 13824> s_gaussianLUT;
    static bool s_gaussianLUTInitialized;
};

} // namespace mc
