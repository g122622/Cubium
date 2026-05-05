#include "WaterMobEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../physics/PhysicsConstants.hpp"

namespace mc {

WaterMobEntity::WaterMobEntity(LegacyEntityType type, EntityId id)
    : CreatureEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

bool WaterMobEntity::isInWater() const {
    // 水生生物使用基类的 isInWater() 实现
    // 基类会检查 m_inWater 标志，该标志在 updateEnvironmentState() 中更新
    return Entity::isInWater();
}

bool WaterMobEntity::isInWaterOrBubble() const {
    // 检查是否在水中或气泡柱中
    // TODO: 添加气泡柱检测 (依赖: Blocks::BUBBLE_COLUMN 实现)
    return isInWater();
}

void WaterMobEntity::tick() {
    CreatureEntity::tick();

    // 检测水状态变化并触发回调
    bool inWater = isInWater();
    if (inWater && !m_wasInWater) {
        onEnterWater();
    } else if (!inWater && m_wasInWater) {
        onLeaveWater();
    }
    m_wasInWater = inWater;

    // 更新空气供应
    updateAirSupply();
}

void WaterMobEntity::registerAttributes() {
    // 调用父类方法
    CreatureEntity::registerAttributes();

    // 水生生物的基础属性
    // 参考 MC 1.16.5 水生生物属性
}

void WaterMobEntity::updateAirSupply() {
    bool inWater = isInWater();

    if (!inWater) {
        // 不在水中，消耗空气（水生生物在水外会窒息）
        m_airSupply--;
        if (m_airSupply <= -20) {
            m_airSupply = 0;
            // 溺水伤害
            m_drownDamageTimer++;
            if (m_drownDamageTimer >= physics::DROWN_DAMAGE_INTERVAL) {
                m_drownDamageTimer = 0;
                // 造成溺水伤害
                // 参考 MC 1.16.5 LivingEntity.baseTick()
                auto damageSource = DamageSources::drown();
                hurt(damageSource, DROWN_DAMAGE_AMOUNT);
            }
        }
    } else {
        // 在水中，恢复空气
        m_airSupply = m_maxAirSupply;
        m_drownDamageTimer = 0;
    }
}

} // namespace mc
