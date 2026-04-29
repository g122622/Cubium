#pragma once

#include "SurfaceBuilder.hpp"

namespace mc {

/**
 * @brief 默认地表构建器
 *
 * 参考 MC DefaultSurfaceBuilder，适用于大多数生物群系。
 * 根据噪声值计算地表深度，放置表层和次层方块。
 */
class DefaultSurfaceBuilder : public SurfaceBuilder {
public:
    DefaultSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "default"; }

protected:
    /**
     * @brief 计算地表深度
     * @param noise 噪声值
     * @param random 随机数生成器
     * @return 地表深度（方块数）
     */
    [[nodiscard]] i32 calculateDepth(f64 noise, math::Random& random) const;
};

/**
 * @brief 山地地表构建器
 *
 * 参考 MC MountainSurfaceBuilder，适用于山地生物群系。
 * 根据噪声值委托给DefaultSurfaceBuilder使用不同配置：
 * - noise > 1.0: STONE_STONE_GRAVEL_CONFIG (石头/石头/沙砾)
 * - 否则: GRASS_DIRT_GRAVEL_CONFIG (草方块/泥土/沙砾)
 */
class MountainSurfaceBuilder : public SurfaceBuilder {
public:
    MountainSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "mountain"; }
};

/**
 * @brief 砾石山地地表构建器
 *
 * 参考 MC GravellyMountainSurfaceBuilder。
 * 根据噪声值选择不同配置：
 * - noise > 2.0 或 noise < -1.0: GRAVEL_CONFIG
 * - noise > 1.0: STONE_STONE_GRAVEL_CONFIG
 * - 否则: GRASS_DIRT_GRAVEL_CONFIG
 */
class GravellyMountainSurfaceBuilder : public SurfaceBuilder {
public:
    GravellyMountainSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "gravelly_mountain"; }
};

/**
 * @brief 破碎热带草原地表构建器
 *
 * 参考 MC ShatteredSavannaSurfaceBuilder，适用于破碎热带草原生物群系。
 * 根据噪声值委托给DefaultSurfaceBuilder使用不同配置：
 * - noise > 1.75: STONE_STONE_GRAVEL_CONFIG
 * - noise > -0.5: CORASE_DIRT_DIRT_GRAVEL_CONFIG
 * - 否则: GRASS_DIRT_GRAVEL_CONFIG
 */
class ShatteredSavannaSurfaceBuilder : public SurfaceBuilder {
public:
    ShatteredSavannaSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "shattered_savanna"; }
};

/**
 * @brief 巨型针叶林地表构建器
 *
 * 参考 MC GiantTreeTaigaSurfaceBuilder，适用于巨型针叶林生物群系。
 * 根据噪声值委托给DefaultSurfaceBuilder使用不同配置：
 * - noise > 1.75: COARSE_DIRT_DIRT_GRAVEL_CONFIG
 * - noise > -0.95: PODZOL_DIRT_GRAVEL_CONFIG
 * - 否则: GRASS_DIRT_GRAVEL_CONFIG
 */
class GiantTreeTaigaSurfaceBuilder : public SurfaceBuilder {
public:
    GiantTreeTaigaSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "giant_tree_taiga"; }
};

/**
 * @brief 沼泽地表构建器
 *
 * 参考 MC SwampSurfaceBuilder，适用于沼泽生物群系。
 * 使用Biome.INFO_NOISE在水面附近生成粘土。
 */
class SwampSurfaceBuilder : public SurfaceBuilder {
public:
    SwampSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "swamp"; }

    void setSeed(u64 seed) override;
};

/**
 * @brief 冻洋地表构建器
 *
 * 参考 MC FrozenOceanSurfaceBuilder，适用于冰冻海洋。
 * 生成浮冰冰山和冰层。
 */
class FrozenOceanSurfaceBuilder : public SurfaceBuilder {
public:
    FrozenOceanSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "frozen_ocean"; }

    void setSeed(u64 seed) override;

private:
    // 噪声生成器（需要在setSeed时初始化）
    void* m_noiseA = nullptr;  // PerlinNoiseGenerator
    void* m_noiseB = nullptr;  // PerlinNoiseGenerator
    u64 m_cachedSeed = 0;
};

