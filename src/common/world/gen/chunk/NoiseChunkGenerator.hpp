#pragma once

#include "IChunkGenerator.hpp"
#include "../noise/OctavesNoiseGenerator.hpp"
#include "../settings/NoiseSettings.hpp"
#include "../carver/WorldCarver.hpp"
#include "../carver/CaveCarver.hpp"
#include "../carver/CanyonCarver.hpp"
#include "../carver/UnderwaterCarver.hpp"
#include "../feature/DecorationStage.hpp"
#include "../feature/ConfiguredFeature.hpp"
#include "../structure/StructureManager.hpp"
#include "../surface/SurfaceBuilders.hpp"
#include "../../biome/BiomeProvider.hpp"
#include "../../biome/BiomeGenerationSettings.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include <array>
#include <memory>

namespace mc {

/**
 * @brief 噪声区块生成器
 *
 * 参考 MC NoiseChunkGenerator，使用多层噪声生成地形。
 * 这是主世界和下界的标准地形生成器。
 *
 * 使用方法：
 * @code
 * DimensionSettings settings = DimensionSettings::overworld();
 * NoiseChunkGenerator generator(seed, std::move(settings));
 *
 * ChunkPrimer primer(chunkX, chunkZ);
 * generator.generateBiomes(region, primer);
 * generator.generateNoise(region, primer);
 * generator.buildSurface(region, primer);
 * @endcode
 *
 * @note 参考 MC 1.16.5 NoiseChunkGenerator 实现
 */
class NoiseChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造噪声区块生成器
     * @param seed 世界种子
     * @param settings 维度设置
     */
    NoiseChunkGenerator(u64 seed, DimensionSettings settings);

    /**
     * @brief 构造噪声区块生成器（带生物群系提供者）
     * @param seed 世界种子
     * @param settings 维度设置
     * @param biomeProvider 生物群系提供者
     */
    NoiseChunkGenerator(u64 seed, DimensionSettings settings, std::unique_ptr<BiomeProvider> biomeProvider);

    ~NoiseChunkGenerator() override;

    // === IChunkGenerator 接口 ===

    void generateStructureStarts(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateStructureReferences(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateBiomes(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void generateNoise(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void buildSurface(WorldGenRegion& region, ChunkPrimer& chunk) override;
    void applyCarvers(WorldGenRegion& region, ChunkPrimer& chunk, bool isLiquid) override;
    void placeFeatures(WorldGenRegion& region, ChunkPrimer& chunk) override;
    i32 spawnInitialMobs(WorldGenRegion& region, ChunkPrimer& chunk,
                          std::vector<SpawnedEntityData>& outEntities) override;

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z, HeightmapType type) const override;
    [[nodiscard]] i32 getGroundHeight() const override { return 64; }

    // === 噪声参数 ===

    [[nodiscard]] const NoiseSettings& noiseSettings() const { return m_settings.noise; }
    [[nodiscard]] i32 noiseSizeX() const { return m_noiseSizeX; }
    [[nodiscard]] i32 noiseSizeY() const { return m_noiseSizeY; }
    [[nodiscard]] i32 noiseSizeZ() const { return m_noiseSizeZ; }

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
    std::unique_ptr<OctavesNoiseGenerator> m_mainDensityNoise;      // 主密度噪声 (16倍频)
    std::unique_ptr<OctavesNoiseGenerator> m_secondaryDensityNoise; // 次密度噪声 (16倍频)
    std::unique_ptr<OctavesNoiseGenerator> m_weightNoise;           // 权重噪声 (8倍频)
    std::unique_ptr<INoiseGenerator> m_surfaceDepthNoise;           // 地表深度噪声（Perlin 或 Octaves）
    std::unique_ptr<OctavesNoiseGenerator> m_randomDensityOffsetNoise; // 随机密度偏移噪声

    // === 生物群系 ===
    std::unique_ptr<BiomeProvider> m_biomeProvider;

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

    // === 5x5 权重查找表（参考 MC field_236081_j_）===
    std::array<f32, 25> m_biomeWeights;

    // === 核心生成方法 ===

    /**
     * @brief 填充噪声列
     *
     * 参考 MC fillNoiseColumn，计算噪声柱的高度值。
     * 这是地形生成的核心算法。
     */
    void fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ, BiomeWindowCache& biomeWindowCache) const;

    /**
     * @brief 计算噪声密度
     *
     * 参考 MC func_222552_a，计算 3D 噪声采样值。
     */
    [[nodiscard]] f32 calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ,
                                             f32 xzScale, f32 yScale,
                                             f32 xzFactor, f32 yFactor) const;

    /**
     * @brief 计算随机密度偏移
     *
     * 参考 MC func_236095_c_，用于增加地形的随机变化。
     */
    [[nodiscard]] f32 calculateRandomDensityOffset(i32 noiseX, i32 noiseZ) const;

    /**
     * @brief 判断密度值对应的方块
     * @return 方块状态指针，nullptr 表示空气
     */
    [[nodiscard]] const BlockState* getBlockForDensity(f32 density, i32 y) const;

    // === 初始化方法 ===

    void initNoiseGenerators();
    void initBiomeWeights();

    /**
     * @brief 初始化洞穴/峡谷雕刻器与概率配置
     *
     * @note 该方法必须在所有构造路径中调用，避免生成阶段出现行为分叉。
     */
    void initCarvers();

    /**
     * @brief 初始化结构与放置器注册表
     *
     * @warning 该方法会触发全局注册逻辑，调用方需保证幂等语义。
     */
    void initGenerationRegistries();

    // === 地表生成 ===

    /**
     * @brief 采样地表深度噪声
     *
     * 该方法统一封装 Perlin/Octaves 两种地表噪声路径，
     * 并尽量对齐 MC 1.16.5 的调用参数。
     */
    [[nodiscard]] f32 sampleSurfaceDepthNoise(i32 worldX, i32 worldZ, i32 localX) const;

    /**
     * @brief 生成顶部/底部基岩层
     *
     * 参考 MC makeBedrock，使用维度设置中的基岩锚点控制上下基岩。
     */
    void applyBedrock(ChunkPrimer& chunk, math::Random& random) const;

    void buildSurfaceForColumn(
        ChunkPrimer& chunk,
        math::Random& random,
        i32 x,
        i32 z,
        i32 startHeight,
        f32 surfaceNoise,
        BiomeId biome);
};

} // namespace mc
