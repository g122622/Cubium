#include "StriderEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/PanicGoal.hpp"
#include "../../ai/goal/goals/BreedGoal.hpp"
#include "../../ai/goal/goals/TemptGoal.hpp"
#include "../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../attribute/Attributes.hpp"
#include <random>

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

void StriderEntity::onPlayerStartRiding(Player* /*player*/) {
    m_isBeingRidden = true;
    // TODO: 设置骑乘者ID
}

void StriderEntity::onPlayerStopRiding(Player* /*player*/) {
    m_isBeingRidden = false;
    m_riderId = 0;
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
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dist(BOOST_DURATION_MIN, BOOST_DURATION_MAX);
        m_boostTime = dist(gen);
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
    // 调用父类方法
    AnimalEntity::registerGoals();

    // 优先级 0: 游泳（在熔岩中）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（诡异菌）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isWarpedFungusPredicate));

    // 优先级 4: 跟随父母
    // m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 0.5));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

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
