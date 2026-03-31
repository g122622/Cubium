#pragma once

#include "../Feature.hpp"
#include "../ConfiguredFeature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 黑曜石柱状态
 *
 * 存储单根黑曜石柱的生成状态
 */
struct EndSpike {
    i32 centerX;        ///< 中心X坐标
    i32 centerZ;        ///< 中心Z坐标
    i32 radius;         ///< 半径（2-5）
    i32 height;         ///< 高度（76-103）
    bool guarded;       ///< 是否有铁栏杆笼子

    EndSpike(i32 x, i32 z, i32 r, i32 h, bool g)
        : centerX(x), centerZ(z), radius(r), height(h), guarded(g) {}
};

/**
 * @brief 黑曜石柱特征配置
 *
 * 参考 MC EndSpikeFeatureConfig
 */
struct EndSpikeFeatureConfig : public IFeatureConfig {
    /// 黑曜石柱列表（如果为空则自动生成）
    std::vector<EndSpike> spikes;

    /// 是否在生成后摧毁柱子（用于末影龙战斗）
    bool destroying = false;

    EndSpikeFeatureConfig() = default;

    explicit EndSpikeFeatureConfig(const std::vector<EndSpike>& spikeList, bool destroy = false)
        : spikes(spikeList)
        , destroying(destroy)
    {}

    /**
     * @brief 生成默认的黑曜石柱配置（10根柱子）
     * @param worldSeed 世界种子
     * @return 黑曜石柱列表
     */
    static std::vector<EndSpike> generateSpikes(u64 worldSeed);
};

/**
 * @brief 黑曜石柱特征
 *
 * 在末地生成黑曜石柱（末影龙战斗区域）。
 * 参考 MC EndSpikeFeature / SpikeFeature
 *
 * 特点：
 * - 10根黑曜石柱围绕末地中心（0,0）
 * - 高度范围：76-103
 * - 半径范围：2-5
 * - 部分柱子顶部有铁栏杆笼子保护
 */
class EndSpikeFeature {
public:
    /**
     * @brief 放置黑曜石柱特征
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置（末地原点）
     * @param config 黑曜石柱配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& pos,
        const EndSpikeFeatureConfig& config);

private:
    /**
     * @brief 检查柱子是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(
        WorldGenRegion& world,
        const BlockPos& pos) const;

    /**
     * @brief 生成单根黑曜石柱
     */
    void generateSpike(
        WorldGenRegion& world,
        math::Random& random,
        const EndSpike& spike);

    /**
     * @brief 生成铁栏杆笼子
     */
    void generateCage(
        WorldGenRegion& world,
        const BlockPos& topPos,
        i32 radius);
};

/**
 * @brief 配置化黑曜石柱特征
 */
class ConfiguredEndSpikeFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndSpikeFeature(
        std::unique_ptr<EndSpikeFeatureConfig> config,
        const char* featureName);

    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }
    [[nodiscard]] const EndSpikeFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<EndSpikeFeatureConfig> m_config;
    std::string m_name;
    EndSpikeFeature m_feature;
};

/**
 * @brief 预定义黑曜石柱特征
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct EndSpikeFeatures {
    /// 初始化所有黑曜石柱特征
    static void initialize();

    /// 获取所有黑曜石柱特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>>& getAllFeatures();

    /// 获取所有黑曜石柱特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> getAllFeaturesAndClear();

    /// 创建标准黑曜石柱配置
    static std::unique_ptr<ConfiguredEndSpikeFeature> createStandard(u64 worldSeed);

private:
    static std::vector<std::unique_ptr<ConfiguredEndSpikeFeature>> s_features;
};

} // namespace mc
