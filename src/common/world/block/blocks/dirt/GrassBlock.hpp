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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/capability/IGrowable.hpp"
#include "common/world/block/core/Block.hpp"

#include <functional>

namespace mc {

class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 草方块
 *
 * 可蔓延的草方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。骨粉可以在其上方生成花朵和草。
 *
 * MC ID: minecraft:grass_block
 */
class GrassBlock : public SpreadableSnowyDirtBlock, public IGrowable {
public:
    /**
     * @brief 花卉放置器函数类型
     *
     * 接收 placed_feature 的 ResourceLocation id 和随机数生成器，
     * 返回对应的花朵 BlockState（或 nullptr 表示该 placed_feature
     * 不是 ConfiguredFlowerFeature 或解析失败）。
     *
     * 该回调由 server 侧后置注入，内部通过 PlacedFeatureRegistry
     * 解析 placed_feature 并从中随机选择花朵方块状态。
     * common 层不依赖 PlacedFeatureRegistry / ConfiguredFlowerFeature。
     *
     * @param placedFeatureId 花卉 placed_feature 的 ResourceLocation
     * @param random 随机数生成器
     * @return 选中的花朵 BlockState，或 nullptr
     */
    using FlowerPlacer = std::function<const BlockState*(const ResourceLocation&, math::IRandom&)>;

    explicit GrassBlock(BlockProperties properties);

    /**
     * @brief 后置注入花卉放置器
     *
     * common 层方块注册时无法引用 server gen 的 PlacedFeatureRegistry，
     * 因此构造时 flowerPlacer 为空，由 server 侧初始化阶段调用此方法
     * 注入真实的 FlowerPlacer。
     */
    void setFlowerPlacer(FlowerPlacer flowerPlacer)
    {
        m_flowerPlacer = std::move(flowerPlacer);
    }

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查草方块是否可以使用骨粉
     *
     * 草方块上方需要有空气才能使用骨粉。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 草方块骨粉总是有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 在草方块上方散布花朵和短草。从生物群系获取花列表，
     * 不同生物群系会产生不同种类的花朵（如平原蒲公英/虞美人、
     * 沼泽兰花、繁花森林全部花种等）。
     * 没有花卉特征的生物群系回退到蒲公英。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 草方块为邻居传播型骨粉类型
     *
     * 粒子在方块上方水平扩散，数量为传入值的3倍。
     */
    [[nodiscard]] BoneMealType getBoneMealType() const override
    {
        return BoneMealType::NEIGHBOR_SPREADER;
    }

private:
    /// 花卉放置器回调（由 server 侧后置注入）
    FlowerPlacer m_flowerPlacer;
};

} // namespace blocks
} // namespace mc
