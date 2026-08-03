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
#include "common/world/fluid/Fluid.hpp"

namespace mc {
namespace fluid {

/**
 * @brief 空流体
 *
 * 表示无流体的状态。单例模式。
 */
class EmptyFluid : public Fluid {
public:
    EmptyFluid();

    // ========== Fluid接口实现 ==========

    [[nodiscard]] bool isSource(const FluidState& state) const override { return false; }

    [[nodiscard]] i32 getLevel(const FluidState& state) const override { return 0; }

    [[nodiscard]] i32 getTickDelay() const override { return 0; }

    [[nodiscard]] bool canSourcesMultiply() const override { return false; }

    [[nodiscard]] const BlockState* getBlockState(const FluidState& state) const override;

    [[nodiscard]] f32 getExplosionResistance() const override { return 0.0f; }

    [[nodiscard]] Vector3 getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const override
    {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    void tick(IWorld& world, const BlockPos& pos, FluidState& state) override
    {
        // 空流体不执行tick
        (void)world;
        (void)pos;
        (void)state;
    }

    [[nodiscard]] bool canDisplace(
        const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const override
    {
        (void)state;
        (void)world;
        (void)pos;
        (void)fluid;
        (void)dir;
        return true;
    }

    [[nodiscard]] bool ticksRandomly() const noexcept override { return false; }

    [[nodiscard]] bool isEmpty() const override { return true; }
};

} // namespace fluid
} // namespace mc
