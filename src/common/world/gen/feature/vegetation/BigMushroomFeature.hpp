#pragma once

#include "../Feature.hpp"
#include "../ConfiguredFeature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 巨型蘑菇特征配置
 *
 * 参考 MC BigMushroomFeatureConfig
 */
struct BigMushroomFeatureConfig : public IFeatureConfig {
    /// 蘑菇盖方块状态
    const BlockState* capState = nullptr;

    /// 蘑菇柄方块状态
    const BlockState* stemState = nullptr;

    /// 蘑菇盖半径
    i32 capRadius = 2;

    BigMushroomFeatureConfig() = default;

    BigMushroomFeatureConfig(
        const BlockState* cap,
        const BlockState* stem,
        i32 radius = 2)
        : capState(cap)
        , stemState(stem)
        , capRadius(radius)
    {}
};

/**
 * @brief 巨型蘑菇特征基类
 *
 * 参考 MC AbstractBigMushroomFeature
 */
class BigMushroomFeature {
public:
    virtual ~BigMushroomFeature() = default;

    /**
     * @brief 放置巨型蘑菇特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 蘑菇配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const BigMushroomFeatureConfig& config);

protected:
    /**
     * @brief 计算给定高度的蘑菇盖半径
     * @param baseRadius 基础半径
     * @param totalHeight 总高度
     * @param capRadius 配置的盖半径
     * @param currentHeight 当前高度
     * @return 当前高度的半径
     */
    [[nodiscard]] virtual i32 getCapRadius(
        i32 baseRadius,
        i32 totalHeight,
        i32 capRadius,
        i32 currentHeight) const = 0;

    /**
     * @brief 生成蘑菇盖
     */
    virtual void generateCap(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) = 0;

    /**
     * @brief 生成蘑菇柄
     */
    void generateStem(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const BigMushroomFeatureConfig& config,
        i32 height);

    /**
     * @brief 计算蘑菇高度
     */
    [[nodiscard]] i32 calculateHeight(math::Random& random) const;

    /**
     * @brief 检查是否可以放置蘑菇
     */
    [[nodiscard]] bool canPlaceAt(
        WorldGenRegion& world,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) const;
};

/**
 * @brief 巨型棕色蘑菇特征
 *
 * 参考 MC BigBrownMushroomFeature
 * 生成平顶的棕色巨型蘑菇
 */
class BigBrownMushroomFeature : public BigMushroomFeature {
public:
    ~BigBrownMushroomFeature() override = default;

protected:
    [[nodiscard]] i32 getCapRadius(
        i32 baseRadius,
        i32 totalHeight,
        i32 capRadius,
        i32 currentHeight) const override;

    void generateCap(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) override;
};

/**
 * @brief 巨型红色蘑菇特征
 *
 * 参考 MC BigRedMushroomFeature
 * 生成圆顶的红色巨型蘑菇
 */
class BigRedMushroomFeature : public BigMushroomFeature {
public:
    ~BigRedMushroomFeature() override = default;

protected:
    [[nodiscard]] i32 getCapRadius(
        i32 baseRadius,
        i32 totalHeight,
        i32 capRadius,
        i32 currentHeight) const override;

    void generateCap(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        i32 height,
        const BigMushroomFeatureConfig& config) override;
};

/**
 * @brief 配置化巨型蘑菇特征
 */
class ConfiguredBigMushroomFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBigMushroomFeature(
        std::unique_ptr<BigMushroomFeatureConfig> config,
        const char* featureName,
        bool isBrown);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }
    [[nodiscard]] const BigMushroomFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<BigMushroomFeatureConfig> m_config;
    std::string m_name;
    std::unique_ptr<BigMushroomFeature> m_feature;
};

/**
 * @brief 预定义巨型蘑菇配置
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct BigMushroomFeatures {
    /// 初始化所有巨型蘑菇特征
    static void initialize();

    /// 获取所有巨型蘑菇特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>>& getAllFeatures();

    /// 获取所有巨型蘑菇特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> getAllFeaturesAndClear();

    /// 创建巨型棕色蘑菇
    static std::unique_ptr<ConfiguredBigMushroomFeature> createBrownMushroom();

    /// 创建巨型红色蘑菇
    static std::unique_ptr<ConfiguredBigMushroomFeature> createRedMushroom();

private:
    static std::vector<std::unique_ptr<ConfiguredBigMushroomFeature>> s_features;
};

} // namespace mc
