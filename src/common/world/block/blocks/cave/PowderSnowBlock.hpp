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

#include "../../Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 细雪方块
 *
 * 一种类似雪的方块，实体陷入其中会缓慢移动。
 * 没有碰撞箱，玩家和生物可以穿过。
 *
 * 参考: net.minecraft.block.PowderSnowBlock
 */
class PowderSnowBlock : public Block {
public:
    explicit PowderSnowBlock(const BlockProperties& properties);

    ~PowderSnowBlock() override = default;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;
};

} // namespace blocks
} // namespace mc
