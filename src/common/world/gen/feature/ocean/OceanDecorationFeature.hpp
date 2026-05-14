#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"

namespace mc {

/**
 * @brief 海洋装饰特征配置
 *
 * 该配置用于补齐海洋可见装饰闭环：
 * - 潮涌核心
 * - 干海带块
 * - 海龟蛋
 * - 气泡柱
 * - 海晶石楼梯与台阶
 */
struct OceanDecorationFeatureConfig : public IFeatureConfig {
    const BlockState* conduitState = nullptr;
    const BlockState* driedKelpBlockState = nullptr;
    const BlockState* turtleEggState = nullptr;
    const BlockState* bubbleColumnState = nullptr;
    const BlockState* prismarineStairsState = nullptr;
    const BlockState* prismarineSlabState = nullptr;
    const BlockState* prismarineState = nullptr;
    const BlockState* magmaState = nullptr;
    const BlockState* sandState = nullptr;

    i32 tries = 2;
    i32 bubbleColumnMaxHeight = 8;
    i32 driedKelpCount = 4;
};

/**
 * @brief 海洋装饰特征
 */
class OceanDecorationFeature {
public:
    bool place(
        WorldGenRegion& world, math::Random& random, const BlockPos& pos, const OceanDecorationFeatureConfig& config);

private:
    [[nodiscard]] bool isWater(WorldGenRegion& world, const BlockPos& pos) const;

    [[nodiscard]] bool hasSolidSupport(WorldGenRegion& world, const BlockPos& pos) const;

    [[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z) const;

    bool placeSingleDecoration(WorldGenRegion& world,
        math::Random& random,
        const BlockPos& centerPos,
        const OceanDecorationFeatureConfig& config);
};

/**
 * @brief 配置化海洋装饰特征
 */
class ConfiguredOceanDecorationFeature : public ConfiguredFeatureBase {
public:
    ConfiguredOceanDecorationFeature(std::unique_ptr<OceanDecorationFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const OceanDecorationFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<OceanDecorationFeatureConfig> m_config;
    std::string m_name;
    OceanDecorationFeature m_feature;
};

/**
 * @brief 预定义海洋装饰特征
 */
struct OceanDecorationFeatures {
    static void initialize();

    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>>& getAllFeatures();

    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>> getAllFeaturesAndClear();

    static std::unique_ptr<ConfiguredOceanDecorationFeature> createOceanProps();

private:
    static std::vector<std::unique_ptr<ConfiguredOceanDecorationFeature>> s_features;
};

} // namespace mc
