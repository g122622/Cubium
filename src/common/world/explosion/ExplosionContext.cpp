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
