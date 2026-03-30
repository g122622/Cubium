#include "AbstractFishEntity.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

AbstractFishEntity::AbstractFishEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    // 设置最大空气供应
    setMaxAirSupply(MAX_AIR_SUPPLY);
    setAirSupply(MAX_AIR_SUPPLY);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

void AbstractFishEntity::tick() {
    WaterMobEntity::tick();

    // 更新游泳状态
    updateSwimming();

    // 更新扑腾状态
    updateFlopping();
}

void AbstractFishEntity::registerGoals() {
    // TODO: 鱼类 AI 目标
    // - FishSwimGoal: 随机游泳
    // - FishPanicGoal: 恐慌逃跑
    // - FishAvoidPlayerGoal: 避开玩家
    // - SchoolingGoal: 群游行为（可选）
}

void AbstractFishEntity::registerAttributes() {
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 鱼类的基础属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void AbstractFishEntity::updateSwimming() {
    if (isInWater()) {
        m_swimming = true;
        m_flopping = false;
    } else {
        m_swimming = false;
        m_flopping = true;
    }
}

void AbstractFishEntity::updateFlopping() {
    if (!isInWater()) {
        // 在陆地上扑腾
        m_flopTimer++;
        if (m_flopTimer >= 100) {
            // 每隔一段时间跳跃
            // TODO: 实现跳跃逻辑
            m_flopTimer = 0;
        }
    } else {
        m_flopTimer = 0;
        m_flopping = false;
    }
}

} // namespace mc
