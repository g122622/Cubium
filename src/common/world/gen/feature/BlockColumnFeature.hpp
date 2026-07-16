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
#include "common/util/Direction.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/ConfiguredFeature.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include "common/world/gen/feature/state/SimpleBlockStateProvider.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc::world::gen::feature::cave {

/**
 * @brief 方块柱层配置
 *
 * 定义方块柱的一层（高度+方块状态）。方块状态由 BlockStateProvider 提供，
 * 运行时按其 kind 采样。
 */
struct BlockColumnLayer {
    /// 层高度提供者
    std::unique_ptr<valueprovider::IntProvider> height;

    /// 层方块状态提供者（simple/weighted/rule_based/... 任意 kind）
    std::unique_ptr<state::BlockStateProvider> stateProvider;

    BlockColumnLayer() = default;
    BlockColumnLayer(std::unique_ptr<valueprovider::IntProvider> h, const BlockState* s)
        : height(std::move(h))
        , stateProvider(std::make_unique<state::SimpleBlockStateProvider>(s))
    {}
    BlockColumnLayer(std::unique_ptr<valueprovider::IntProvider> h, std::unique_ptr<state::BlockStateProvider> sp)
        : height(std::move(h))
        , stateProvider(std::move(sp))
    {}

    /**
     * @brief 获取当前层在指定位置的方块状态
     */
    [[nodiscard]] const BlockState* getState(const IWorld& world, math::IRandom& rng, i32 x, i32 y, i32 z) const
    {
        if (stateProvider != nullptr) {
            return stateProvider->getState(world, rng, x, y, z);
        }
        return nullptr;
    }
};

/**
 * @brief 方块柱配置
 *
 * 定义方块柱的生成参数，用于洞穴藤蔓和垂滴叶的生成。
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
 * @brief 方块柱特征
 *
 * 沿指定方向放置多层方块柱。
 * 用于洞穴藤蔓和大型垂滴叶茎的生成。
 */
class BlockColumnFeature {
public:
    /**
     * @brief 在指定位置放置方块柱
     * @param region 世界生成区域
     * @param random 随机数生成器
     * @param pos 起始位置
     * @param config 方块柱配置
     * @return 是否成功放置
     */
    static bool place(
        WorldGenRegion& region, math::Random& random, const BlockPos& pos, const BlockColumnConfig& config);
};

/**
 * @brief 配置化方块柱特征
 *
 * 数据驱动下 placement 链由 PlacedFeature 持有并在 place() 前走完，
 * 本类只负责在已确定的 pos 处放置方块柱。
 */
class ConfiguredBlockColumnFeature : public ConfiguredFeatureBase {
public:
    ConfiguredBlockColumnFeature(std::unique_ptr<BlockColumnConfig> config, const char* featureName);

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) const override;
    [[nodiscard]] const char* name() const override { return m_name.c_str(); }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

private:
    std::unique_ptr<BlockColumnConfig> m_config;
    std::string m_name;
};

} // namespace mc::world::gen::feature::cave
