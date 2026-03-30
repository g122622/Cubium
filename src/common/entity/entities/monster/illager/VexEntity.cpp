#include "VexEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

VexEntity::VexEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 恼鬼体型小
}

std::unique_ptr<Entity> VexEntity::create(IWorld* /*world*/) {
    return std::make_unique<VexEntity>(LegacyEntityType::Unknown, 0);
}

void VexEntity::tick() {
    MonsterEntity::tick();

    // 更新生命时间
    if (m_limitedLife && m_lifeTime > 0) {
        m_lifeTime--;

        // 生命结束时开始闪烁
        if (m_lifeTime <= 100) {
            // TODO: 闪烁效果
        }

        if (m_lifeTime <= 0) {
            // 死亡
            // TODO: 使用伤害方法
        }
    }

    // 恼鬼可以穿墙
    // TODO: 设置 noclip 标志
}

void VexEntity::registerGoals() {
    MonsterEntity::registerGoals();

    // TODO: 恼鬼特有 AI 目标
    // - VexAttackGoal (近战攻击)
    // - VexChargeGoal (充电攻击)
    // - VexMoveGoal (穿墙移动)
}

void VexEntity::registerAttributes() {
    MonsterEntity::registerAttributes();

    // 恼鬼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 14.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3f);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 9.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 20.0f);
}

} // namespace mc
