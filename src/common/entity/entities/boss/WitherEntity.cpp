#include "WitherEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include "../projectile/AbstractFireballEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

std::unique_ptr<Entity> WitherEntity::create(IWorld* /*world*/) {
    return std::make_unique<WitherEntity>(LegacyEntityType::Unknown, 0);
}

WitherEntity::WitherEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    registerAttributes();
    registerGoals();
}

void WitherEntity::tick() {
    MobEntity::tick();

    // 根据阶段更新
    switch (m_phase) {
        case Phase::Invulnerable:
            updateInvulnerablePhase();
            break;

        case Phase::Charging:
            updateChargingPhase();
            break;

        case Phase::Attacking:
            updateAttackingPhase();
            break;
    }

    // 更新冷却
    if (m_mainHeadCooldown > 0) m_mainHeadCooldown--;
    if (m_leftHeadCooldown > 0) m_leftHeadCooldown--;
    if (m_rightHeadCooldown > 0) m_rightHeadCooldown--;

    // 凋灵效果光环
    // TODO: 对周围玩家/生物施加凋零效果
}

bool WitherEntity::isInvulnerableTo(DamageSource& source) const {
    // 凋灵免疫火焰、溺水、凋零伤害
    switch (source.type()) {
        case DamageType::OnFire:
        case DamageType::Lava:
        case DamageType::Drown:
        case DamageType::Wither:
            return true;
        default:
            // 无敌阶段免疫所有伤害
            return m_phase == Phase::Invulnerable;
    }
}

void WitherEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge) {
    shootWitherSkull(target, false);
}

void WitherEntity::shootWitherSkull(LivingEntity* target, bool isBlue) {
    if (!target || !target->isAlive()) {
        return;
    }

    // 计算发射方向
    f32 dx = target->x() - m_position.x;
    f32 dy = target->y() + target->eyeHeight() - (m_position.y + 1.0f);
    f32 dz = target->z() - m_position.z;

    // 归一化
    f32 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > 0.0f) {
        dx /= dist;
        dy /= dist;
        dz /= dist;
    }

    // TODO: 创建凋灵之首
    // auto skull = std::make_unique<WitherSkullEntity>(LegacyEntityType::Unknown, id);
    // skull->setPosition(m_position.x, m_position.y + 1.0f, m_position.z);
    // skull->setShooter(this);
    // skull->shoot(dx, dy, dz, 1.5f, 0.0f);
    // skull->setBlue(isBlue);
    // world->spawnEntity(std::move(skull));

    m_skullsSpawned++;
}

void WitherEntity::explodeOnSpawn() {
    // TODO: 在生成位置创建爆炸
    // world->createExplosion(this, x(), y(), z(), 7.0f, ...);
}

void WitherEntity::updateInvulnerablePhase() {
    m_invulnerableTime++;

    // 无敌持续220 ticks (11秒)
    if (m_invulnerableTime >= 220) {
        m_phase = Phase::Attacking;
        // 触发爆炸
        explodeOnSpawn();
    }

    // 恢复生命值
    if (health() < maxHealth()) {
        heal(1.0f);  // 每tick恢复1点
    }
}

void WitherEntity::updateChargingPhase() {
    m_chargeTime++;

    // 充能持续20 ticks (1秒)
    if (m_chargeTime >= 20) {
        m_chargeTime = 0;
        m_phase = Phase::Attacking;
    }
}

void WitherEntity::updateAttackingPhase() {
    // 更新头部目标
    updateHeadTargets();

    // 主头发射凋灵之首
    if (m_headTarget && m_mainHeadCooldown <= 0) {
        shootWitherSkull(m_headTarget, false);
        m_mainHeadCooldown = 100;  // 5秒冷却
    }

    // 左侧头发射凋灵之首
    if (m_leftHeadTarget && m_leftHeadCooldown <= 0) {
        shootWitherSkull(m_leftHeadTarget, false);
        m_leftHeadCooldown = 80;  // 4秒冷却
    }

    // 右侧头发射凋灵之首
    if (m_rightHeadTarget && m_rightHeadCooldown <= 0) {
        shootWitherSkull(m_rightHeadTarget, false);
        m_rightHeadCooldown = 80;  // 4秒冷却
    }

    // 生命值低于一半时，充能发射蓝色凋灵之首
    if (health() < maxHealth() * 0.5f) {
        // 充能状态
        if (m_chargeTime <= 0) {
            m_phase = Phase::Charging;
            m_chargeTime = 0;
        }
    }
}

void WitherEntity::updateHeadTargets() {
    // TODO: 为每个头寻找独立目标
    // 主头追踪最近的玩家或生物
    // 左右头追踪其他目标

    // 主头目标
    m_headTarget = m_attackTarget;

    // 左右头寻找其他目标
    // TODO: 实现目标搜索
    m_leftHeadTarget = nullptr;
    m_rightHeadTarget = nullptr;
}

void WitherEntity::registerGoals() {
    MobEntity::registerGoals();

    // TODO: 添加凋灵AI目标
    // - 攻击最近玩家
    // - 攻击非亡灵生物
    // - 充能攻击
}

void WitherEntity::registerAttributes() {
    MobEntity::registerAttributes();

    // 凋灵属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 300.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);

    // 凋灵可以飞行
    // setNoGravity(true);
}

} // namespace entity
} // namespace mc
