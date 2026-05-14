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

#include "../../../world/IWorld.hpp"
#include "../../damage/DamageSource.hpp"

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

    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter != nullptr) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Magic, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(DamageType::Magic, this, this, false);
    }

    (void)damageSource;
    // TODO: 接入凋灵之首伤害、凋灵效果与爆炸
    remove();
}

void WitherSkullEntity::onBlockHit(const RayTraceResult& /*result*/)
{
    // TODO: 接入凋灵之首爆炸与蓝头破坏方块逻辑
    remove();
}

} // namespace entity
} // namespace mc
