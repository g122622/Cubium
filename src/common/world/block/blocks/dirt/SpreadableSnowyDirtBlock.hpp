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
#include "../../IGrowable.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
class BlockItemUseContext;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 可蔓延的雪覆盖泥土方块基类
 *
 * 这是草方块和菌丝的基类，提供蔓延和退化机制：
 * - 当光照不足时退化成泥土
 * - 当光照足够时向周围泥土蔓延
 * - 支持 SNOWY 属性，表示顶部是否覆盖雪
 */
class SpreadableSnowyDirtBlock : public Block {
public:
    explicit SpreadableSnowyDirtBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 处理蔓延和退化逻辑：
     * - 光照不足时退化成泥土
     * - 光照充足时向周围泥土蔓延
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

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

protected:
    /**
     * @brief 检查是否为雪覆盖条件
     *
     * 检查上方是否有雪（仅1层）或者光照条件是否满足。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件
     */
    [[nodiscard]] static bool isSnowyConditions(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否为雪覆盖且非水下条件
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件且不在水下
     */
    [[nodiscard]] static bool isSnowyAndNotUnderwater(IWorld& world, const BlockPos& pos, const BlockState& state);
};

/**
 * @brief 草方块
 *
 * 可蔓延的草方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。骨粉可以在其上方生成花朵和草。
 *
 * MC ID: minecraft:grass_block
 */
class GrassBlock : public SpreadableSnowyDirtBlock, public IGrowable {
public:
    explicit GrassBlock(BlockProperties properties);

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查草方块是否可以使用骨粉
     *
     * 草方块上方需要有空气才能使用骨粉。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 草方块骨粉总是有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 在草方块上方散布花朵和短草。当前为简化实现。
     *
     * TODO: 完整实现需要 PlacedFeature/ConfiguredFeature 系统，当前为简化版本，
     * 仅放置短草和少量花朵。待世界生成系统完善后应改为从生物群系获取花列表。
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 草方块为邻居传播型骨粉类型
     *
     * 粒子在方块上方水平扩散，数量为传入值的3倍。
     */
    [[nodiscard]] BoneMealType getBoneMealType() const override { return BoneMealType::NEIGHBOR_SPREADER; }
};

/**
 * @brief 菌丝方块
 *
 * 可蔓延的菌丝方块，在光照充足时向周围泥土蔓延，
 * 在光照不足时退化成泥土。
 *
 * MC ID: minecraft:mycelium
 */
class MyceliumBlock : public SpreadableSnowyDirtBlock {
public:
    explicit MyceliumBlock(BlockProperties properties);
};

} // namespace blocks
} // namespace mc
