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

#include "GrowingPlantBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 生长植物头部方块（生长尖端）
 *
 * 参考 MC 1.21.11: GrowingPlantHeadBlock
 * 向上或向下生长的植物尖端，具有年龄属性和随机生长逻辑。
 *
 * 子类需要：
 * - 设置 m_growPerTickProbability（每 tick 生长概率）
 * - 实现 getHeadBlock() / getBodyBlock()
 * - 可选覆盖 getGrowIntoState() 自定义生长后状态
 * - 可选覆盖 canGrowInto() 自定义生长目标位置判断
 * - 可选覆盖 updateBodyAfterConvertedFromHead() 在身体转换时传递状态
 */
class GrowingPlantHeadBlock : public GrowingPlantBlock {
public:
    /**
     * @brief 构造生长植物头部方块
     * @param properties 方块属性
     * @param growthDirection 生长方向（UP 或 DOWN）
     * @param shape 碰撞形状
     * @param growPerTickProbability 每 tick 生长概率（0.0-1.0）
     * @param scheduleFluidTicks 是否调度流体 tick
     */
    GrowingPlantHeadBlock(const BlockProperties& properties,
        Direction growthDirection,
        const CollisionShape& shape,
        f32 growPerTickProbability,
        bool scheduleFluidTicks = false);

    ~GrowingPlantHeadBlock() override = default;

    // ========== 年龄属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;

    // ========== 随机 tick ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 骨粉接口 ==========

    /**
     * @brief 检查是否可以使用骨粉
     *
     * 默认实现：检查年龄是否小于最大值。
     * 子类可以覆盖以添加额外条件（如 CaveVines 检查是否有浆果）。
     */
    [[nodiscard]] virtual bool isValidBonemealTarget(
        IBlockReader& world, const BlockPos& pos, const BlockState& state) const;

    // ========== 生长逻辑 ==========

    /**
     * @brief 获取生长后的方块状态
     *
     * 默认实现：cycle AGE + 放置身体方块。
     * 子类可以覆盖以自定义状态（如 CaveVines 随机设置 BERRIES）。
     */
    [[nodiscard]] virtual BlockState getGrowIntoState(
        IWorld& world, const BlockPos& pos, BlockState& currentState, math::IRandom& random);

    /**
     * @brief 检查目标位置是否可以生长
     *
     * 默认实现：检查目标位置是否为空气。
     * 子类可以覆盖以添加额外条件（如海带检查是否含水）。
     */
    [[nodiscard]] virtual bool canGrowInto(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 当身体方块从头部转换时调用
     *
     * 子类可以覆盖以将头部状态传递到身体方块
     * （如 CaveVines 传递 BERRIES 属性）。
     */
    [[nodiscard]] virtual BlockState updateBodyAfterConvertedFromHead(const BlockState& headState) const;

protected:
    f32 m_growPerTickProbability;
};

} // namespace blocks
} // namespace mc
