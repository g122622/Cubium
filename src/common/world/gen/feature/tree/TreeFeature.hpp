/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../../block/Block.hpp"
#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "../state/BlockStateProvider.hpp"
#include "featuresize/FeatureSize.hpp"
#include "foliage/FoliagePlacer.hpp"
#include "trunk/TrunkPlacer.hpp"
#include <memory>

namespace mc {

// 前向声明
class BlockState;

/**
 * @brief 树木特征配置
 *
 * 参考: net.minecraft.world.gen.feature.BaseTreeFeatureConfig
 *
 * trunkBlock 为树干单一状态（trunk_provider 解析期降级，trunk 仅支持 simple）。
 *
 * 树叶采用双轨：
 * - foliageBlock 为单一树叶状态，用于放置器第一遍逐层放置与坐标收集；
 * - foliageProvider 为多态树叶状态提供者（多态基类，支持 weighted/rule_based 等），
 *   非空时第二遍对每个叶片独立采样，用于杜鹃树等混合多种叶子的场景。
 * 单一树叶树（橡/桦/云杉...）仅设置 foliageBlock，foliageProvider 为空。
 */
struct TreeFeatureConfig : public IFeatureConfig {
    /// 树干方块状态（trunk_provider 解析期降级的单一状态，trunk 仅支持 simple）
    const BlockState* trunkBlock = nullptr;

    /// 树叶方块状态（单一，放置器第一遍使用；foliageProvider 为空时即唯一树叶来源）
    const BlockState* foliageBlock = nullptr;

    /// 树叶方块状态提供者（多态，非空时第二遍按叶片独立采样，优先于 foliageBlock）
    std::unique_ptr<world::gen::feature::state::BlockStateProvider> foliageProvider;

    /// 树干放置器
    std::unique_ptr<TrunkPlacer> trunkPlacer;

    /// 树叶放置器
    std::unique_ptr<FoliagePlacer> foliagePlacer;

    /// 最小尺寸约束（对应 MC 1.21.11 BaseTreeFeatureConfig.minimumSize）
    /// 用于 getMaxFreeTreeHeight 阶段确定每层 y 的水平检查半径。
    /// 若为空，TreeFeature 会使用默认的"底部细、顶部粗"退化检查逻辑。
    std::unique_ptr<FeatureSize> minimumSize;

    /// 最大水深（树木不能生成在深水中）
    i32 maxWaterDepth = 0;

    /// 是否忽略藤蔓
    bool ignoreVines = false;

    /// 强制放置（跳过高度检查）
    bool forcePlacement = false;

    /// 最小高度
    i32 minHeight = 4;

    TreeFeatureConfig() = default;

    TreeFeatureConfig(const BlockState* trunk,
        const BlockState* foliage,
        std::unique_ptr<TrunkPlacer> trunkPlacer_,
        std::unique_ptr<FoliagePlacer> foliagePlacer_)
        : trunkBlock(trunk)
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
        // 深拷贝树叶状态提供者
        if (other.foliageProvider) {
            foliageProvider = other.foliageProvider->clone();
        }
        // 深拷贝最小尺寸约束
        if (other.minimumSize) {
            minimumSize = other.minimumSize->clone();
        }
    }

    /**
     * @brief 赋值运算符（深拷贝）
     */
    TreeFeatureConfig& operator=(const TreeFeatureConfig& other)
    {
        if (this != &other) {
            trunkBlock = other.trunkBlock;
            foliageBlock = other.foliageBlock;
            maxWaterDepth = other.maxWaterDepth;
            ignoreVines = other.ignoreVines;
            forcePlacement = other.forcePlacement;
            minHeight = other.minHeight;
            if (other.trunkPlacer) {
                trunkPlacer = other.trunkPlacer->clone();
            } else {
                trunkPlacer.reset();
            }
            if (other.foliagePlacer) {
                foliagePlacer = other.foliagePlacer->clone();
            } else {
                foliagePlacer.reset();
            }
            if (other.foliageProvider) {
                foliageProvider = other.foliageProvider->clone();
            } else {
                foliageProvider.reset();
            }
            if (other.minimumSize) {
                minimumSize = other.minimumSize->clone();
            } else {
                minimumSize.reset();
            }
        }
        return *this;
    }

