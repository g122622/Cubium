#include "WaterMobEntity.hpp"
#include "../../../world/World.hpp"
#include "../../attribute/Attributes.hpp"

namespace mc {

WaterMobEntity::WaterMobEntity(LegacyEntityType type, EntityId id)
    : LivingEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

bool WaterMobEntity::isInWater() const {
    // TODO: 检查是否在水中
    // return isInWaterState();
    return false;
}

bool WaterMobEntity::isInWaterOrBubble() const {
    // TODO: 检查是否在水中或气泡柱中
    // return isInWater() || isInBubbleColumn();
    return isInWater();
}

void WaterMobEntity::tick() {
    LivingEntity::tick();

    // 更新空气供应
    updateAirSupply();
}

void WaterMobEntity::registerAttributes() {
    // 调用父类方法
    LivingEntity::registerAttributes();

    // 水生生物的基础属性
    // 参考 MC 1.16.5 水生生物属性
}

void WaterMobEntity::updateAirSupply() {
    bool wasInWater = isInWater();

    if (!wasInWater) {
        // 不在水中，消耗空气
        m_airSupply--;
        if (m_airSupply <= -20) {
            m_airSupply = 0;
            // 溺水伤害
            m_drownDamageTimer++;
            if (m_drownDamageTimer >= DROWN_DAMAGE_INTERVAL) {
                m_drownDamageTimer = 0;
                // TODO: 造成伤害
                // damage(DamageSource::DROWN, DROWN_DAMAGE_AMOUNT);
            }
        }
    } else {
        // 在水中，恢复空气
        m_airSupply = m_maxAirSupply;
        m_drownDamageTimer = 0;
    }
}

} // namespace mc
