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
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/FlowingFluid.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 岩浆流体基类
 *
 * 特性：
 * - tick延迟: 主世界30tick，下界10tick
 * - 每格衰减: 主世界2级，下界1级
 * - 斜坡搜索距离: 主世界2格，下界4格
 * - 不能形成无限源
 * - 随机tick可能引燃周围方块
 */
class LavaFluid : public FlowingFluid {
public:
    [[nodiscard]] i32 getTickDelay(IWorld& world) const override;

    [[nodiscard]] i32 getTickDelay(
        IWorld& world, const BlockPos& pos, const FluidState& state, const FluidState& correctState) const override;

    [[nodiscard]] i32 getTickDelay() const override
    {
        // 基础延迟值，实际延迟由 getTickDelay(IWorld&) 根据维度计算
        // 主世界: 30 tick, 下界: 10 tick
        return 30;
    }

    [[nodiscard]] bool canDisplace(
        const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const override;

    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override;

    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override;

    [[nodiscard]] bool canSourcesMultiply() const override { return false; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }

    void randomTick(IWorld& world, const BlockPos& pos, const FluidState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    /**
     * @brief 检查是否等效于指定流体
     *
     * 岩浆和流动岩浆视为等效。
     *
     * @param other 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& other) const noexcept override;

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state) override;

    /**
     * @brief 流入指定位置（重写以处理岩浆与水的交互）
     *
     * 只有向下流动时(direction == DOWN)才检查水交互
     * 岩浆向下流入水时，把目标位置变成石头
     *
     * @param world 世界
     * @param pos 目标位置
     * @param blockState 目标位置的方块状态
     * @param dir 流入方向
     * @param fluidState 流入的流体状态
     */
    void flowInto(IWorld& world,
        const BlockPos& pos,
        const BlockState* blockState,
        Direction dir,
        const FluidState& fluidState) override;

private:
    /**
     * @brief 检查周围方块是否可燃
     *
     * @param world 世界
     * @param pos 位置
     * @return 周围是否有可燃方块
     */
    [[nodiscard]] bool _isSurroundingBlockFlammable(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检查指定方块是否可燃
     *
     * @param world 世界
     * @param pos 位置
     * @return 方块是否可燃
     */
    [[nodiscard]] bool _isBlockFlammable(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 触发岩浆效果（烟雾和嘶嘶声）
     *
     * @param world 世界
     * @param pos 位置
     */
    void _triggerEffects(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 岩浆源头
 *
 * 源头岩浆方块，level=8，没有LEVEL属性。
 */
class LavaSourceFluid : public LavaFluid {
public:
    LavaSourceFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override
    {
        (void)state;
        return true;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override
    {
        (void)state;
        return SOURCE_LEVEL;
    }

    [[nodiscard]] FlowingFluid& getFlowing() override;
    [[nodiscard]] FlowingFluid& getStill() override { return *this; }

    /**
     * @brief 检查是否等效于指定流体
     *
     * 岩浆和流动岩浆视为等效。
     *
     * @param fluid 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& fluid) const noexcept override;

private:
    mutable FlowingFluid* m_flowingCache = nullptr;
};

/**
 * @brief 流动岩浆
 *
 * 流动岩浆方块，有LEVEL属性(1-8)和FALLING属性。
 */
class LavaFlowingFluid : public LavaFluid {
public:
    LavaFlowingFluid();

    [[nodiscard]] bool isSource(const FluidState& state) const override
    {
        (void)state;
        return false;
    }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override;

    [[nodiscard]] FlowingFluid& getFlowing() override { return *this; }
    [[nodiscard]] FlowingFluid& getStill() override;

    /**
     * @brief 检查是否等效于指定流体
     *
     * 岩浆和流动岩浆视为等效。
     *
     * @param fluid 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& fluid) const noexcept override;

private:
    mutable FlowingFluid* m_stillCache = nullptr;
};

} // namespace fluid
} // namespace mc
