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

#include "AbstractButtonBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 石头按钮方块
 *
 * 石头按钮被按下后持续 10 tick（1 秒）。
 *
 * 参考: net.minecraft.block.StoneButtonBlock
 */
class StoneButtonBlock : public AbstractButtonBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit StoneButtonBlock(const BlockProperties& properties);

protected:
    void playClickSound(IWorld& world, const BlockPos& pos, bool pressed) const override;
};

} // namespace blocks
} // namespace mc
