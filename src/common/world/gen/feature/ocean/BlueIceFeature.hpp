#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"

namespace mc {

/**
 * @brief 蓝冰特征配置
 *
 * 参考 MC 1.16.5 BlueIceFeature 的冷海域蓝冰生成逻辑。
 */
struct BlueIceFeatureConfig : public IFeatureConfig {
    const BlockState* blueIceState = nullptr;
    const BlockState* packedIceState = nullptr;

    /// 传播迭代次数（原版为 200）
    i32 spreadAttempts = 200;
};

/**
 * @brief 蓝冰特征
 */
class BlueIceFeature {
public:
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const BlueIceFeatureConfig& config,
        i32 seaLevel);

private:
    [[nodiscard]] bool isWater(WorldGenRegion& world, const BlockPos& pos) const;

    [[nodiscard]] bool isReplaceableForSpread(
        WorldGenRegion& world,
        const BlockPos& pos,
        const BlueIceFeatureConfig& config) const;

    [[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const;
};

/**
 * @brief 配置化蓝冰特征
 */
class ConfiguredBlueIceFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBlueIceFeature(
        std::unique_ptr<BlueIceFeatureConfig> config,
        const char* featureName);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const BlueIceFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BlueIceFeatureConfig> m_config;
    std::string m_name;
    BlueIceFeature m_feature;
};

/**
 * @brief 预定义蓝冰特征
 */
struct BlueIceFeatures {
    static void initialize();

    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredBlueIceFeature>>& getAllFeatures();

    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredBlueIceFeature>> getAllFeaturesAndClear();

    static std::unique_ptr<ConfiguredBlueIceFeature> createBlueIce();

private:
    static std::vector<std::unique_ptr<ConfiguredBlueIceFeature>> s_features;
};

} // namespace mc