    /**
     * @brief 移动构造函数
     */
    TreeFeatureConfig(TreeFeatureConfig&& other) noexcept
        : trunkBlock(other.trunkBlock)
        , foliageBlock(other.foliageBlock)
        , foliageProvider(std::move(other.foliageProvider))
        , trunkPlacer(std::move(other.trunkPlacer))
        , foliagePlacer(std::move(other.foliagePlacer))
        , minimumSize(std::move(other.minimumSize))
        , maxWaterDepth(other.maxWaterDepth)
        , ignoreVines(other.ignoreVines)
        , forcePlacement(other.forcePlacement)
        , minHeight(other.minHeight)
    {}

    /**
     * @brief 移动赋值运算符
     */
    TreeFeatureConfig& operator=(TreeFeatureConfig&& other) noexcept
    {
        if (this != &other) {
            trunkBlock = other.trunkBlock;
            foliageBlock = other.foliageBlock;
            foliageProvider = std::move(other.foliageProvider);
            trunkPlacer = std::move(other.trunkPlacer);
            foliagePlacer = std::move(other.foliagePlacer);
            minimumSize = std::move(other.minimumSize);
            maxWaterDepth = other.maxWaterDepth;
            ignoreVines = other.ignoreVines;
            forcePlacement = other.forcePlacement;
            minHeight = other.minHeight;
        }
        return *this;
    }

    /**
     * @brief 是否使用树叶状态提供者
     * @return 若 foliageProvider 非空，返回 true
     */
    [[nodiscard]] bool hasFoliageProvider() const noexcept { return foliageProvider != nullptr; }

    /**
     * @brief 获取单个树叶方块状态
     *
     * 当 foliageProvider 存在时，每次调用独立采样（用于支持加权混合叶子）。
     * 否则返回 foliageBlock。
     *
     * @param world 世界区域
     * @param random 随机数生成器
     * @param x 采样坐标 X
     * @param y 采样坐标 Y
     * @param z 采样坐标 Z
     * @return 树叶方块状态（可能为 nullptr，调用方需自行检查）
     */
    [[nodiscard]] const BlockState* getFoliageState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const
    {
        if (hasFoliageProvider()) {
            return foliageProvider->getState(world, random, x, y, z);
        }
        return foliageBlock;
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
    bool place(WorldGenRegion& world, math::Random& random, const BlockPos& startPos, const TreeFeatureConfig& config);

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
    [[nodiscard]] i32 _calculateAvailableHeight(
        WorldGenRegion& world, i32 maxHeight, const BlockPos& startPos, const TreeFeatureConfig& config) const;

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
    void _setFoliageDistance(
        WorldGenRegion& world, const std::set<BlockPos>& trunkBlocks, const std::set<BlockPos>& foliageBlocks);
};

/**
 * @brief 配置化的树木特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置树木。
 * 继承 ConfiguredFeatureBase 以支持统一的特征注册。
 */
class ConfiguredTreeFeature : public ConfiguredFeatureBase {
public:
    /**
     * @brief 构造配置化树木特征
     * @param featureConfig 树木配置
     * @param featureName 特征名称
     */
    ConfiguredTreeFeature(std::unique_ptr<TreeFeatureConfig> featureConfig, const char* featureName = "tree");

    /**
     * @brief 在指定位置放置树木（实现 ConfiguredFeatureBase 接口）
     */
    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

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
    std::string m_name;
    // TreeFeature::place() 算法重载非 const（工具类无状态），但 ConfiguredTreeFeature::place() 语义不变
    // feature 对象本身在放置时不可变。标记 mutable 使 const override 可调用算法。
    mutable TreeFeature m_feature;
};

/**
 * @brief 预定义的树木配置
 *
 * 提供各树种的纯配置工厂方法，供 sapling→tree 生长与 JSON 特征类型解析器复用。
 */
struct TreeFeatures {
    /// 创建树木特征的基础配置（公开供 TreeGenerators 使用）
    static TreeFeatureConfig oakConfig();
    static TreeFeatureConfig birchConfig();
    static TreeFeatureConfig spruceConfig();
    static TreeFeatureConfig jungleConfig();
    static TreeFeatureConfig acaciaConfig();
    static TreeFeatureConfig darkOakConfig();
    static TreeFeatureConfig giantSpruceConfig();
    static TreeFeatureConfig giantJungleConfig();
    static TreeFeatureConfig fancyOakConfig();
    static TreeFeatureConfig pineConfig();
    static TreeFeatureConfig jungleBushConfig();
    static TreeFeatureConfig swampConfig();
    static TreeFeatureConfig megaPineConfig();
    static TreeFeatureConfig tallBirchConfig();
    static TreeFeatureConfig cherryConfig();
    static TreeFeatureConfig paleOakConfig();
    static TreeFeatureConfig azaleaConfig();
};

} // namespace mc
