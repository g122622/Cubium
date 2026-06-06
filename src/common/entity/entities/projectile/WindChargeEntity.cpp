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

#include "WindChargeEntity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {

WindChargeEntity::WindChargeEntity(EntityId id)
    : ThrowableEntity(id)
{}

std::unique_ptr<Entity> WindChargeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WindChargeEntity>(EntityId(0));
}

void WindChargeEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    auto* living = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (living != nullptr) {
        // 风弹造成1点伤害
        // TODO(trial_chambers): 使用 DamageSources::windBurst 或等效伤害源
        auto damageSource = DamageSources::generic();
        living->hurt(damageSource, PLAYER_DAMAGE);
    }

    // 命中实体后触发风爆
    applyWindBurst();
    remove();
}

void WindChargeEntity::onBlockHit(const RayTraceResult& result)
{
    // 命中方块后触发风爆
    applyWindBurst();
    remove();
}

void WindChargeEntity::onImpact(const RayTraceResult& result)
{
    // onEntityHit / onBlockHit 已处理
}

void WindChargeEntity::applyWindBurst()
{
    if (m_hasBurst) {
        return;
    }
    m_hasBurst = true;

    if (m_world == nullptr) {
        return;
    }

    // TODO(trial_chambers): 实现风爆效果 - 推开范围内的实体
    // 1. 获取风爆范围内的所有实体
    // 2. 对内圈（3.5格）内的实体施加较大推力
    // 3. 对外圈（5.5格）内的实体施加衰减推力
    // 4. 推力方向从风爆中心指向实体
    // 5. 推力Y分量额外+0.4使实体被弹起
    // 6. 播放风爆音效和粒子效果
    // 7. 对弹射物也施加推力

    // 参考: net.minecraft.world.entity.projectile.WindCharge
    // 完整实现需要:
    // - world->getEntitiesInAABB() 范围查询
    // - 实体推力计算: direction.normalize() * power + Vec3(0, 0.4, 0)
    // - 音效: SoundEvents.ENTITY_WIND_CHARGE_WIND_BURST
    // - 粒子: ParticleTypes.GUST 或 GUST_EMITTER
}

} // namespace entity
} // namespace mc
