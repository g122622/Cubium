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
#include "Explosion.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

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

bool ExplosionContext::shouldDamageEntity(const Explosion& /*explosion*/, const Entity& /*entity*/) const
{
    // 默认：所有实体都可被爆炸伤害
    return true;
}

f32 ExplosionContext::getKnockbackMultiplier(const Explosion& /*explosion*/, const Entity& /*entity*/) const
{
    // 默认：正常击退倍率
    return 1.0f;
}

f32 ExplosionContext::getEntityDamageAmount(const Explosion& explosion, const Entity& entity, f32 seenPercent) const
{
    // 伤害公式：
    //   d0 = 距离 / (radius*2) = 距离比例（钳到 1.0）
    //   d1 = (1-d0) * seenPercent = 冲击系数 impact
    //   damage = floor((d1²+d1)/2 * 7 * (radius*2) + 1)
    // damageRadius = radius*2（实体影响范围），与采样距离的分母一致
    const f32 damageRadius = explosion.damageRadius();
    const Vector3 entityPos = entity.position();
    const Vector3 delta = entityPos - explosion.position();
    const f32 distSq = delta.lengthSquared();
    const f32 d0 = std::min(1.0f, std::sqrt(distSq) / damageRadius);
    const f32 d1 = (1.0f - d0) * seenPercent;
    return std::floor((d1 * d1 + d1) / 2.0f * game::explosion::DAMAGE_MULTIPLIER * damageRadius + 1.0f);
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

// ========== WitherSkullExplosionContext ==========

WitherSkullExplosionContext::WitherSkullExplosionContext(const Entity* source, bool isDangerous)
    : EntityExplosionContext(source)
    , m_isDangerous(isDangerous)
{}

std::optional<f32> WitherSkullExplosionContext::getExplosionResistance(
    const BlockState& blockState, const fluid::FluidState* fluidState) const
{
    // 获取基类的默认爆炸抗性
    auto baseResistance = EntityExplosionContext::getExplosionResistance(blockState, fluidState);

    // 普通凋灵之首：不做任何修改
    if (!m_isDangerous) {
        return baseResistance;
    }

    // 蓝色凋灵之首（dangerous skull）：对不在 WITHER_IMMUNE 中的非空方块，限制抗性上限为 0.8
    // 其中 WitherBoss.canDestroy() = !state.isAir() && !state.is(BlockTags.WITHER_IMMUNE)

    if (!blockState.isAir() && !BlockTags::WITHER_IMMUNE().contains(blockState)) {
        if (baseResistance.has_value()) {
            return std::min(0.8f, *baseResistance);
        }
    }

    return baseResistance;
}

} // namespace explosion
} // namespace world
} // namespace mc
