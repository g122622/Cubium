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

#include "AbstractFireballEntity.hpp"

#include "../../../core/Constants.hpp"
#include "../../../core/Types.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../effect/EffectInstance.hpp"
#include "../../effect/EffectType.hpp"
#include <cmath>

namespace mc {
namespace entity {

AbstractFireballEntity::AbstractFireballEntity(LegacyEntityType type, EntityId id)
    : DamagingProjectileEntity(type, id)
{}

FireballEntity::FireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(6.0f);
}

std::unique_ptr<Entity> FireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FireballEntity>(LegacyEntityType::Unknown, 0);
}

void FireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Fireball, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Fireball, this, this, false);
    }

    (void)damageSource;
    // TODO: 接入 LivingEntity::hurt 后补齐火球直接伤害
    result.hitEntity->setFire(5);
    // TODO: 接入爆炸系统后补齐大型火球爆炸
    remove();
}

void FireballEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // TODO: 接入爆炸系统后补齐大型火球爆炸
    remove();
}

SmallFireballEntity::SmallFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(5.0f);
}

std::unique_ptr<Entity> SmallFireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SmallFireballEntity>(LegacyEntityType::Unknown, 0);
}

void SmallFireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Fireball, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Fireball, this, this, false);
    }

    (void)damageSource;
    // TODO: 接入 LivingEntity::hurt 后补齐小火球直接伤害
    if (!result.hitEntity->isOnFire()) {
        result.hitEntity->setFire(5);
    }

    remove();
}

void SmallFireballEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // TODO: 接入可燃方块逻辑后补齐点火行为
    remove();
}

DragonFireballEntity::DragonFireballEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(12.0f);
}

std::unique_ptr<Entity> DragonFireballEntity::create(IWorld* /*world*/)
{
    return std::make_unique<DragonFireballEntity>(LegacyEntityType::Unknown, 0);
}

void DragonFireballEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    // TODO: 接入龙息伤害与 AreaEffectCloudEntity
    remove();
}

void DragonFireballEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // TODO: 接入龙息云生成
    remove();
}

WitherSkullEntity::WitherSkullEntity(LegacyEntityType type, EntityId id)
    : AbstractFireballEntity(type, id)
{
    setDamage(8.0f);
}

std::unique_ptr<Entity> WitherSkullEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherSkullEntity>(LegacyEntityType::Unknown, 0);
}

void WitherSkullEntity::onEntityHit(const RayTraceResult& result)
{
    if (result.hitEntity == nullptr) {
        return;
    }

    IWorld* worldPtr = world();

    // MC 1.16.5: 凋灵之首造成伤害
    Entity* shooter = getShooter();
    auto damageSource = std::make_unique<IndirectEntityDamageSource>(
        DamageType::Magic,
        shooter != nullptr ? shooter : this,
        this,
        false);
    damageSource->setMagicDamage();

    // MC 1.16.5: 对 LivingEntity 造成伤害并施加凋零效果
    LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(result.hitEntity);
    if (livingTarget != nullptr) {
        // 造成直接伤害
        // TODO: 调用 livingTarget->hurt(*damageSource, getDamage())

        // MC 1.16.5: 施加凋零效果
        // 普通难度：凋零 II 10 秒（200 ticks）
        // 困难难度：凋零 II 40 秒（800 ticks）
        // 简单难度：无凋零效果
        // TODO: 根据世界难度调整持续时间
        // 当前使用普通难度（10秒）
        entity::effect::EffectInstance witherEffect(
            entity::effect::EffectType::Wither, // 凋零效果
            200,                                 // 持续时间（ticks）
            1                                    // 等级（II级 = 1，因为 0 = I级）
        );
        livingTarget->addEffect(std::move(witherEffect));
    }

    // MC 1.16.5: 凋灵之首爆炸半径 1.0
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首破坏更多方块类型（爆炸抗性更高的方块）
        // TODO: 蓝色凋灵之首有特殊的方块破坏规则
        (void)m_blue; // 暂时忽略蓝色凋灵之首的特殊处理

        worldPtr->createExplosion(
            result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS, // 1.0f
            mode,
            false, // 不生成火焰
            shooter);
    }

    remove();
}

void WitherSkullEntity::onBlockHit(const RayTraceResult& result)
{
    IWorld* worldPtr = world();
    Entity* shooter = getShooter();

    // MC 1.16.5: 凋灵之首在方块上爆炸
    if (worldPtr != nullptr) {
        world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
            ? world::explosion::ExplosionMode::Destroy
            : world::explosion::ExplosionMode::None;

        // 蓝色凋灵之首破坏更多方块类型
        (void)m_blue;

        worldPtr->createExplosion(
            result.hitPosition,
            game::explosion::WITHER_SKULL_RADIUS, // 1.0f
            mode,
            false, // 不生成火焰
            shooter);
    }

    remove();
}

f32 WitherSkullEntity::getMotionFactor() const
{
    // MC 1.16.5: 蓝色凋灵之首运动因子为 0.73，普通为 0.95
    return m_blue ? 0.73f : 0.95f;
}

bool WitherSkullEntity::isFiery() const
{
    // MC 1.16.5: 凋灵之首不燃烧
    return false;
}

} // namespace entity
} // namespace mc
