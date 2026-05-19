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

#include "../../../../util/property/Properties.hpp"
#include "../../../../util/property/StateContainer.hpp"
#include "../../Block.hpp"

#include <unordered_map>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 雪层方块
 *
 * 可堆叠的雪层方块（1-8层），在光照足够时会融化。
 * 每层高度为 2 像素（1/8 方块）。
 *
 * MC ID: minecraft:snow
 *
 * 参考 MC 1.16.5 SnowBlock
 */
class SnowBlock : public Block {
public:
    /**
     * @brief 获取 LAYERS 属性
     */
    [[nodiscard]] static const IntegerProperty& LAYERS() { return BlockStateProperties::LAYERS_1_8(); }

    /**
     * @brief 构造雪层方块
     */
    explicit SnowBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 在光照 > 11 时融化（掉落雪层物品并移除方块）。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const override { return true; }
};

} // namespace blocks
} // namespace mc
