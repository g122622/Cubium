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
#include "../../Block.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/BooleanProperty.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 雪覆盖泥土方块基类
 *
 * 持有 SNOWY 布尔属性，表示顶部是否覆盖雪。提供放置时与邻居更新时的 SNOWY 同步逻辑，
 * 但不包含蔓延/退化机制（那是 SpreadableSnowyDirtBlock 的职责）。
 *
 * 典型子类：
 * - SpreadableSnowyDirtBlock：草方块/菌丝，在 SNOWY 基础上额外实现蔓延与退化。
 * - podzol 等不蔓延但需要 SNOWY 属性的方块直接用本类。
 */
class SnowyDirtBlock : public Block {
public:
    explicit SnowyDirtBlock(BlockProperties properties);

    /**
     * @brief 获取放置时的方块状态
     *
     * 根据放置位置上方是否有雪设置 SNOWY 属性。
     *
     * @param context 放置上下文
     * @return 方块状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 方块更新后处理
     *
     * 当上方方块变化时更新 SNOWY 属性。
     *
     * @param state 当前方块状态
     * @param facing 更新的方向
     * @param facingState 邻居状态
     * @param world 世界
     * @param currentPos 当前方块位置
     * @param facingPos 邻居位置
     * @return 更新后的状态
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 获取 SNOWY 属性
     * @return SNOWY 布尔属性引用
     */
    [[nodiscard]] static const BooleanProperty& SNOWY() { return BlockStateProperties::SNOWY(); }
};

} // namespace blocks
} // namespace mc
