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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 双格植物方块基类
 *
 * 用于高度为2格的植物（高草、大型蕨、向日葵等）。
 * 上半部分和下半部分使用同一个方块，通过 HALF 属性区分。
 *
 * 状态属性：
 * - HALF: DoubleBlockHalf (UPPER, LOWER)
 *
 * 参考: net.minecraft.block.DoublePlantBlock
 */
class DoublePlantBlock : public BushBlock {
public:
    // 使用 BlockStateProperties 中的 DoubleBlockHalf 枚举
    using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DoublePlantBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~DoublePlantBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取半部分属性
     */
    static const EnumProperty<DoubleBlockHalf>& halfProperty();

    /**
     * @brief 获取方块的半部分
     */
    [[nodiscard]] DoubleBlockHalf getHalf(const BlockState& state) const;

    /**
     * @brief 创建指定半部分的状态
     */
    [[nodiscard]] BlockState withHalf(DoubleBlockHalf half) const;

    // ========== 放置逻辑 ==========

    /**
     * @brief 获取放置状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 其他 ==========

    /**
     * @brief 放置上半部分
     * @param world 世界
     * @param pos 位置（下半部分位置）
     * @param state 下半部分状态
     * @param flags 更新标志
     * @return 是否成功放置
     */
    static bool placeAt(IWorld& world, const BlockPos& pos, const BlockState& state, i32 flags);

protected:
    /// 下半部分形状
    CollisionShape m_lowerShape;
    /// 上半部分形状
    CollisionShape m_upperShape;
};

} // namespace blocks
} // namespace mc
