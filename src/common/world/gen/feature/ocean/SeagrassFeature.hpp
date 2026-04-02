#pragma once

#include "../Feature.hpp"
#include "../ConfiguredFeature.hpp"

namespace mc {

/**
 * @brief 海草特征配置
 *
 * 参考 MC BlockStateFeatureConfig
 */
struct SeagrassFeatureConfig : public IFeatureConfig {
    /// 海草方块状态
    const BlockState* seagrassState = nullptr;

    /// 高海草下方方块状态
    const BlockState* tallSeagrassLowerState = nullptr;

    /// 高海草上方方块状态
    const BlockState* tallSeagrassUpperState = nullptr;

    /// 高海草概率 (0.0 - 1.0)
    f32 tallSeagrassChance = 0.0f;

    SeagrassFeatureConfig() = default;

    explicit SeagrassFeatureConfig(const BlockState* seagrass)
        : seagrassState(seagrass)
        , tallSeagrassChance(0.0f)
    {}

    SeagrassFeatureConfig(
        const BlockState* seagrass,
        const BlockState* tallLower,
        const BlockState* tallUpper,
        f32 tallChance = 0.3f)
        : seagrassState(seagrass)
        , tallSeagrassLowerState(tallLower)
        , tallSeagrassUpperState(tallUpper)
        , tallSeagrassChance(tallChance)
    {}
};

/**
 * @brief 海草特征
 *
 * 在水下生成海草。
 * 可以生成普通海草和高海草。
 *
 * 参考 MC SeagrassFeature
 */
class SeagrassFeature {
public:
    /**
     * @brief 放置海草特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海草配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const SeagrassFeatureConfig& config);

private:
    /**
     * @brief 检查海草是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(
        WorldGenRegion& world,
        const BlockPos& pos) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWater(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 放置高海草
     */
    bool placeTallSeagrass(
        WorldGenRegion& world,
        const BlockPos& pos,
        const SeagrassFeatureConfig& config) const;
};

/**
 * @brief 配置化海草特征
 */
class ConfiguredSeagrassFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSeagrassFeature(
        std::unique_ptr<SeagrassFeatureConfig> config,
        const char* featureName);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const SeagrassFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<SeagrassFeatureConfig> m_config;
    std::string m_name;
    SeagrassFeature m_feature;
};

/**
 * @brief 预定义海草配置
 */
struct SeagrassFeatures {
    /// 初始化所有海草特征
    static void initialize();

    /// 获取所有海草特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredSeagrassFeature>>& getAllFeatures();

    /// 获取所有海草特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> getAllFeaturesAndClear();

    /// 创建普通海草（仅普通海草）
    static std::unique_ptr<ConfiguredSeagrassFeature> createSimpleSeagrass();

    /// 创建混合海草（普通+高海草）
    static std::unique_ptr<ConfiguredSeagrassFeature> createMixedSeagrass();

private:
    static std::vector<std::unique_ptr<ConfiguredSeagrassFeature>> s_features;
};

} // namespace mc
