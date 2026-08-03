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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/DecorationStage.hpp"
#include <memory>
#include <string>

namespace mc::world::gen::feature::cave {

/**
 * @brief 根系配置
 *
 * 定义杜鹃树根系统的生成参数。
 */
struct RootSystemConfig {
    /// 树木特征ID（ConfiguredFeatureRegistry 中的 ResourceLocation）
    ResourceLocation treeFeatureId;

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
 * @brief 根系特征
 *
 * 生成杜鹃树及其根系：先向上寻找有效位置放置树木，
 * 然后填充缠根泥土柱，最后在下方放置垂根。
 * 用于ROOTED_AZALEA_TREE。
 */
class RootSystemFeature {
public:
    static bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos,
        const RootSystemConfig& config);

private:
    /**
     * @brief 检查树木是否有足够的垂直空间
     */
    static bool spaceForTree(WorldGenRegion& region, const BlockPos& pos, i32 requiredSpace, i32 allowedWater);

    /**
     * @brief 放置缠根泥土柱
     */
    static void placeRootedDirtColumn(WorldGenRegion& region,
        math::Random& random,
        const BlockPos& origin,
        i32 targetY,
        const RootSystemConfig& config);

    /**
     * @brief 放置垂根
     */
    static void placeHangingRoots(
        WorldGenRegion& region, math::Random& random, const BlockPos& rootCenter, const RootSystemConfig& config);
};

/**
 * @brief 配置化根系特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，本类只负责在已确定的 pos 处放置。
 */
class ConfiguredRootSystemFeature : public ConfiguredFeatureBase {
public:
    ConfiguredRootSystemFeature(std::unique_ptr<RootSystemConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<RootSystemConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
