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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"

namespace mc {

class IWorld;
class BlockPos;
class BlockState;
class Entity;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 岩浆块方块
 *
 * 下界的岩浆块，站在上面会受伤，在水中会产生气泡柱。
 * 不响应随机刻，气泡柱的视觉效果由 BubbleColumnBlock::animateTick 产生。
 *
 * MC ID: minecraft:magma_block
 */
class MagmaBlock : public Block {
public:
    explicit MagmaBlock(BlockProperties properties);

    /**
     * @brief 方块被添加时
     *
     * 调度 tick 以检查气泡柱。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居更新
     *
     * 当上方有水时调度 tick。
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 计划 tick 更新
     *
     * 在上方生成气泡柱。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 实体踩上方块时
     *
     * 非潜行的活体生物踩在岩浆块上会受到烫脚伤害（1点 = 半颗心）。
     */
    void onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;
};

} // namespace blocks
} // namespace mc
