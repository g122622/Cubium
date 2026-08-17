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
 */

#pragma once

#include "../vegetation/FlowerBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 仙人掌花方块
 *
 * 生长在仙人掌顶部的小型花朵，也可放置在耕地和具有实心顶面的方块上。
 * 比普通花朵更宽更高（14x12像素 vs 6x6像素）。
 * 不具有可疑炖汤效果。
 *
 * MC ID: minecraft:cactus_flower
 */
class CactusFlowerBlock : public FlowerBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CactusFlowerBlock(const BlockProperties& properties);

    ~CactusFlowerBlock() noexcept override = default;

protected:
    /**
     * @brief 检查下方是否可支撑
     *
     * 仙人掌花可放置在：
     * - 仙人掌方块上方
     * - 耕地
     * - 具有实心顶面的方块（SupportType::CENTER）
     */
    [[nodiscard]] bool canSustain(
        const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const override;
};

} // namespace blocks
} // namespace mc
