#include "RabbitEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"  // 包含 LookRandomlyGoal
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../attribute/Attributes.hpp"
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
    std::uniform_int_distribution<int> dist(0, 5);
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

void RabbitEntity::setJumping(bool jumping) {
    LivingEntity::setJumping(jumping);

    if (!jumping) {
        return;
    }

    auto soundEvent = makeSoundEventId("jump");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, getSoundVolume(), ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 0.8f);
}

sound::SoundCategory RabbitEntity::getSoundCategory() const {
    return isKillerRabbit() ? sound::SoundCategory::Hostile : sound::SoundCategory::Neutral;
}

void RabbitEntity::playAttackSound(LivingEntity& /*target*/) {
    if (!isKillerRabbit()) {
        return;
    }

    auto soundEvent = makeSoundEventId("attack");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
}

void RabbitEntity::registerGoals() {
    // 调用父类方法（AgeableEntity 会调用 AnimalEntity，现在 AnimalEntity 不注册任何目标）
    AgeableEntity::registerGoals();

    // MC 1.16.5 RabbitEntity.registerGoals()
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表
    // 兔子有特殊的 AI 行为（逃跑更快）

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（兔子逃跑速度更快）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.2));

    // 优先级 2: 逃离玩家（野生兔子） - TODO: 需要 AvoidEntityGoal
    // m_goalSelector.addGoal(2, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 8.0f, 2.2, 2.2));

    // 优先级 3: 繁殖
    m_goalSelector.addGoal(3, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 4: 食物诱惑（胡萝卜、金胡萝卜、蒲公英）
    // TODO: 实现兔子的食物诱惑

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 6: 随机漫步
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

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
