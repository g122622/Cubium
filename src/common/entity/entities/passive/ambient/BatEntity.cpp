#include "BatEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

BatEntity::BatEntity(LegacyEntityType type, EntityId id)
    : AmbientEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 默认飞行
    m_flying = true;
}

std::unique_ptr<Entity> BatEntity::create(IWorld* /*world*/) {
    return std::make_unique<BatEntity>(LegacyEntityType::Unknown, 0);
}

bool BatEntity::canRest() const {
    // TODO: 检查上方是否有固体方块
    // BlockPos above = position().above();
    // BlockState state = world()->getBlockState(above);
    // return state.isSolid();
    return false;
}

void BatEntity::tick() {
    AmbientEntity::tick();

    // TODO: 检查是否是白天
    // bool isDay = world()->isDay();

    // 简化逻辑：随机决定是否休息
    if (m_flying && !m_resting) {
        m_flyTimer++;

        // 检查是否可以休息
        if (canRest()) {
            math::Random rng = getRandom();
            if (rng.nextInt(1, 100) == 1) {
                m_resting = true;
                m_flying = false;
            }
        }
    }

    // 休息状态更新
    if (m_resting) {
        // 保持静止
        // TODO: 检查是否应该唤醒
        // if (world()->isNight() || nearbyPlayer) {
        //     m_resting = false;
        //     m_flying = true;
        // }
    }
}

void BatEntity::registerGoals() {
    // TODO: 蝙蝠 AI 目标
    // - BatFlyGoal: 随机飞行
    // - BatRestGoal: 休息
}

void BatEntity::registerAttributes() {
    // 调用父类方法
    AmbientEntity::registerAttributes();

    // 蝙蝠的属性
    // 参考 MC 1.16.5 蝙蝠属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, FLY_SPEED);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, FLY_SPEED);
}

} // namespace mc
