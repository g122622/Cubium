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

#include "ExplosionContext.hpp"
#include "../../entity/core/Entity.hpp"
#include "../block/Block.hpp"
#include "../fluid/Fluid.hpp"

namespace mc {
namespace world {
namespace explosion {

// ========== ExplosionContext ==========

std::optional<f32> ExplosionContext::getExplosionResistance(
    const BlockState& blockState, const fluid::FluidState* fluidState) const
{

    // 如果是空气方块且没有流体，返回空（不消耗爆炸强度）
    if (blockState.isAir()) {
        if (fluidState == nullptr || fluidState->isEmpty()) {
            return std::nullopt;
        }
    }

    // 返回方块抗性
    f32 blockResistance = blockState.resistance();

    // 如果有流体，取方块和流体抗性的较大值
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        const fluid::Fluid& fluid = fluidState->getFluid();
        f32 fluidResistance = fluid.getExplosionResistance();
        return std::max(blockResistance, fluidResistance);
    }

    return blockResistance;
}

bool ExplosionContext::canDestroyBlock(const BlockState& blockState, f32 /*explosionPower*/) const
{

    // 默认情况下，所有方块都可以被破坏
    // 但方块的实际抗性会在射线追踪中决定是否真的被破坏
    return !blockState.isAir();
}

// ========== EntityExplosionContext ==========

EntityExplosionContext::EntityExplosionContext(const Entity* source)
    : m_source(source)
{}

std::optional<f32> EntityExplosionContext::getExplosionResistance(
    const BlockState& blockState, const fluid::FluidState* fluidState) const
{

    // 默认行为：使用基类实现
    // 特殊实体（如凋灵之首）可以覆盖此方法
    return ExplosionContext::getExplosionResistance(blockState, fluidState);
}

bool EntityExplosionContext::canDestroyBlock(const BlockState& blockState, f32 explosionPower) const
{

    // 默认行为：使用基类实现
    // 特殊实体（如 TNT 矿车不破坏铁轨）可以覆盖此方法
    return ExplosionContext::canDestroyBlock(blockState, explosionPower);
}

} // namespace explosion
} // namespace world
} // namespace mc
