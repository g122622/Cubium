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
#include "common/world/block/blocks/RotatedPillarBlock.hpp"

namespace mc {

class IWorld;

namespace blocks {

/**
 * @brief 干草块
 *
 * 柱状方块（继承 RotatedPillarBlock 保留 axis 属性，与原木/骨块同基类）。
 *
 * 物理：实体摔在干草块上时，摔落伤害减少 80%（保留 20%）。与蜂蜜块减伤乘数
 * 相同（0.2），区别于粘液块的 0.0 完全免疫。onLanded 不重写——干草块不做弹跳、
 * 不做特殊速度处理，行为与普通方块一致。
 */
class HayBlock : public RotatedPillarBlock {
public:
    explicit HayBlock(const BlockProperties& properties);
    ~HayBlock() override = default;

    // ========== 实体交互 ==========

    /**
     * @brief 实体摔落于干草块上时调用
     *
     * 干草块减伤 80%（保留 20%）：以 damageMultiplier=0.2 调 causeFallDamage。
     * 对齐 Java HayBlock#fallOn（causeFallDamage(distance, 0.2F, fall)）与 wiki
     * "摔在干草块上的生物受到的跌落伤害会减少 80%"。大落差仍会受少量伤害
     * （非完全免疫，区别于粘液块的 0.0）。
     */
    void onFallenUpon(
        IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance) override;
};

} // namespace blocks
} // namespace mc
