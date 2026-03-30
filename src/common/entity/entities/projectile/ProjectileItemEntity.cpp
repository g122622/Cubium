#include "ProjectileItemEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../../item/Items.hpp"
#include "../../../damage/DamageSource.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ============================================================================
// ProjectileItemEntity
// ============================================================================

ProjectileItemEntity::ProjectileItemEntity(LegacyEntityType type, EntityId id)
    : ThrowableEntity(type, id)
{
}

void ProjectileItemEntity::tick() {
    ThrowableEntity::tick();

    // TODO: 在水中生成气泡粒子
}

// ============================================================================
// SnowballEntity
// ============================================================================

SnowballEntity::SnowballEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> SnowballEntity::create(IWorld* /*world*/) {
    return std::make_unique<SnowballEntity>(LegacyEntityType::Unknown, 0);
}

const Item* SnowballEntity::getDefaultItem() const {
    return Items::SNOWBALL;
}

void SnowballEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 对烈焰人造成额外伤害
    // TODO: 检查是否是烈焰人
    i32 damage = 0;  // result.hitEntity->type() == EntityType::Blaze ? 3 : 0;

    if (damage > 0) {
        mc::Entity* shooter = getShooter();
        std::unique_ptr<DamageSource> damageSource;
        if (shooter) {
            damageSource = std::make_unique<IndirectEntityDamageSource>(
                DamageType::MobProjectile, shooter, this, false);
        } else {
            damageSource = std::make_unique<IndirectEntityDamageSource>(
                DamageType::MobProjectile, this, this, false);
        }

        // TODO: target->attackEntityFrom(*damageSource, damage);
    }
}

void SnowballEntity::onImpact(const RayTraceResult& result) {
    // 播放破裂粒子效果
    // world->playEvent(Event::SNOWBALL_POOF, pos, 0);

    remove();
}

// ============================================================================
// EggEntity
// ============================================================================

EggEntity::EggEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> EggEntity::create(IWorld* /*world*/) {
    return std::make_unique<EggEntity>(LegacyEntityType::Unknown, 0);
}

const Item* EggEntity::getDefaultItem() const {
    return Items::EGG;
}

void EggEntity::onEntityHit(const RayTraceResult& result) {
    if (!result.hitEntity) {
        return;
    }

    // 造成伤害（非常小）
    mc::Entity* shooter = getShooter();
    std::unique_ptr<DamageSource> damageSource;
    if (shooter) {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, shooter, this, false);
    } else {
        damageSource = std::make_unique<IndirectEntityDamageSource>(
            DamageType::MobProjectile, this, this, false);
    }

    // TODO: target->attackEntityFrom(*damageSource, 0.0f);
}

void EggEntity::onImpact(const RayTraceResult& /*result*/) {
    // 12.5% 概率孵化小鸡
    if (tryHatchChicken()) {
        // 孵化成功，不播放破裂效果
    } else {
        // 播放破裂粒子效果
        // world->playEvent(Event::EGG_POOF, pos, 0);
    }

    remove();
}

bool EggEntity::tryHatchChicken() {
    // 12.5% (1/8) 概率孵化
    // rand.nextInt(8) == 0
    // TODO: 生成小鸡
    return false;
}

// ============================================================================
// EnderPearlEntity
// ============================================================================

EnderPearlEntity::EnderPearlEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> EnderPearlEntity::create(IWorld* /*world*/) {
    return std::make_unique<EnderPearlEntity>(LegacyEntityType::Unknown, 0);
}

const Item* EnderPearlEntity::getDefaultItem() const {
    return Items::ENDER_PEARL;
}

void EnderPearlEntity::onEntityHit(const RayTraceResult& result) {
    mc::Entity* shooter = getShooter();

    // 对发射者造成5点伤害（传送伤害）
    if (shooter && shooter->isAlive()) {
        // TODO: shooter->attackEntityFrom(DamageSources::fall(), 5.0f);
    }
}

void EnderPearlEntity::onImpact(const RayTraceResult& result) {
    mc::Entity* shooter = getShooter();

    // 传送发射者
    if (shooter && shooter->isAlive()) {
        // 检查发射者是否是玩家或生物
        LivingEntity* livingShooter = dynamic_cast<LivingEntity*>(shooter);
        if (livingShooter) {
            // 传送到落点
            // 如果命中实体，传送实体会受到伤害
            if (result.type == RayTraceResultType::Entity && result.hitEntity) {
                // 不能传送到其他实体内部
            } else {
                // 传送
                shooter->setPosition(result.hitPosition.x,
                                     result.hitPosition.y,
                                     result.hitPosition.z);
                // 造成摔落伤害
                // shooter->attackEntityFrom(DamageSources::fall(), 5.0f);
            }
        }
    }

    remove();
}

// ============================================================================
// PotionEntity
// ============================================================================

PotionEntity::PotionEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> PotionEntity::create(IWorld* /*world*/) {
    return std::make_unique<PotionEntity>(LegacyEntityType::Unknown, 0);
}

const Item* PotionEntity::getDefaultItem() const {
    // TODO: 返回默认药水物品
    return nullptr;
}

void PotionEntity::onImpact(const RayTraceResult& /*result*/) {
    // TODO: 应用药水效果
    // 1. 获取药水效果列表
    // 2. 影响范围内的生物
    // 3. 如果是滞留型，创建区域效果云

    remove();
}

// ============================================================================
// ExperienceBottleEntity
// ============================================================================

ExperienceBottleEntity::ExperienceBottleEntity(LegacyEntityType type, EntityId id)
    : ProjectileItemEntity(type, id)
{
}

std::unique_ptr<Entity> ExperienceBottleEntity::create(IWorld* /*world*/) {
    return std::make_unique<ExperienceBottleEntity>(LegacyEntityType::Unknown, 0);
}

const Item* ExperienceBottleEntity::getDefaultItem() const {
    return Items::EXPERIENCE_BOTTLE;
}

void ExperienceBottleEntity::onImpact(const RayTraceResult& /*result*/) {
    // 生成经验球
    // 经验值 = 3-11 随机
    // TODO: 生成经验球实体

    // 播放破裂效果
    // world->playEvent(Event::EXPERIENCE_BOTTLE_POOF, pos, 0);

    remove();
}

} // namespace entity
} // namespace mc
