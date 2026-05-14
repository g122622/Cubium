#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 草丛特征配置
 *
 * 参考 MC BlockClusterFeatureConfig
 * 用于配置草、蕨类等植被的生成参数。
 */
struct GrassFeatureConfig : public IFeatureConfig {
    /// 可放置的方块状态列表（随机选择）
    std::vector<const BlockState*> states;

    /// 尝试放置次数
    i32 tries = 64;

    /// X方向扩散范围
    i32 xSpread = 7;

    /// Y方向扩散范围
    i32 ySpread = 3;

    /// Z方向扩散范围
    i32 zSpread = 7;

    /// 是否可以替换现有方块
    bool canReplace = false;

    /// 是否需要水
    bool requiresWater = false;

    /// 是否投影到地面（从高度图获取Y坐标）
    bool project = true;

    GrassFeatureConfig() = default;

    /**
     * @brief 添加方块状态
     */
    void addState(const BlockState* state) { states.push_back(state); }

    /**
     * @brief 获取随机方块状态
     */
    [[nodiscard]] const BlockState* getRandomState(math::Random& random) const;
};

/**
 * @brief 草丛特征
 *
 * 在指定位置周围随机放置草、蕨类等植被。
 * 参考 MC RandomPatchFeature
 */
class GrassFeature {
public:
    /**
     * @brief 放置草丛特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 草丛配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const GrassFeatureConfig& config);

private:
    /**
     * @brief 检查草丛是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos, const GrassFeatureConfig& config) const;

    /**
     * @brief 检查下方方块是否支持草丛生长
     */
    [[nodiscard]] bool isValidGround(WorldGenRegion& world, const BlockPos& pos) const;
};

/**
 * @brief 配置化草丛特征
 */
class ConfiguredGrassFeature : public ConfiguredFeatureBase {
public:
    ConfiguredGrassFeature(std::unique_ptr<GrassFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const GrassFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<GrassFeatureConfig> m_config;
    std::string m_name;
    GrassFeature m_feature;
};

/**
 * @brief 预定义草丛配置
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct GrassFeatures {
    /// 初始化所有草丛特征
    static void initialize();

    /// 获取所有草丛特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredGrassFeature>>& getAllFeatures();

    /// 获取所有草丛特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredGrassFeature>> getAllFeaturesAndClear();

    /// 创建平原草丛（高草、蕨类）
    static std::unique_ptr<ConfiguredGrassFeature> createPlainsGrass();

    /// 创建森林草丛（高草、蕨类、少量花）
    static std::unique_ptr<ConfiguredGrassFeature> createForestGrass();

    /// 创建丛林草丛（高草、蕨类）
    static std::unique_ptr<ConfiguredGrassFeature> createJungleGrass();

    /// 创建沼泽草丛
    static std::unique_ptr<ConfiguredGrassFeature> createSwampGrass();

    /// 创建稀树草原草丛（高草）
    static std::unique_ptr<ConfiguredGrassFeature> createSavannaGrass();

    /// 创建针叶林草丛（蕨类）
    static std::unique_ptr<ConfiguredGrassFeature> createTaigaGrass();

    /// 创建恶地枯萎灌木
    static std::unique_ptr<ConfiguredGrassFeature> createBadlandsDeadBush();

private:
    static std::vector<std::unique_ptr<ConfiguredGrassFeature>> s_features;
};

} // namespace mc
