#pragma once

#include "../Feature.hpp"
#include "../ConfiguredFeature.hpp"

namespace mc {

/**
 * @brief 海带特征配置
 *
 * 参考 MC BlockStateFeatureConfig
 */
struct KelpFeatureConfig : public IFeatureConfig {
    /// 海带方块状态
    const BlockState* kelpState = nullptr;

    /// 海带顶部方块状态（用于顶端海带）
    const BlockState* kelpTopState = nullptr;

    /// 放置尝试次数
    i32 tries;

    /// 单株最大高度
    i32 maxHeight;

    KelpFeatureConfig() = default;

    explicit KelpFeatureConfig(const BlockState* kelp, const BlockState* kelpTop, i32 t, i32 maxH)
        : kelpState(kelp)
        , kelpTopState(kelpTop)
        , tries(t)
        , maxHeight(maxH)
    {}
};

/**
 * @brief 海带特征
 *
 * 在水下生成海带。
 * 海带可以从海底向上生长到水面。
 *
 * 参考 MC KelpFeature
 */
class KelpFeature {
public:
    /**
     * @brief 放置海带特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海带配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const KelpFeatureConfig& config);

private:
    /**
     * @brief 检查海带是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(
        WorldGenRegion& world,
        const BlockPos& pos) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWater(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化海带特征
 */
class ConfiguredKelpFeature : public ConfiguredFeatureBase {
public:
    ConfiguredKelpFeature(
        std::unique_ptr<KelpFeatureConfig> config,
        const char* featureName);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const KelpFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<KelpFeatureConfig> m_config;
    std::string m_name;
    KelpFeature m_feature;
};

/**
 * @brief 预定义海带配置
 */
struct KelpFeatures {
    /// 初始化所有海带特征
    static void initialize();

    /// 获取所有海带特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredKelpFeature>>& getAllFeatures();

    /// 获取所有海带特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredKelpFeature>> getAllFeaturesAndClear();

    /// 创建冷海带
    static std::unique_ptr<ConfiguredKelpFeature> createColdKelp();

    /// 创建暖海带
    static std::unique_ptr<ConfiguredKelpFeature> createWarmKelp();

private:
    static std::vector<std::unique_ptr<ConfiguredKelpFeature>> s_features;
};

} // namespace mc
