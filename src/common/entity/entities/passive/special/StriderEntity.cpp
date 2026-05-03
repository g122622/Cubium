#include "StriderEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc {

StriderEntity::StriderEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> StriderEntity::create(IWorld* /*world*/) {
    return std::make_unique<StriderEntity>(LegacyEntityType::Unknown, 0);
}

bool StriderEntity::isInLava() const {
    // TODO: 检查是否在熔岩中
    // return isInLavaState();
    return false;
}

void StriderEntity::setSaddle(bool saddle) {
    m_saddled = saddle;
}

f32 StriderEntity::getSteeringSpeed() const {
    f32 baseSpeed = static_cast<f32>(m_attributes.getValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    if (m_boosting) {
        return baseSpeed + BOOST_SPEED;
    }
    return baseSpeed;
}

bool StriderEntity::boost() {
    if (m_boostCooldown <= 0 && !m_boosting) {
        m_boosting = true;
        math::Random rng = getRandom();
        m_boostTime = rng.nextInt(BOOST_DURATION_MIN, BOOST_DURATION_MAX);
        m_boostCooldown = m_boostTime + 40; // 冷却时间
        return true;
    }
    return false;
}

bool StriderEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是诡异菌
    // return itemStack.getItem() == Items::WARPED_FUNGUS;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> StriderEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小炽足兽
    // auto baby = std::make_unique<StriderEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    (void)partner;
    return nullptr;
}

void StriderEntity::tick() {
    AnimalEntity::tick();

    // 更新寒冷计时器
    if (!isInLava()) {
        if (m_coldTimer < COLD_DURATION) {
            m_coldTimer++;
        }
    } else {
        m_coldTimer = 0;
    }

    // 更新加速状态
    if (m_boosting) {
        if (m_boostTime > 0) {
            m_boostTime--;
        }
        if (m_boostTime <= 0) {
            m_boosting = false;
        }
    }

    // 更新加速冷却
    if (m_boostCooldown > 0) {
        m_boostCooldown--;
    }
}

void StriderEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 炽足兽特有目标
    // 优先级 3: 食物诱惑（诡异菌）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isWarpedFungusPredicate));

    // TODO: 炽足兽特有目标
    // - StriderGoToLavaGoal: 前往熔岩
    // - StriderWalkOnLavaGoal: 在熔岩上行走
}

void StriderEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 炽足兽的属性
    // 参考 MC 1.16.5 炽足兽属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.15);
}

} // namespace mc
