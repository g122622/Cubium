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

#include "common/core/Types.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace mc {

class IWorld;
class BlockPos;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 冰方块
 *
 * 透明冰方块，在明亮环境中会融化成水。
 * 挖掘后会变成水源方块。
 *
 * MC ID: minecraft:ice
 */
class IceBlock : public Block {
public:
    /**
     * @brief 构造冰方块
     */
    explicit IceBlock(BlockProperties properties);

    /**
     * @brief 方块被移除后
     * 冰在非寒冷生物群系会融化成水，在温暖光源附近也会融化
     */
    void onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston) override;

    /**
     * @brief 随机刻
     * 在明亮环境中融化
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // 不重写 ticksRandomly：vanilla frosted_ice 没有 randomTicks()，也没有重写
    // isRandomlyTicking。霜冰的融化与修复走的是它自己的 tick 调度，不是随机刻。
};

} // namespace blocks
} // namespace mc
