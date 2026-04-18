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
#include "../../../ai/goal/goals/LookAtGoal.hpp"
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
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了：SwimGoal(0), PanicGoal(1), BreedGoal(2),
    // FollowParentGoal(4), RandomWalkingGoal(5), LookAtGoal(6), LookRandomlyGoal(7)
    AnimalEntity::registerGoals();

    // 兔子特有目标
    // 注意：兔子速度更快，所以用更高的速度参数替换 PanicGoal
    // 但由于 GoalSelector 不支持替换，这里只添加额外的目标

    // 优先级 2: 逃离玩家（野生兔子）
    // TODO: 需要 AvoidEntityGoal 支持
    // m_goalSelector.addGoal(2, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 8.0f, 2.2, 2.2));

    // 优先级 3: 食物诱惑（胡萝卜）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isCarrotPredicate));
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
