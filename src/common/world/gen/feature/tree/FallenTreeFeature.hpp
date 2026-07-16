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

#include "../ConfiguredFeature.hpp"
#include "../Feature.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include "decorator/TreeDecorator.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {

/**
 * @brief 倒木配置（MC FallenTreeConfiguration）
 *
 * trunkProvider：原木方块状态提供者（数据包常为 simple_state_provider，axis=y）。
 * logLength：倒木长度 IntProvider（codec 范围 0..16；实际放置长度 = sample - 2）。
 * stumpDecorators / logDecorators：树桩/倒木的装饰器列表（attached_to_logs、trunk_vine）。
 */
struct FallenTreeConfig : public IFeatureConfig {
    std::unique_ptr<state::BlockStateProvider> trunkProvider;
    std::unique_ptr<valueprovider::IntProvider> logLength;
    std::vector<std::unique_ptr<decorator::TreeDecorator>> stumpDecorators;
    std::vector<std::unique_ptr<decorator::TreeDecorator>> logDecorators;
};

/**
 * @brief 倒木特征（MC FallenTreeFeature）
 *
 * 算法（MC 1.21.11 FallenTreeFeature.placeFallenTree）：
 * 1. placeStump(origin)：在原点放 1 格原木（axis=y，identity 状态修饰），对 stumpDecorators
 *    跑装饰（logs={origin}）。
 * 2. direction = 水平随机方向；i = logLength.sample - 2；倒木起点 =
 *    origin.relative(direction, 2 + nextInt(2))。
 * 3. setGroundHeightForFallenLogStartPos：起点上移 1，再最多下移 6 格寻找 mayPlaceOn
 *    （validTreePos && 下方 isFaceSturdy(UP)）的位置。
 * 4. canPlaceEntireFallenLog：沿 direction 遍历 i 格，每格需 validTreePos；累计非实地
 *    （下方不 sturdy）格数 >2 则失败；通过则 placeFallenLog。
 * 5. placeFallenLog：沿 direction 放 i 格原木（axis 切到 direction.getAxis()），收集 logs，
 *    对 logDecorators 跑装饰。
 *
 * validTreePos = isAir || BlockTags.REPLACEABLE_BY_TREES（对齐 MC TreeFeature.validTreePos）。
 * 装饰阶段为 VegetalDecoration。
 */
class FallenTreeFeature {
public:
    bool place(WorldGenRegion& region,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& origin,
        const FallenTreeConfig& config);

private:
    /// MC FallenTreeFeature.mayPlaceOn：validTreePos && 下方 UP 面 sturdy。
    [[nodiscard]] static bool mayPlaceOn(WorldGenRegion& region, const BlockPos& pos);

    /// MC FallenTreeFeature.isOverSolidGround：下方 isFaceSturdy(UP, Full)。
    [[nodiscard]] static bool isOverSolidGround(WorldGenRegion& region, const BlockPos& pos);

    /// MC TreeFeature.validTreePos：isAir || REPLACEABLE_BY_TREES。
    [[nodiscard]] static bool validTreePos(WorldGenRegion& region, const BlockPos& pos);

    /// MC FallenTreeFeature.placeLogBlock：写原木方块（应用 stateModifier）。
    [[nodiscard]] static BlockPos placeLogBlock(WorldGenRegion& region,
        const FallenTreeConfig& config,
        math::Random& random,
        const BlockPos& pos,
        const std::function<const BlockState*(const BlockState*)>& stateModifier);

    /// MC FallenTreeFeature.decorateLogs：对 logs 跑 decorators。
    static void decorateLogs(WorldGenRegion& region,
        math::Random& random,
        const std::vector<BlockPos>& logs,
        const std::vector<std::unique_ptr<decorator::TreeDecorator>>& decorators);
};

/**
 * @brief 配置化倒木特征
 */
class ConfiguredFallenTreeFeature : public ConfiguredFeatureBase {
public:
    ConfiguredFallenTreeFeature(std::unique_ptr<FallenTreeConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;

    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<FallenTreeConfig> m_config;
    std::string m_name;
    mutable FallenTreeFeature m_feature;
};

} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
