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

#include "common/world/block/blocks/base/SimpleBlock.hpp"
#include "common/world/block/capability/ILiquidSealed.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 海带茎方块（海带的非尖端部分）
 *
 * 无状态属性的水下植物方块，与 KelpBlock 共同构成一株海带。
 *
 * 实现 ILiquidSealed：海带茎不被水流替换。缺此实现时，水流每次 tick 都会把海带茎当作
 * 被水冲毁的方块，经 WaterFluid::beforeReplacingBlock 生成一次掉落物。
 *
 */
class KelpPlantBlock : public SimpleBlock, public ILiquidSealed {
public:
    explicit KelpPlantBlock(BlockProperties properties);
    ~KelpPlantBlock() override = default;

    /** @brief 海带茎部始终携带水源流体。 */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查上方一格能否托起海带茎
     *
     * 原版 KelpPlantBlock 继承 GrowingPlantBodyBlock.canSurvive，并转发海带头的
     * canAttachTo（排除岩浆块）：上一格是海带头/海带茎，或顶面是完整支撑面。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;
};

} // namespace blocks
} // namespace mc
