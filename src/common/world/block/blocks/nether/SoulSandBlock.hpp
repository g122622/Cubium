/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 灵魂沙方块
 *
 * 下界的灵魂沙，走在上面会被减速（speedFactor=0.4，由 BlockProperties 设置）。
 * 在水源下方放置时会产生涌流气泡柱（向上推动实体），与岩浆块产生的涡流气泡柱相反。
 * 不响应随机刻，气泡柱的视觉效果由 BubbleColumnBlock::animateTick 产生。
 *
 * MC ID: minecraft:soul_sand
 */
class SoulSandBlock : public Block {
public:
    explicit SoulSandBlock(BlockProperties properties);

    /**
     * @brief 方块被添加时
     *
     * 调度 tick 以检查上方水源并生成涌流气泡柱。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 邻居更新
     *
     * 当上方变为水时调度 tick，以便在水源上方延伸气泡柱。
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 计划 tick 更新
     *
     * 在上方水源位置生成涌流气泡柱（DRAG=false，向上推动）。
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;
};

} // namespace blocks
} // namespace mc
