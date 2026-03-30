#include "AbstractHorseEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {

AbstractHorseEntity::AbstractHorseEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 初始化随机属性
    initRandomAttributes();
}

void AbstractHorseEntity::setSaddle(bool saddle) {
    m_saddled = saddle;
    // 标记实体数据需要同步
    // TODO: dataManager.set(FLAG_SADDLE, saddle);
}

void AbstractHorseEntity::onPlayerStartRiding(Player* player) {
    m_rider = player;
    // TODO: 设置乘客位置
}

void AbstractHorseEntity::onPlayerStopRiding(Player* player) {
    m_rider = nullptr;
    // TODO: 移除乘客
}

f32 AbstractHorseEntity::getSteeringSpeed() const {
    if (!m_saddled) return 0.0f;

    f32 speed = getSpeed();

    // 加速时增加速度
    if (m_isBoosting) {
        speed *= 1.5f;
    }

    return speed;
}

bool AbstractHorseEntity::boost() {
    if (!m_saddled || m_boostTime > 0) {
        return false;
    }

    // 开始加速
    m_isBoosting = true;
    m_boostTime = MAX_BOOST_TIME;
    return true;
}

void AbstractHorseEntity::onJump() {
    if (!m_saddled || !m_isJumping) {
        return;
    }

    performJump();
}

void AbstractHorseEntity::setJumpPower(f32 power) {
    m_jumpPower = std::clamp(power, 0.0f, 1.0f);
}

f32 AbstractHorseEntity::getMaxJumpHeight() const {
    // 根据跳跃强度计算最大跳跃高度
    // MC 公式: 0.6 * jumpStrength^2 + 0.1 * jumpStrength + 0.3
    return 0.6f * m_jumpStrength * m_jumpStrength + 0.1f * m_jumpStrength + 0.3f;
}

bool AbstractHorseEntity::canJump() const {
    return m_saddled && m_jumpCooldown <= 0;
}

void AbstractHorseEntity::startJumping() {
    if (!canJump()) {
        return;
    }

    m_isJumping = true;
    m_jumpPower = 0.0f;
}

void AbstractHorseEntity::stopJumping() {
    if (!m_isJumping) {
        return;
    }

    // 执行跳跃
    performJump();
    m_isJumping = false;
    m_jumpPower = 0.0f;
    m_jumpCooldown = 10; // 跳跃冷却
}

bool AbstractHorseEntity::isBeingRidden() const {
    return m_rider != nullptr;
}

bool AbstractHorseEntity::canBeRiddenBy(Player* player) const {
    // 已被骑乘或未驯服
    if (m_rider != nullptr) {
        return false;
    }

    // 需要驯服才能骑乘（子类可覆盖此逻辑）
    if (!m_tame) {
        return false;
    }

    return true;
}

void AbstractHorseEntity::setTame(bool tame) {
    m_tame = tame;
    // TODO: 数据同步
}

bool AbstractHorseEntity::increaseTemper(i32 amount) {
    m_temper += amount;

    if (m_temper >= m_maxTemper) {
        // 达到驯服阈值
        m_temper = m_maxTemper;
        setTame(true);
        return true;
    }

    return false;
}

bool AbstractHorseEntity::isTameItem(const ItemStack& itemStack) const {
    // 默认情况下，马不响应任何驯服物品
    // 子类应覆盖此方法
    (void)itemStack;
    return false;
}

f32 AbstractHorseEntity::getSpeed() const {
    return m_speed;
}

void AbstractHorseEntity::tick() {
    AnimalEntity::tick();

    // 更新跳跃冷却
    if (m_jumpCooldown > 0) {
        m_jumpCooldown--;
    }

    // 更新跳跃蓄力
    if (m_isJumping) {
        updateJumpPower();
    }

    // 更新加速状态
    updateBoost();

    // 更新骑乘状态
    updateRiding();
}

void AbstractHorseEntity::registerAttributes() {
    AnimalEntity::registerAttributes();

    // 马类基础属性
    m_attributes.registerAttribute(*entity::attribute::Attributes::horseJumpStrength());
    m_attributes.setBaseValue(entity::attribute::Attributes::JUMP_STRENGTH, m_jumpStrength);

    // 设置生命值和速度
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
}

void AbstractHorseEntity::updateRiding() {
    // TODO: 更新骑乘者位置
    // TODO: 处理骑乘者控制
}

void AbstractHorseEntity::updateJumpPower() {
    if (m_jumpPower < 1.0f) {
        // 蓄力增加
        m_jumpPower += 0.05f;
        m_jumpPower = std::min(m_jumpPower, 1.0f);
    }
}

void AbstractHorseEntity::performJump() {
    if (!canJump() || m_jumpPower <= 0.0f) {
        return;
    }

    // 计算跳跃力度
    f32 jumpForce = m_jumpStrength * m_jumpPower;

    // 设置垂直速度
    setVelocity(velocityX(), jumpForce, velocityZ());

    // 设置跳跃冷却
    m_jumpCooldown = 10;
}

void AbstractHorseEntity::updateBoost() {
    if (m_boostTime > 0) {
        m_boostTime--;

        if (m_boostTime <= 0) {
            m_isBoosting = false;
        }
    }
}

void AbstractHorseEntity::initRandomAttributes() {
    math::Random rng(ticksExisted());

    // 随机生成马特有属性
    m_speed = rng.nextFloat(MIN_SPEED, MAX_SPEED);
    m_jumpStrength = rng.nextFloat(MIN_JUMP, MAX_JUMP);
    m_horseHealth = rng.nextFloat(MIN_HEALTH, MAX_HEALTH);
}

} // namespace mc