/**
 * @brief 恶地地表构建器
 *
 * 参考 MC BadlandsSurfaceBuilder，适用于恶地生物群系。
 * 生成彩色陶瓦层，需要基于种子的噪声生成色带。
 */
class BadlandsSurfaceBuilder : public SurfaceBuilder {
public:
    BadlandsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "badlands"; }

    void setSeed(u64 seed) override;

private:
    // 陶瓦色带数组（基于种子生成）
    std::array<const BlockState*, 64> m_terracottaBands{};
    void* m_bandNoise = nullptr;  // PerlinNoiseGenerator
    u64 m_cachedSeed = 0;

    void initBands(u64 seed);
    const BlockState* getTerracottaLayer(i32 worldX, i32 worldY, i32 worldZ);
};

/**
 * @brief 侵蚀恶地地表构建器
 *
 * 参考 MC ErodedBadlandsSurfaceBuilder，继承自BadlandsSurfaceBuilder。
 * 在侵蚀恶地生物群系使用。
 */
class ErodedBadlandsSurfaceBuilder : public SurfaceBuilder {
public:
    ErodedBadlandsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "eroded_badlands"; }
};

/**
 * @brief 疏林恶地地表构建器
 *
 * 参考 MC WoodedBadlandsSurfaceBuilder，继承自BadlandsSurfaceBuilder。
 * 在疏林恶地生物群系使用，地表有草和泥土。
 */
class WoodedBadlandsSurfaceBuilder : public SurfaceBuilder {
public:
    WoodedBadlandsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "wooded_badlands"; }
};

/**
 * @brief 下界地表构建器
 *
 * 参考 MC NetherSurfaceBuilder，适用于下界生物群系。
 */
class NetherSurfaceBuilder : public SurfaceBuilder {
public:
    NetherSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "nether"; }
};

/**
 * @brief 下界森林地表构建器
 *
 * 参考 MC NetherForestsSurfaceBuilder，适用于下界森林生物群系。
 * 使用噪声决定表层方块类型。
 */
class NetherForestsSurfaceBuilder : public SurfaceBuilder {
public:
    NetherForestsSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "nether_forests"; }

    void setSeed(u64 seed) override;

private:
    void* m_noise = nullptr;  // OctavesNoiseGenerator
    u64 m_cachedSeed = 0;
};

/**
 * @brief 灵魂沙峡谷地表构建器
 *
 * 参考 MC SoulSandValleySurfaceBuilder，适用于灵魂沙峡谷生物群系。
 */
class SoulSandValleySurfaceBuilder : public SurfaceBuilder {
public:
    SoulSandValleySurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "soul_sand_valley"; }
};

/**
 * @brief 玄武岩三角洲地表构建器
 *
 * 参考 MC BasaltDeltasSurfaceBuilder，适用于玄武岩三角洲生物群系。
 */
class BasaltDeltasSurfaceBuilder : public SurfaceBuilder {
public:
    BasaltDeltasSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override;

    [[nodiscard]] const char* name() const override { return "basalt_deltas"; }
};

/**
 * @brief 空操作地表构建器
 *
 * 参考 MC NoopSurfaceBuilder，不执行任何操作。
 * 用于不需要地表生成的生物群系。
 */
class NoopSurfaceBuilder : public SurfaceBuilder {
public:
    NoopSurfaceBuilder() = default;

    void buildSurface(
        math::Random& random,
        ChunkPrimer& chunk,
        const Biome& biome,
        i32 x, i32 z,
        i32 startHeight,
        f64 surfaceNoise,
        const BlockState* defaultBlock,
        const BlockState* defaultFluid,
        i32 seaLevel,
        u64 worldSeed,
        const SurfaceBuilderConfig& config) override
    {
        (void)random; (void)chunk; (void)biome; (void)x; (void)z;
        (void)startHeight; (void)surfaceNoise; (void)defaultBlock; (void)defaultFluid;
        (void)seaLevel; (void)worldSeed; (void)config;
        // 空操作
    }

    [[nodiscard]] const char* name() const override { return "noop"; }
};

} // namespace mc
