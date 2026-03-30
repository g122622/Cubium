#include "OcelotEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/PanicGoal.hpp"
#include "../../ai/goal/goals/BreedGoal.hpp"
#include "../../ai/goal/goals/TemptGoal.hpp"
#include "../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../attribute/Attributes.hpp"
#include <random>
#include <unordered_set>

namespace mc {

OcelotEntity::OcelotEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> OcelotEntity::create(IWorld* /*world*/) {
    return std::make_unique<OcelotEntity>(LegacyEntityType::Unknown, 0);
}

bool OcelotEntity::trustsPlayer(u64 playerId) const {
    return m_trusting && m_trustingPlayerId == playerId;
}

void OcelotEntity::setPlayerTrust(u64 playerId, bool trust) {
    if (trust && !m_trusting) {
        m_trusting = true;
        m_trustingPlayerId = playerId;
    }
}

void OcelotEntity::setTrusting(bool trusting) {
    m_trusting = trusting;
}

bool OcelotEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是生鱼
    // return itemStack.getItem() == Items::COD ||
    //        itemStack.getItem() == Items::SALMON;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> OcelotEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小豹猫
    // auto baby = std::make_unique<OcelotEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    (void)partner;
    return nullptr;
}

void OcelotEntity::tick() {
    AnimalEntity::tick();

    // 如果已建立信任，停止逃跑
    if (m_trusting) {
        m_fleeing = false;
    }
}

void OcelotEntity::registerGoals() {
    // 调用父类方法
    AnimalEntity::registerGoals();

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, FLEE_SPEED));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（生鱼）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, FOLLOW_SPEED, isFishPredicate));

    // 优先级 4: 跟随父母
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, FOLLOW_SPEED));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 0.6));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

    // TODO: 豹猫特有目标
    // - OcelotFleeGoal: 逃离玩家（未信任时）
    // - OcelotTrustGoal: 建立信任
    // - OcelotHuntGoal: 狩猎小鸡和海龟
}

void OcelotEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 豹猫的属性
    // 参考 MC 1.16.5 豹猫属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
