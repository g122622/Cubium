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
#include "common/world/block/BlockPos.hpp"
#include "common/world/fluid/FlowingFluid.hpp"
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 水流体基类
 *
 * 特性：
 * - tick延迟: 5 tick
 * - 每格衰减: 1级
 * - 斜坡搜索距离: 4格
 * - 可以形成无限源（2+相邻源头）
 */
class WaterFluid : public FlowingFluid {
public:
    // ========== FlowingFluid接口实现 ==========

    using FlowingFluid::getTickDelay;

    [[nodiscard]] i32 getTickDelay() const override { return 5; }

    [[nodiscard]] bool canDisplace(
        const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const override;

    [[nodiscard]] i32 getLevelDecrease(IWorld& world) const override
    {
        (void)world;
        return 1;
    }

    [[nodiscard]] i32 getSpreadDistance(IWorld& world) const override
    {
        (void)world;
        return 4;
    }

    [[nodiscard]] bool canSourcesMultiply() const override { return true; }

    [[nodiscard]] f32 getExplosionResistance() const override { return 100.0f; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    /**
     * @brief 检查是否等效于指定流体
     *
     * 水和流动水视为等效。
     *
     * @param other 其他流体
     * @return 是否等效
     */
    [[nodiscard]] bool isEquivalentTo(const Fluid& other) const noexcept override;

protected:
    void beforeReplacingBlock(IWorld& world, const BlockPos& pos, const BlockState* state) override;
};

/**
 * @brief 水源头
 *
 * 源头水方块，level=8，没有LEVEL属性。
 */
class WaterSourceFluid : public WaterFluid {
public:
    WaterSourceFluid();

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

private:
    mutable FlowingFluid* m_flowingCache = nullptr;
};

/**
 * @brief 流动水
 *
 * 流动水方块，有LEVEL属性(1-8)和FALLING属性。
 */
class WaterFlowingFluid : public WaterFluid {
public:
    WaterFlowingFluid();

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
     * 水和流动水视为等效。
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
