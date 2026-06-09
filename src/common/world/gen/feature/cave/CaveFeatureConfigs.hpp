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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/cave/CaveSurface.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/WeightedBlockStateProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::feature::cave {

// 前向声明
class CaveFeatures;

/**
 * @brief 植被贴片配置
 *
 * 定义洞穴地面/天花板植被贴片的生成参数。
 * 用于苔藓贴片和黏土贴片的生成。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.VegetationPatchConfiguration
 */
struct VegetationPatchConfig {
    /// 可被替换的方块标签名称
    std::string replaceableTag;

    /// 地面方块状态提供者
    const BlockState* groundState = nullptr;

    /// 植被特征ID（在FeatureRegistry中注册的ID）
    u32 vegetationFeatureId = 0;

    /// 贴片表面方向
    CaveSurface surface = CaveSurface::Floor;

    /// 地面深度（层数）
    std::unique_ptr<valueprovider::IntProvider> depth;

    /// 底部额外方块概率
    f32 extraBottomBlockChance = 0.0f;

    /// 垂直搜索范围
    i32 verticalRange = 5;

    /// 植被放置概率
    f32 vegetationChance = 0.8f;

    /// XZ半径范围
    std::unique_ptr<valueprovider::IntProvider> xzRadius;

    /// 边缘额外列概率
    f32 extraEdgeColumnChance = 0.3f;

    VegetationPatchConfig() = default;

    /**
     * @brief 构造地面贴片配置
     */
    static VegetationPatchConfig floorPatch(const std::string& replaceableTag,
        const BlockState* groundState,
        u32 vegetationFeatureId,
        std::unique_ptr<valueprovider::IntProvider> depth,
        f32 extraBottomBlockChance,
        i32 verticalRange,
        f32 vegetationChance,
        std::unique_ptr<valueprovider::IntProvider> xzRadius,
        f32 extraEdgeColumnChance);

    /**
     * @brief 构造天花板贴片配置
     */
    static VegetationPatchConfig ceilingPatch(const std::string& replaceableTag,
        const BlockState* groundState,
        u32 vegetationFeatureId,
        std::unique_ptr<valueprovider::IntProvider> depth,
        f32 extraBottomBlockChance,
        i32 verticalRange,
        f32 vegetationChance,
        std::unique_ptr<valueprovider::IntProvider> xzRadius,
        f32 extraEdgeColumnChance);
};

/**
 * @brief 方块柱层配置
 *
 * 定义方块柱的一层（高度+方块状态）。
 * 支持固定方块状态和加权随机方块状态提供者。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.BlockColumnConfiguration.Layer
 */
struct BlockColumnLayer {
    /// 层高度提供者
    std::unique_ptr<valueprovider::IntProvider> height;

    /// 层方块状态（固定状态，当 stateProvider 为空时使用）
    const BlockState* state = nullptr;

    /// 层方块状态提供者（加权随机，优先于 state）
    std::unique_ptr<state::WeightedBlockStateProvider> stateProvider;

    BlockColumnLayer() = default;
    BlockColumnLayer(std::unique_ptr<valueprovider::IntProvider> h, const BlockState* s)
        : height(std::move(h))
        , state(s)
    {}
    BlockColumnLayer(
        std::unique_ptr<valueprovider::IntProvider> h, std::unique_ptr<state::WeightedBlockStateProvider> sp)
        : height(std::move(h))
        , stateProvider(std::move(sp))
    {}

    /**
     * @brief 获取当前层在指定位置的方块状态
     *
     * 如果有 stateProvider 则使用随机选择，否则返回固定 state
     */
    [[nodiscard]] const BlockState* getState(math::IRandom& rng) const
    {
        if (stateProvider && !stateProvider->empty()) {
            return stateProvider->getState(rng);
        }
        return state;
    }
};

/**
 * @brief 方块柱配置
 *
 * 定义方块柱的生成参数，用于洞穴藤蔓和垂滴叶的生成。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.BlockColumnConfiguration
 */
struct BlockColumnConfig {
    /// 层列表（从底部到顶部）
    std::vector<BlockColumnLayer> layers;

    /// 生长方向
    Direction direction = Direction::Up;

    /// 放置谓词（检查位置是否允许放置）
    std::unique_ptr<predicate::BlockPredicate> allowedPlacement;

    /// 是否优先保留尖端（截断时从底部删除而非顶部）
    bool prioritizeTip = false;

    BlockColumnConfig() = default;
};

/**
 * @brief 根系配置
 *
 * 定义杜鹃树根系统的生成参数。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.RootSystemConfiguration
 */
struct RootSystemConfig {
    /// 树木特征ID
    u32 treeFeatureId = 0;

    /// 树木所需垂直空间
    i32 requiredVerticalSpaceForTree = 3;

    /// 根半径
    i32 rootRadius = 3;

    /// 根可替换的方块标签
    std::string rootReplaceableTag;

    /// 根方块状态（缠根泥土）
    const BlockState* rootState = nullptr;

    /// 根放置尝试次数
    i32 rootPlacementAttempts = 20;

    /// 根柱最大高度
    i32 rootColumnMaxHeight = 100;

    /// 垂根半径
    i32 hangingRootRadius = 20;

    /// 垂根垂直跨度
    i32 hangingRootsVerticalSpan = 2;

    /// 垂根方块状态
    const BlockState* hangingRootState = nullptr;

    /// 垂根放置尝试次数
    i32 hangingRootPlacementAttempts = 3;

    /// 允许树木位置中的水量
    i32 allowedVerticalWaterForTree = 2;

    RootSystemConfig() = default;
};

/**
 * @brief 简单方块配置
 *
 * 用于放置单个方块（如孢子花、苔藓植被）。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.SimpleBlockConfiguration
 */
struct SimpleBlockConfig {
    /// 要放置的方块状态
    const BlockState* toPlace = nullptr;

    /// 是否调度刻更新
    bool scheduleTick = false;

    SimpleBlockConfig() = default;
    explicit SimpleBlockConfig(const BlockState* state, bool tick = false)
        : toPlace(state)
        , scheduleTick(tick)
    {}
};

/**
 * @brief 随机布尔选择配置
 *
 * 50%概率选择两个特征中的一个。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.RandomBooleanFeatureConfiguration
 */
struct RandomBooleanFeatureConfig {
    /// true时放置的特征ID
    u32 featureTrueId = 0;

    /// false时放置的特征ID
    u32 featureFalseId = 0;

    RandomBooleanFeatureConfig() = default;
    RandomBooleanFeatureConfig(u32 trueId, u32 falseId)
        : featureTrueId(trueId)
        , featureFalseId(falseId)
    {}
};

/**
 * @brief 随机选择配置
 *
 * 从特征列表中均匀随机选择一个。
 *
 * 参考: net.minecraft.world.level.levelgen.feature.configurations.SimpleRandomFeatureConfiguration
 */
struct SimpleRandomFeatureConfig {
    /// 可选特征ID列表
    std::vector<u32> featureIds;

    SimpleRandomFeatureConfig() = default;
    explicit SimpleRandomFeatureConfig(std::vector<u32> ids)
        : featureIds(std::move(ids))
    {}
};

} // namespace mc::world::gen::feature::cave
