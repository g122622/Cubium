#include "SquidEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

SquidEntity::SquidEntity(LegacyEntityType type, EntityId id)
    : WaterMobEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SquidEntity::create(IWorld* /*world*/) {
    return std::make_unique<SquidEntity>(LegacyEntityType::Unknown, 0);
}

void SquidEntity::sprayInk() {
    if (!m_sprayingInk) {
        m_sprayingInk = true;
        m_sprayTimer = SPRAY_INK_DURATION;
        // TODO: 生成墨汁粒子
    }
}

void SquidEntity::tick() {
    WaterMobEntity::tick();

    // 更新喷墨计时器
    if (m_sprayingInk && m_sprayTimer > 0) {
        m_sprayTimer--;
        if (m_sprayTimer <= 0) {
            m_sprayingInk = false;
        }
    }

    // 更新游泳行为
    if (isInWater()) {
        m_swimTimer++;
        m_changeDirectionTimer++;

        // 随机改变方向
        if (m_changeDirectionTimer >= 100) {
            math::Random rng = getRandom();
            m_targetSwimAngle = rng.nextFloat(0.0f, 360.0f);
            m_changeDirectionTimer = 0;
        }

        // 平滑转向
        f32 angleDiff = m_targetSwimAngle - m_swimAngle;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;
        m_swimAngle += angleDiff * 0.1f;

        // 游泳推进
        if (m_swimTimer >= SWIM_DURATION) {
            m_swimming = false;
            m_swimTimer = 0;
        }
    } else {
        // 在陆地上扑腾
        m_swimming = false;
    }
}

void SquidEntity::registerGoals() {
    // TODO: 鱿鱼 AI 目标
    // - SquidSwimGoal: 随机游泳
    // - SquidFleeGoal: 逃跑
}

void SquidEntity::registerAttributes() {
    // 调用父类方法
    WaterMobEntity::registerAttributes();

    // 鱿鱼的属性
    // 参考 MC 1.16.5 鱿鱼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
