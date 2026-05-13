#include "OcelotEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"
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
    // MC 1.16.5: 豹猫使用生鳕鱼和生鲑鱼繁殖
    // BREEDING_ITEMS = Ingredient.fromItems(Items.COD, Items.SALMON)
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::COD || item == Items::SALMON;
}

std::unique_ptr<AnimalEntity> OcelotEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // MC 1.16.5: OcelotEntity.func_241840_a (createChild)
    // 创建一个新的豹猫实体，不需要继承父母特征
    auto baby = std::make_unique<OcelotEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void OcelotEntity::tick() {
    AnimalEntity::tick();

    // 如果已建立信任，停止逃跑
    if (m_trusting) {
        m_fleeing = false;
    }
}

void OcelotEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 豹猫特有目标
    // 优先级 3: 食物诱惑（生鱼）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, FOLLOW_SPEED, isFishPredicate));

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
