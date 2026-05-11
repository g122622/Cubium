#pragma once

#include "IChunkGenerator.hpp"
#include "../noise/OctavesNoiseGenerator.hpp"
#include "../settings/NoiseSettings.hpp"
#include "../structure/StructureManager.hpp"
#include "../../biome/provider/end/EndBiomeProvider.hpp"
#include "../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

/**
 * @brief 末地区块生成器
 *
 * 参考 MC 1.16.5 EndChunkGenerator
 * 专门用于末地维度的区块生成器，与主世界和下界有以下区别：
 *
 * - 高度范围：0-255
 * - 主岛：中心 (0, 0) 半径约 96 方块
 * - 外岛：距离主岛 1000+ 方块
 * - 无天空光
 * - 中央岛屿：末地石和黑曜石柱
 * - 外岛：末地石、紫颂树、末地城
 * - 使用 EndBiomeProvider（2D 噪声采样区分主岛/外岛）
 *
 * 使用示例：
 * @code
 * EndChunkGenerator generator(seed);
 * generator.generateBiomes(region, primer);
 * generator.generateNoise(region, primer);
 * generator.buildSurface(region, primer);
 * @endcode
 */
class EndChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造末地区块生成器
     * @param seed 世界种子
     */
    explicit EndChunkGenerator(u64 seed);

    /**
     * @brief 构造末地区块生成器（带自定义设置）
     * @param seed 世界种子
     * @param settings 维度设置
     */
    EndChunkGenerator(u64 seed, DimensionSettings settings);

    ~EndChunkGenerator() override = default;

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

    // === 生物群系提供者 ===

    [[nodiscard]] BiomeProvider* getBiomeProvider() override { return m_biomeProvider.get(); }
    [[nodiscard]] const BiomeProvider* getBiomeProvider() const override { return m_biomeProvider.get(); }

    // === 末地特有参数 ===

    /**
     * @brief 获取主岛半径
     */
    [[nodiscard]] i32 mainIslandRadius() const { return m_mainIslandRadius; }

    /**
     * @brief 检查位置是否在主岛范围内
     */
    [[nodiscard]] bool isInMainIsland(i32 x, i32 z) const;

private:
    // === 噪声生成器 ===
    std::unique_ptr<SimplexNoiseGenerator> m_islandNoise;      // 岛屿噪声
    std::unique_ptr<OctavesNoiseGenerator> m_densityNoise;     // 密度噪声

    // === 生物群系提供者 ===
    std::unique_ptr<biome::end::EndBiomeProvider> m_biomeProvider;

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 末地特有参数 ===
    i32 m_mainIslandRadius = 256;  // 主岛半径（方块单位），MC 1.16.5 使用 sqrt(4096) * 4 = 256
    i32 m_endIslandHeight = 64;    // 末地岛高度
    f32 m_islandNoiseThreshold = 1.0f;  // 岛屿生成阈值

    // === 缓存的噪声参数 ===
    i32 m_noiseSizeX;
    i32 m_noiseSizeY;
    i32 m_noiseSizeZ;

    // === 随机数生成 ===
    mutable math::Random m_random;

    // === 核心生成方法 ===

    /**
     * @brief 计算岛屿高度
     * @param x 世界 X 坐标
     * @param z 世界 Z 坐标
     * @return 岛屿高度（0 表示无岛屿）
     */
    [[nodiscard]] f32 calculateIslandHeight(i32 x, i32 z) const;

    /**
     * @brief 计算噪声密度
     */
    [[nodiscard]] f32 calculateNoiseDensity(i32 noiseX, i32 noiseY, i32 noiseZ) const;

    /**
     * @brief 判断密度值对应的方块
     * @param density 密度值
     * @param y Y 坐标
     * @return 方块状态指针，nullptr 表示空气
     */
    [[nodiscard]] const BlockState* getBlockForDensity(f32 density, i32 y) const;

    /**
     * @brief 生成主岛
     * @param chunk 区块
     */
    void generateMainIsland(ChunkPrimer& chunk);

    /**
     * @brief 生成外岛
     * @param chunk 区块
     */
    void generateOuterIslands(ChunkPrimer& chunk);

    /**
     * @brief 生成末地黑曜石柱
     * @param chunk 区块
     */
    void generateObsidianPillars(ChunkPrimer& chunk);

    /**
     * @brief 判断区块是否在主岛范围内
     */
    [[nodiscard]] bool isChunkInMainIsland(ChunkCoord chunkX, ChunkCoord chunkZ) const;

    // === 初始化方法 ===
    void initNoiseGenerators();
    void initSettings();
};

} // namespace mc
