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

#include "Fluid.hpp"

#include "FluidRegistry.hpp"
#include "FluidTags.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/FluidProperties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace fluid {

// ============================================================================
// FluidState 实现
// ============================================================================

FluidState::FluidState(const Fluid& fluid,
    std::vector<size_t> valueIndices,
    const std::vector<StateHolder<Fluid, FluidState>::PropertyLayout>* propertyLayouts,
    const std::vector<FluidState*>* allStates,
    u32 stateId)
    : StateHolder<Fluid, FluidState>(&fluid, std::move(valueIndices), propertyLayouts, allStates, stateId)
{
    m_fluidId = fluid.fluidId();
}

bool FluidState::isSource() const
{
    return m_owner->isSource(*this);
}

i32 FluidState::getLevel() const
{
    return m_owner->getLevel(*this);
}

bool FluidState::isFalling() const
{
    auto& falling = FluidProperties::FALLING();
    auto opt = getOptional(falling);
    return opt.has_value() && opt.value();
}

f32 FluidState::getHeight() const
{
    i32 level = getLevel();
    return static_cast<f32>(level) / static_cast<f32>(MAX_AMOUNT);
}

f32 FluidState::getActualHeight(IWorld& world, const BlockPos& pos) const
{
    // 检查上方是否有同种流体（满高度情况）
    BlockPos above = pos.up();
    const FluidState* aboveFluid = world.getFluidState(above);

    if (aboveFluid != nullptr && !aboveFluid->isEmpty() && aboveFluid->getFluid().isEquivalentTo(*m_owner)) {
        // 上方有同种流体，返回满高度
        return 1.0f;
    }

    // 返回基础高度
    return getHeight();
}

bool FluidState::isEmpty() const
{
    return m_owner->isEmpty();
}

const Fluid& FluidState::getFluid() const
{
    return *m_owner;
}

const BlockState* FluidState::getBlockState() const
{
    if (m_cachedBlockState == nullptr) {
        m_cachedBlockState = m_owner->getBlockState(*this);
    }
    return m_cachedBlockState;
}

Vector3 FluidState::getFlow(IBlockReader& world, const BlockPos& pos) const
{
    return m_owner->getFlow(world, pos, *this);
}

bool FluidState::canDisplace(IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const
{
    return m_owner->canDisplace(*this, world, pos, fluid, dir);
}

std::string FluidState::ownerName() const
{
    return m_owner->toString();
}

// ============================================================================
// Fluid 实现
// ============================================================================

Fluid* Fluid::getFluid(u32 fluidId)
{
    return FluidRegistry::instance().getFluid(fluidId);
}

Fluid* Fluid::getFluid(const ResourceLocation& id)
{
    return FluidRegistry::instance().getFluid(id);
}

void Fluid::createFluidState(std::unique_ptr<StateContainer<Fluid, FluidState>> container)
{
    m_stateContainer = std::move(container);
}

void Fluid::setDefaultState(const FluidState& state)
{
    m_defaultState = &state;
}

Vector3 Fluid::getFlow(IBlockReader& world, const BlockPos& pos, const FluidState& state) const
{
    // 默认实现：无流动
    (void)world;
    (void)pos;
    (void)state;
    return Vector3(0.0f, 0.0f, 0.0f);
}

void Fluid::tick(IWorld& world, const BlockPos& pos, FluidState& state)
{
    // 默认实现：无操作
    (void)world;
    (void)pos;
    (void)state;
}

void Fluid::randomTick(IWorld& world, const BlockPos& pos, const FluidState& state, math::IRandom& random)
{
    // 默认实现：无操作
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

bool Fluid::canDisplace(
    const FluidState& state, IWorld& world, const BlockPos& pos, const Fluid& fluid, Direction dir) const
{
    // 默认实现：如果新流体相同，则不可替换
    (void)state;
    (void)world;
    (void)pos;
    (void)dir;
    return !isEquivalentTo(fluid);
}

CollisionShape Fluid::getShape(const FluidState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 基类默认返回空碰撞形状
    // EmptyFluid 继承此行为（空流体无碰撞）
    // FlowingFluid 重写此方法，根据流体高度返回正确的碰撞形状
    (void)state;
    (void)world;
    (void)pos;
    return CollisionShape::empty();
}

bool Fluid::isIn(const FluidTag& tag) const
{
    return tag.contains(*this);
}

} // namespace fluid
} // namespace mc
