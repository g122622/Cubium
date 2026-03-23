#pragma once

#include "../Feature.hpp"
#include "../ConfiguredFeature.hpp"
#include "trunk/TrunkPlacer.hpp"
#include "foliage/FoliagePlacer.hpp"
#include "../../../block/Block.hpp"
#include "../../placement/Placement.hpp"
#include <memory>

namespace mc {

// 前向声明
class BlockState;

/**
 * @brief 树木特征配置
 *
 * 参考: net.minecraft.world.gen.feature.BaseTreeFeatureConfig
 */
struct TreeFeatureConfig : public IFeatureConfig {
    /// 树干方块状态
    const BlockState* trunkBlock = nullptr;

    /// 树叶方块状态
    const BlockState* foliageBlock = nullptr;

    /// 树干放置器
    std::unique_ptr<TrunkPlacer> trunkPlacer;

    /// 树叶放置器
    std::unique_ptr<FoliagePlacer> foliagePlacer;

    /// 最大水深（树木不能生成在深水中）
    i32 maxWaterDepth = 0;

    /// 是否忽略藤蔓
    bool ignoreVines = false;

    /// 强制放置（跳过高度检查）
    bool forcePlacement = false;

    /// 最小高度
    i32 minHeight = 4;

    TreeFeatureConfig() = default;

    TreeFeatureConfig(
        const BlockState* trunk,
        const BlockState* foliage,
        std::unique_ptr<TrunkPlacer> trunkPlacer_,
        std::unique_ptr<FoliagePlacer> foliagePlacer_
    ) : trunkBlock(trunk)
      , foliageBlock(foliage)
      , trunkPlacer(std::move(trunkPlacer_))
      , foliagePlacer(std::move(foliagePlacer_))
    {}

    /**
     * @brief 复制构造函数（深拷贝）
     */
    TreeFeatureConfig(const TreeFeatureConfig& other)
        : trunkBlock(other.trunkBlock)
        , foliageBlock(other.foliageBlock)
        , maxWaterDepth(other.maxWaterDepth)
        , ignoreVines(other.ignoreVines)
        , forcePlacement(other.forcePlacement)
        , minHeight(other.minHeight)
    {
        // 深拷贝放置器
        if (other.trunkPlacer) {
            trunkPlacer = other.trunkPlacer->clone();
        }
        if (other.foliagePlacer) {
            foliagePlacer = other.foliagePlacer->clone();
        }
    }

    /**
     * @brief 赋值运算符（深拷贝）
     */
    TreeFeatureConfig& operator=(const TreeFeatureConfig& other) {
        if (this != &other) {
            trunkBlock = other.trunkBlock;
            foliageBlock = other.foliageBlock;
            maxWaterDepth = other.maxWaterDepth;
            ignoreVines = other.ignoreVines;
            forcePlacement = other.forcePlacement;
            minHeight = other.minHeight;
            if (other.trunkPlacer) {
                trunkPlacer = other.trunkPlacer->clone();
            }
            if (other.foliagePlacer) {
                foliagePlacer = other.foliagePlacer->clone();
            }
        }
        return *this;
    }
};

/**
 * @brief 树木特征
 *
 * 生成树木的主要特征类。
 *
 * 参考: net.minecraft.world.gen.feature.TreeFeature
 */
class TreeFeature {
public:
    /**
     * @brief 默认构造函数
     */
    TreeFeature() = default;

    /**
     * @brief 放置树木
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param startPos 起始位置
     * @param config 树木配置
     * @return 是否成功放置
     */
    bool place(
        WorldGenRegion& world,
        math::Random& random,
        const BlockPos& startPos,
        const TreeFeatureConfig& config
    );

    /**
     * @brief 检查位置是否可以放置树干
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否可以放置
     */
    [[nodiscard]] static bool isReplaceableAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是空气或树叶
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是空气或树叶
     */
    [[nodiscard]] static bool isAirOrLeavesAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是泥土或耕地
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是泥土或耕地
     */
    [[nodiscard]] static bool isDirtOrFarmlandAt(WorldGenRegion& world, const BlockPos& pos);

