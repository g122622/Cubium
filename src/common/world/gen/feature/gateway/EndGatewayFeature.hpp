#pragma once

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include <vector>

namespace mc {

/**
 * @brief 末地折跃门配置
 *
 * 参考 MC EndGatewayFeatureConfig
 */
struct EndGatewayFeatureConfig : public IFeatureConfig {
    /// 是否为退出折跃门（在玩家进入时生成）
    bool isExit = false;

    /// 传送到外岛的精确位置（如果为空则使用默认位置）
    std::optional<BlockPos> exactPosition;

    EndGatewayFeatureConfig() = default;

    explicit EndGatewayFeatureConfig(bool exit, const std::optional<BlockPos>& pos = {})
        : isExit(exit)
        , exactPosition(pos)
    {}
};

/**
 * @brief 末地折跃门特征
 *
 * 在末地生成末地折跃门，用于在主岛和外岛之间传送。
 * 参考 MC EndGatewayFeature / EndGatewayBlock
 *
 * 特点：
 * - 末影龙死亡后生成（最多20个）
 * - 由基岩、末地折跃门方块组成
 * - 传送到1024格外的外岛
 * - 折跃门方块有紫色光柱效果
 */
class EndGatewayFeature {
public:
    /**
     * @brief 放置末地折跃门
     * @param world 世界区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 折跃门配置
     * @return 是否成功放置
     */
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& pos, const EndGatewayFeatureConfig& config);

    /**
     * @brief 计算折跃门的传送目标
     * @param currentPos 当前折跃门位置
     * @param seed 世界种子
     * @return 传送目标位置
     */
    static BlockPos calculateTeleportTarget(const BlockPos& currentPos, u64 seed);

private:
    /**
     * @brief 检查折跃门是否可以放置在指定位置
     */
    [[nodiscard]] bool canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const;

    /**
     * @brief 生成折跃门结构
     */
    void generateGateway(WorldGenRegion& world, math::Random& random, const BlockPos& pos);
};

/**
 * @brief 配置化末地折跃门特征
 */
class ConfiguredEndGatewayFeature : public ConfiguredFeatureBase {
public:
    ConfiguredEndGatewayFeature(std::unique_ptr<EndGatewayFeatureConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::SurfaceStructures; }
    [[nodiscard]] const EndGatewayFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<EndGatewayFeatureConfig> m_config;
    std::string m_name;
    EndGatewayFeature m_feature;
};

/**
 * @brief 预定义末地折跃门特征
 *
 * 注意：调用 getAllFeaturesAndClear() 后，所有权转移给调用者。
 */
struct EndGatewayFeatures {
    /// 初始化所有末地折跃门特征
    static void initialize();

    /// 获取所有末地折跃门特征
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>>& getAllFeatures();

    /// 获取所有末地折跃门特征并清空（转移所有权）
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>> getAllFeaturesAndClear();

    /// 创建标准末地折跃门
    static std::unique_ptr<ConfiguredEndGatewayFeature> createGateway();

    /// 创建退出折跃门
    static std::unique_ptr<ConfiguredEndGatewayFeature> createExitGateway();

private:
    static std::vector<std::unique_ptr<ConfiguredEndGatewayFeature>> s_features;
};

} // namespace mc
