#include "AbstractFishEntity.hpp"

#include "../../../attribute/Attributes.hpp"

namespace mc {

AbstractFishEntity::AbstractFishEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    // 设置鱼类最大空气供应量（480 ticks = 24秒）
    setAir(maxAir());

    registerGoals();
    registerAttributes();
}

void AbstractFishEntity::tick()
{
    WaterMobEntity::tick();
    updateSwimming();
    updateFlopping();
}

void AbstractFishEntity::registerGoals()
{
    // TODO: 对齐 1.16.5 的 PanicGoal、AvoidEntityGoal 和 RandomSwimmingGoal
    // 依赖: AI Goal 系统完善
}

void AbstractFishEntity::registerAttributes()
{
    WaterMobEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void AbstractFishEntity::updateSwimming()
{
    if (isInWater()) {
        m_swimming = true;
        m_flopping = false;
        return;
    }

    m_swimming = false;
    m_flopping = true;
}

void AbstractFishEntity::updateFlopping()
{
    if (isInWater()) {
        m_flopTimer = 0;
        m_flopping = false;
        return;
    }

    ++m_flopTimer;
    if (m_flopTimer >= 100) {
        // MC 1.16.5: 离水扑腾逻辑
        // 鱼会随机跳跃并播放 flop 声音
        // TODO: 实现跳跃和声音 (依赖: 完善的 motion 系统和 sound event)
        m_flopTimer = 0;
    }
}

} // namespace mc
