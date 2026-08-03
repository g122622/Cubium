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

#include "DispenserBlock.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc {
namespace blocks {

/**
 * @brief 投掷器方块
 *
 * 投掷器简单地投掷物品，没有特殊行为。
 * 继承自 DispenserBlock，复用基本发射逻辑。
 *
 * ## 特性
 * - 9格存储空间
 * - 红石激活时随机投掷物品
 * - 向容器输出物品
 * - 方向性：可向6个方向投掷
 *
 * ## 与发射器的区别
 * - 投掷器只投掷物品，没有特殊行为
 * - 发射器对特定物品有特殊行为（如箭矢发射、火焰球等）
 * - 投掷器会尝试向相邻容器输出物品
 *
 * 参考: net.minecraft.block.DropperBlock
 */
class DropperBlock : public DispenserBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DropperBlock(const BlockProperties& properties) noexcept;

    // ========== 重写发射器方法 ==========

    /**
     * @brief 投掷物品（重写发射器的发射行为）
     *
     * 投掷器使用简单的投掷行为，不使用发射行为注册表。
     * 如果前方是容器，会尝试将物品放入容器。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void dispense(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

protected:
    /**
     * @brief 尝试从投掷器位置投掷物品
     *
     * @param world 世界引用
     * @param pos 投掷器位置
     * @param state 当前方块状态
     * @return true 如果成功投掷
     */
    bool tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state) override;
};

} // namespace blocks
} // namespace mc
