#include "SlimeEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

SlimeEntity::SlimeEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 随机设置尺寸
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<i32> dist(1, 4);
    setSlimeSize(dist(gen));
}

std::unique_ptr<Entity> SlimeEntity::create(IWorld* /*world*/) {
    return std::make_unique<SlimeEntity>(LegacyEntityType::Unknown, 0);
}

void SlimeEntity::setSlimeSize(i32 size) {
    m_size = std::clamp(size, 1, 4);
    updateSizeAttributes();
}

void SlimeEntity::split() {
    if (!canSplit()) {
        return;
    }

    // TODO: 生成 2-4 个小史莱姆
    // i32 smallSize = m_size / 2;
    // for (int i = 0; i < 2 + rand() % 3; i++) {
    //     auto smallSlime = std::make_unique<SlimeEntity>(LegacyEntityType::Unknown, 0);
    //     smallSlime->setSlimeSize(smallSize);
    //     // 在附近生成
    // }
}

f32 SlimeEntity::eyeHeight() const {
    // 根据尺寸计算眼睛高度
    return static_cast<f32>(m_size) * 0.5f + 0.1f;
}

void SlimeEntity::tick() {
    MonsterEntity::tick();

    // 更新弹跳冷却
    if (m_jumpCooldown > 0) {
        m_jumpCooldown--;
    }

    // 更新攻击冷却
    if (m_attackCooldown > 0) {
        m_attackCooldown--;
    }

    // 弹跳行为
    if (!m_jumping && m_jumpCooldown <= 0 && isOnGround()) {
        // 随机弹跳
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dist(1, 100);
        if (dist(gen) == 1) {
            m_jumping = true;
            m_jumpTimer = 0;
            m_jumpCooldown = JUMP_COOLDOWN_MIN + (rand() % (JUMP_COOLDOWN_MAX - JUMP_COOLDOWN_MIN));
        }
    }

    // 更新弹跳计时器
    if (m_jumping) {
        m_jumpTimer++;
        if (m_jumpTimer >= 10) {
            m_jumping = false;
        }
    }
}

void SlimeEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 史莱姆 AI 目标
    // - SlimeAttackGoal: 攻击目标
    // - SlimeJumpGoal: 弹跳
    // - SlimeFaceRandomGoal: 随机转向
}

void SlimeEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 更新尺寸属性
    updateSizeAttributes();
}

void SlimeEntity::updateSizeAttributes() {
    // 根据尺寸更新属性
    // 尺寸 1: 1 HP, 尺寸 4: 16 HP
    f32 health = static_cast<f32>(m_size * m_size);
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, health);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2f + 0.1f / m_size);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, static_cast<f32>(m_size));
}

} // namespace mc
