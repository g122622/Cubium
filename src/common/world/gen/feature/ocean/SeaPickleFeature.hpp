#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"

namespace mc {

/**
 * @brief 海泡菜特征配置
 *
 * 参考 MC SeaPickleFeatureConfig
 */
struct SeaPickleFeatureConfig : public IFeatureConfig {
    /// 海泡菜方块状态
    const BlockState* seaPickleState = nullptr;

    /// 尝试放置次数
    i32 tries = 10;

    /// 最大数量 (1-4)
    i32 maxCount = 4;

    SeaPickleFeatureConfig() = default;

    explicit SeaPickleFeatureConfig(const BlockState* state, i32 t = 10, i32 maxC = 4)
        : seaPickleState(state)
        , tries(t)
        , maxCount(maxC)
    {}
};

/**
 * @brief 海泡菜特征
 *
 * 在水下生成海泡菜，通常在暖水海洋。
 * 海泡菜可以堆叠1-4个。
 *
 * 参考 MC SeaPickleFeature
 */
class SeaPickleFeature {
public:
    /**
     * @brief 放置海泡菜特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 海泡菜配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const SeaPickleFeatureConfig& config);

private:
    /**
     * @brief 检查海泡菜是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const BlockState& pickleState) const;

    /**
     * @brief 检查位置是否为水
     */
    [[nodiscard]] bool isWater(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化海泡菜特征
 */
class ConfiguredSeaPickleFeature : public ConfiguredFeatureBase {
public:
    ConfiguredSeaPickleFeature(std::unique_ptr<SeaPickleFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const SeaPickleFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<SeaPickleFeatureConfig> m_config;
    std::string m_name;
    SeaPickleFeature m_feature;
};

/**
 * @brief 预定义海泡菜配置
 */
struct SeaPickleFeatures {
    /// 初始化所有海泡菜特征
    static void initialize();

    /// 获取所有海泡菜特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>>& getAllFeatures();

    /// 获取所有海泡菜特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>> getAllFeaturesAndClear();

    /// 创建普通海泡菜
    static std::unique_ptr<ConfiguredSeaPickleFeature> createNormalSeaPickle();

private:
    static std::vector<std::unique_ptr<ConfiguredSeaPickleFeature>> s_features;
};

} // namespace mc
