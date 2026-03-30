#include "RabbitEntity.hpp"
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
#include "../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../attribute/Attributes.hpp"
#include <random>

namespace mc {

RabbitEntity::RabbitEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 随机设置皮肤类型
    setRandomRabbitType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> RabbitEntity::create(IWorld* /*world*/) {
    return std::make_unique<RabbitEntity>(LegacyEntityType::Unknown, 0);
}

void RabbitEntity::setRandomRabbitType() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // 杀手兔有极小概率生成（1/1000）
    std::uniform_int_distribution<int> killerDist(0, 999);
    if (killerDist(gen) == 0) {
        m_rabbitType = RabbitType::Killer;
        return;
    }

    // 正常皮肤随机
    std::uniform_int_distribution<u8> dist(0, 5);
    m_rabbitType = static_cast<RabbitType>(dist(gen));
}

bool RabbitEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是胡萝卜、金胡萝卜或蒲公英
    // return itemStack.getItem() == Items::CARROT
    //     || itemStack.getItem() == Items::GOLDEN_CARROT
    //     || itemStack.getItem() == Items::DANDELION;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> RabbitEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小兔子
    // auto baby = std::make_unique<RabbitEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // baby->setRabbitType(m_rabbitType); // 继承父母的皮肤类型
    // return baby;
    return nullptr;
}

void RabbitEntity::registerGoals() {
    // 调用父类方法
    AnimalEntity::registerGoals();

    // 兔子特有目标
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.2));

    // 优先级 2: 逃离玩家（野生兔子）
    // TODO: 需要 AvoidEntityGoal 支持
    // m_goalSelector.addGoal(2, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 8.0f, 2.2, 2.2));

    // 优先级 3: 繁殖（当处于爱心状态时）
    m_goalSelector.addGoal(3, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 4: 食物诱惑（胡萝卜）
    // m_goalSelector.addGoal(4, new entity::ai::goal::TemptGoal(this, 1.0, isCarrotPredicate));

    // 优先级 5: 跟随父母（幼体行为）
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 6: 随机漫步（兔子跳跃式移动）
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 0.6));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));
}

void RabbitEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 兔子的属性
    // 参考 MC 1.16.5 兔子属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
