#pragma once

#include "IChunkGenerator.hpp"
#include "../noise/OctavesNoiseGenerator.hpp"
#include "../settings/NoiseSettings.hpp"
#include "../structure/StructureManager.hpp"
#include "../../biome/provider/nether/NetherBiomeProvider.hpp"
#include "../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

/**
 * @brief 下界区块生成器
 *
 * 参考 MC 1.16.5 NetherChunkGenerator
 * 专门用于下界维度的区块生成器，与主世界有以下区别：
 *
 * - 高度范围：0-127（主世界 0-255）
 * - 基岩天花板：Y=127
 * - 熔岩海：Y=31
 * - 无天空光
 * - 不同的地形生成参数
 * - 使用 NetherBiomeProvider（3D 生物群系采样）
 *
 * 使用示例：
 * @code
 * NetherChunkGenerator generator(seed);
 * generator.generateBiomes(region, primer);
 * generator.generateNoise(region, primer);
 * generator.buildSurface(region, primer);
 * @endcode
 */
class NetherChunkGenerator : public BaseChunkGenerator {
public:
    /**
     * @brief 构造下界区块生成器
     * @param seed 世界种子
     */
    explicit NetherChunkGenerator(u64 seed);

    /**
     * @brief 构造下界区块生成器（带自定义设置）
     * @param seed 世界种子
     * @param settings 维度设置
     */
    NetherChunkGenerator(u64 seed, DimensionSettings settings);

    ~NetherChunkGenerator() override = default;

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

    // === 下界特有参数 ===

    /**
     * @brief 获取熔岩海高度
     */
    [[nodiscard]] i32 lavaLevel() const { return m_lavaLevel; }

    /**
     * @brief 获取基岩天花板高度
     */
    [[nodiscard]] i32 bedrockCeiling() const { return m_bedrockCeiling; }

    /**
     * @brief 获取基岩地板高度
     */
    [[nodiscard]] i32 bedrockFloor() const { return m_bedrockFloor; }

private:
    // === 噪声生成器 ===
    std::unique_ptr<OctavesNoiseGenerator> m_mainDensityNoise;      // 主密度噪声
    std::unique_ptr<OctavesNoiseGenerator> m_secondaryDensityNoise; // 次密度噪声
    std::unique_ptr<SimplexNoiseGenerator> m_simplexNoise;          // Simplex 噪声

    // === 生物群系提供者 ===
    std::unique_ptr<biome::nether::NetherBiomeProvider> m_biomeProvider;

    // === 结构管理器 ===
    std::unique_ptr<world::gen::structure::StructureManager> m_structureManager;

    // === 下界特有参数 ===
    i32 m_lavaLevel = 31;        // 熔岩海高度
    i32 m_bedrockCeiling = 127;  // 基岩天花板
    i32 m_bedrockFloor = 0;      // 基岩地板

    // === 缓存的噪声参数 ===
    i32 m_noiseSizeX;
    i32 m_noiseSizeY;
    i32 m_noiseSizeZ;

    // === 随机数生成 ===
    mutable math::Random m_random;

    // === 核心生成方法 ===

    /**
     * @brief 填充噪声列
     *
     * 下界使用类似主世界的密度噪声算法，但参数不同。
     */
    void fillNoiseColumn(std::vector<f32>& column, i32 noiseX, i32 noiseZ) const;

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
     * @brief 生成基岩层
     * @param chunk 区块
     * @param x 本地 X 坐标 (0-15)
     * @param z 本地 Z 坐标 (0-15)
     */
    void generateBedrock(ChunkPrimer& chunk, i32 x, i32 z);

    // === 初始化方法 ===
    void initNoiseGenerators();
    void initSettings();
};

} // namespace mc
