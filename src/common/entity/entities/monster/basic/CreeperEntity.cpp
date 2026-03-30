#include "CreeperEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

CreeperEntity::CreeperEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CreeperEntity::create(IWorld* /*world*/) {
    return std::make_unique<CreeperEntity>(LegacyEntityType::Unknown, 0);
}

void CreeperEntity::explode() {
    // TODO: 在当前位置创建爆炸
    // world()->createExplosion(position(), getExplosionPower(), this);

    // 移除实体
    // remove();
}

void CreeperEntity::tick() {
    MonsterEntity::tick();

    // 更新膨胀状态
    m_oldSwell = m_swell;

    if (m_ignited) {
        // 被点燃时，膨胀增加
        m_swell++;
        if (m_swell >= MAX_SWELL) {
            explode();
            return;
        }
    } else if (m_swell > 0) {
        // 未点燃时，膨胀减少
        m_swell--;
    }

    // 重置点燃状态
    m_ignited = false;
}

void CreeperEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 苦力怕 AI 目标
    // - CreeperSwellGoal: 膨胀爆炸
    // - FleeSunGoal: 避开阳光（不燃烧但要避开）
    // - AvoidEntityGoal: 避开猫
}

void CreeperEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 苦力怕的属性
    // 参考 MC 1.16.5 苦力怕属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
