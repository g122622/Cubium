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

#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/vegetation/FlowerBlock.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 凋灵玫瑰方块
 *
 * 继承 FlowerBlock 保留花的特性（放置规则、可疑炖汤效果 Wither I/6s）。
 *
 * 额外行为：非和平难度下，任何边界箱与凋灵玫瑰所在方块相交的活体生物（非亡灵）
 * 会持续获得凋零 I 效果（40 tick / 2 秒），每 1 一次 1hp 伤害（受击后伤害免疫
 * 减慢至每 0.5 秒一次）。亡灵族（含凋灵骷髅、凋灵 boss）免疫。
 *
 * MC ID: minecraft:wither_rose
 */
class WitherRoseBlock : public FlowerBlock {
public:
    using FlowerBlock::FlowerBlock;

    ~WitherRoseBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体碰撞时调用
     *
     * 非和平难度下，对边界箱与本方块相交的活体生物（非亡灵）施加凋零 I / 40 tick。
     * 已有凋零效果则跳过（避免刷新剩余时间）。亡灵族（EntityTypeTags::UNDEAD，含
     * 凋灵骷髅与凋灵 boss）免疫。
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;
};

} // namespace blocks
} // namespace mc
