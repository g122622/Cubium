#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 花卉特征配置
 *
 * 参考 MC DefaultFlowerFeature / FlowerFeatureConfig
 */
struct FlowerFeatureConfig : public IFeatureConfig {
    /// 可放置的花卉方块状态列表（随机选择）
    std::vector<const BlockState*> flowers;

    /// 花卉尝试放置次数
    i32 tries = 64;

    /// 每次尝试的 XZ 范围
    i32 xzSpread = 7;

    /// 是否需要特定方块才能放置
    bool requiresWater = false;

    FlowerFeatureConfig() = default;

    explicit FlowerFeatureConfig(const BlockState* flower)
        : flowers{flower}
    {}

    FlowerFeatureConfig(std::vector<const BlockState*> flowerList, i32 attemptCount)
        : flowers(std::move(flowerList))
        , tries(attemptCount)
    {}

    /**
     * @brief 添加花卉
     */
    void addFlower(const BlockState* flower) { flowers.push_back(flower); }

    /**
     * @brief 获取随机花卉
     */
    [[nodiscard]] const BlockState* getRandomFlower(math::Random& random) const;
};

/**
 * @brief 花卉特征
 *
 * 在指定位置周围随机放置花卉。
 * 参考 MC DefaultFlowerFeature
 */
class FlowerFeature {
public:
    /**
     * @brief 放置花卉特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 花卉配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const FlowerFeatureConfig& config);

private:
    /**
     * @brief 检查花卉是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const FlowerFeatureConfig& config) const;

    /**
     * @brief 检查下方方块是否支持花卉生长
     */
    [[nodiscard]] bool isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化花卉特征
 */
class ConfiguredFlowerFeature : public ConfiguredFeatureBase {
public:
    ConfiguredFlowerFeature(std::unique_ptr<FlowerFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const FlowerFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<FlowerFeatureConfig> m_config;
    std::string m_name;
    FlowerFeature m_feature;
};

/**
 * @brief 预定义花卉配置
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct FlowerFeatures {
    /// 初始化所有花卉特征
    static void initialize();

    /// 获取所有花卉特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredFlowerFeature>>& getAllFeatures();

    /// 获取所有花卉特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFlowerFeature>> getAllFeaturesAndClear();

    /// 创建平原花卉（蒲公英、虞美人等）
    static std::unique_ptr<ConfiguredFlowerFeature> createPlainsFlowers();

    /// 创建森林花卉（蒲公英、虞美人、铃兰、茜草花）
    static std::unique_ptr<ConfiguredFlowerFeature> createForestFlowers();

    /// 创建繁花森林花卉（更多种类）
    static std::unique_ptr<ConfiguredFlowerFeature> createFlowerForestFlowers();

    /// 创建沼泽花卉（兰花）
    static std::unique_ptr<ConfiguredFlowerFeature> createSwampFlowers();

    /// 创建向日葵
    static std::unique_ptr<ConfiguredFlowerFeature> createSunflower();

private:
    static std::vector<std::unique_ptr<ConfiguredFlowerFeature>> s_features;
};

} // namespace mc
