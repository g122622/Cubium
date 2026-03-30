#include "EndermanEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

EndermanEntity::EndermanEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> EndermanEntity::create(IWorld* /*world*/) {
    return std::make_unique<EndermanEntity>(LegacyEntityType::Unknown, 0);
}

bool EndermanEntity::teleport() {
    if (m_teleportCooldown > 0) {
        return false;
    }

    // TODO: 执行瞬移
    // 随机选择一个位置并瞬移

    m_teleportCooldown = TELEPORT_COOLDOWN;
    return true;
}

bool EndermanEntity::teleportToTarget() {
    if (m_teleportCooldown > 0) {
        return false;
    }

    // TODO: 瞬移到目标附近

    m_teleportCooldown = TELEPORT_COOLDOWN;
    return true;
}

bool EndermanEntity::teleportAwayFromWater() {
    return teleport();
}

void EndermanEntity::placeHeldBlock() {
    if (!m_holdingBlock) {
        return;
    }

    // TODO: 放置方块
    m_holdingBlock = false;
    m_heldBlock = 0;
}

void EndermanEntity::pickUpBlock() {
    // TODO: 拾取方块
    // m_heldBlock = ...;
    // m_holdingBlock = true;
}

bool EndermanEntity::isInWater() const {
    // TODO: 检查是否在水中
    // return world()->containsLiquid(getBoundingBox());
    return false;
}

void EndermanEntity::tick() {
    MonsterEntity::tick();

    // 更新瞬移冷却
    if (m_teleportCooldown > 0) {
        m_teleportCooldown--;
    }

    // 更新愤怒时间
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
            m_provoked = false;
        }
    }

    // 检查水伤害
    if (isInWater()) {
        // TODO: 受到水伤害
        // damage(DamageSource::DROWN, WATER_DAMAGE);
        teleportAwayFromWater();
    }

    // 搬方块行为
    // TODO: 随机搬起/放下方块
}

void EndermanEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 末影人 AI 目标
    // - EndermanAttackGoal: 攻击目标
    // - EndermanTeleportGoal: 瞬移
    // - EndermanPlaceBlockGoal: 放置方块
    // - EndermanPickupBlockGoal: 拾取方块
}

void EndermanEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 末影人的属性
    // 参考 MC 1.16.5 末影人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

} // namespace mc
