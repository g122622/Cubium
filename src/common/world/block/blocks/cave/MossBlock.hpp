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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../Block.hpp"
#include "../../IGrowable.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 苔藓方块
 *
 * 一种绿色的装饰性方块，可以通过骨粉传播到相邻的石头方块。
 * 骨粉会在3x3x7范围内将MOSS_REPLACEABLE标签中的方块替换为苔藓块，
 * 并在相邻空气位置放置苔藓地毯和杜鹃花丛。
 *
 * 参考: net.minecraft.block.MossBlock
 */
class MossBlock : public Block, public IGrowable {
public:
    explicit MossBlock(const BlockProperties& properties);

    ~MossBlock() override = default;

    // ========== IGrowable 接口 ==========

    /**
     * @brief 苔藓方块始终可以使用骨粉
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 骨粉对苔藓方块100%有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉传播苔藓
     *
     * 在以骨粉位置为中心、上方1格下方5格的3x7x3范围内：
     * 1. 将MOSS_REPLACEABLE标签中的方块替换为苔藓块
     * 2. 在苔藓块旁的空气位置随机放置苔藓地毯或杜鹃花丛
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

private:
    /**
     * @brief 在苔藓块上方的空气位置放置植被
     *
     * 50%概率放苔藓地毯，10%概率放杜鹃花丛
     */
    static void _placeMossVegetation(IWorld& world, math::IRandom& random, const BlockPos& pos);
};

} // namespace blocks
} // namespace mc
