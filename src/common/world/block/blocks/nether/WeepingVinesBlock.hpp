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

#include "../growing_plant/GrowingPlantHeadBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 垂泪藤顶端方块（向下生长的藤蔓尖端）
 *
 * 参考 MC 1.21.11: WeepingVinesBlock (继承自 GrowingPlantHeadBlock)
 * 从下界天花板向下生长的藤蔓头部，具有 AGE_0_25 属性。
 * 每 tick 10% 概率向下生长一格；骨粉可加速生长。
 * 柱身由 WEEPING_VINES_PLANT 承载，头部由本方块承载。
 *
 * 状态属性：AGE_0_25
 */
class WeepingVinesBlock : public GrowingPlantHeadBlock {
public:
    explicit WeepingVinesBlock(const BlockProperties& properties);
    ~WeepingVinesBlock() override = default;

    [[nodiscard]] const Block* getHeadBlock() const override;
    [[nodiscard]] const Block* getBodyBlock() const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;
};

} // namespace blocks
} // namespace mc