    /**
     * @brief 检查位置是否是水
     *
     * @param world 世界区域
     * @param pos 位置
     * @return 是否是水
     */
    [[nodiscard]] static bool isWaterAt(WorldGenRegion& world, const BlockPos& pos);

private:
    /**
     * @brief 计算可用的树干高度
     *
     * 从起始位置向上检查，找到可以放置树干的最大高度。
     *
     * @param world 世界区域
     * @param maxHeight 最大高度
     * @param startPos 起始位置
     * @param config 树木配置
     * @return 可用高度
     */
    [[nodiscard]] i32 calculateAvailableHeight(
        WorldGenRegion& world,
        i32 maxHeight,
        const BlockPos& startPos,
        const TreeFeatureConfig& config
    ) const;

    /**
     * @brief 设置树叶距离属性
     *
     * 遍历树叶方块，设置其到最近树干的距离。
     * 这个距离用于树叶腐烂机制。
     *
     * @param world 世界区域
     * @param trunkBlocks 树干方块集合
     * @param foliageBlocks 树叶方块集合
     */
    void setFoliageDistance(
        WorldGenRegion& world,
        const std::set<BlockPos>& trunkBlocks,
        const std::set<BlockPos>& foliageBlocks
    );
};

/**
 * @brief 配置化的树木特征
 *
 * 组合树木特征、配置和放置规则。
 * 继承 ConfiguredFeatureBase 以支持统一的特征注册。
 */
class ConfiguredTreeFeature : public ConfiguredFeatureBase {
public:
    /**
     * @brief 构造配置化树木特征
     * @param featureConfig 树木配置
     * @param placement 放置规则
     * @param featureName 特征名称
     */
    ConfiguredTreeFeature(
        std::unique_ptr<TreeFeatureConfig> featureConfig,
        std::unique_ptr<ConfiguredPlacement> placement,
        const char* featureName = "tree");

    /**
     * @brief 在指定位置放置树木（实现 ConfiguredFeatureBase 接口）
     */
    bool place(
        WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override;

    /**
     * @brief 获取特征名称
     */
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }

    /**
     * @brief 获取装饰阶段
     */
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

    /**
     * @brief 获取树木配置
     */
    [[nodiscard]] const TreeFeatureConfig& getConfig() const { return *m_config; }

private:
    std::unique_ptr<TreeFeatureConfig> m_config;
    std::unique_ptr<ConfiguredPlacement> m_placement;
    std::string m_name;
    TreeFeature m_feature;
};

/**
 * @brief 预定义的树木配置
 *
 * 管理所有预配置的树木特征。
 * 参考 MC Features / ConfiguredFeatures
 */
struct TreeFeatures {
    /**
     * @brief 初始化所有树木特征
     */
    static void initialize();

    /**
     * @brief 获取所有树木特征并转移所有权
     * @note 调用后内部存储被清空
     */
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredTreeFeature>> getAllFeaturesAndClear();

    /**
     * @brief 获取所有树木特征（只读访问）
     */
    [[nodiscard]] static const std::vector<std::unique_ptr<ConfiguredTreeFeature>>& getAllFeatures();

    /// 创建橡树配置
    static std::unique_ptr<ConfiguredTreeFeature> createOakTree();

    /// 创建白桦配置
    static std::unique_ptr<ConfiguredTreeFeature> createBirchTree();

    /// 创建云杉配置
    static std::unique_ptr<ConfiguredTreeFeature> createSpruceTree();

    /// 创建丛林木配置
    static std::unique_ptr<ConfiguredTreeFeature> createJungleTree();

    /// 创建金合欢配置
    static std::unique_ptr<ConfiguredTreeFeature> createAcaciaTree();

    /// 创建深色橡树配置
    static std::unique_ptr<ConfiguredTreeFeature> createDarkOakTree();

    /// 创建稀疏橡树（平原用）
    static std::unique_ptr<ConfiguredTreeFeature> createSparseOakTree();

    /// 创建巨型云杉配置
    static std::unique_ptr<ConfiguredTreeFeature> createGiantSpruceTree();

    /// 创建巨型丛林木配置
    static std::unique_ptr<ConfiguredTreeFeature> createGiantJungleTree();

private:
    static std::vector<std::unique_ptr<ConfiguredTreeFeature>> s_features;

    /// 创建树木特征的基础配置
    static TreeFeatureConfig oakConfig();
    static TreeFeatureConfig birchConfig();
    static TreeFeatureConfig spruceConfig();
    static TreeFeatureConfig jungleConfig();
    static TreeFeatureConfig acaciaConfig();
    static TreeFeatureConfig darkOakConfig();
    static TreeFeatureConfig giantSpruceConfig();
    static TreeFeatureConfig giantJungleConfig();
};

} // namespace mc
