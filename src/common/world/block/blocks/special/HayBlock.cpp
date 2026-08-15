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

#include "HayBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace blocks {

// ========== HayBlock ==========

HayBlock::HayBlock(const BlockProperties& properties)
    : RotatedPillarBlock(properties)
{}

void HayBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    // 干草块减伤 80%（保留 20%）：以 damageMultiplier=0.2 调 causeFallDamage。
    // 对齐 Java HayBlock#fallOn（causeFallDamage(distance, 0.2F, fall)）与 wiki
    // "摔在干草块上的生物受到的跌落伤害会减少 80%"。LivingEntity::causeFallDamage 计算
    // (distance-3)*0.2，大落差仍受少量伤害（非完全免疫，区别于粘液块的 0.0 完全免疫）。
    // onLanded 不重写：干草块不弹跳、不做特殊速度处理，行为与普通方块一致。
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    entity.causeFallDamage(fallDistance, physics::HAY_BLOCK_FALL_DAMAGE_MULTIPLIER, DamageSources::fall());
}

} // namespace blocks
} // namespace mc
