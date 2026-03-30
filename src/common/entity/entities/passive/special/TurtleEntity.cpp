#include "TurtleEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../../world/World.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/PanicGoal.hpp"
#include "../../ai/goal/goals/BreedGoal.hpp"
#include "../../ai/goal/goals/TemptGoal.hpp"
#include "../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../attribute/Attributes.hpp"

namespace mc {

TurtleEntity::TurtleEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> TurtleEntity::create(IWorld* /*world*/) {
    return std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
}

void TurtleEntity::setHomePos(const BlockPos& pos) {
    m_homePos = pos;
    m_hasHomePos = true;
}

bool TurtleEntity::isInWater() const {
    // TODO: 检查是否在水中
    // return isInWaterState();
    return false;
}

bool TurtleEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是海草
    // return itemStack.getItem() == Items::SEAGRASS;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> TurtleEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小海龟
    // auto baby = std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // baby->setHomePos(m_homePos); // 继承出生地
    // return baby;
    (void)partner;
    return nullptr;
}

void TurtleEntity::tick() {
    AnimalEntity::tick();

    // 更新产卵计时器
    if (m_layingEgg && m_layEggTimer > 0) {
        m_layEggTimer--;
        if (m_layEggTimer <= 0) {
            // 产卵完成
            m_layingEgg = false;
            m_hasEgg = false;
            // TODO: 在脚下生成海龟蛋方块
        }
    }
}

void TurtleEntity::registerGoals() {
    // 调用父类方法
    AnimalEntity::registerGoals();

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（水陆两用）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（海草）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isSeagrassPredicate));

    // 优先级 4: 随机漫步
    m_goalSelector.addGoal(4, new entity::ai::goal::RandomWalkingGoal(this, 0.5));

    // 优先级 5: 看向玩家
    m_goalSelector.addGoal(5, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, new entity::ai::goal::LookRandomlyGoal(this));

    // TODO: 海龟特有目标
    // - TurtleGoHomeGoal: 返回出生地
    // - TurtleLayEggGoal: 产卵
    // - TurtleTravelGoal: 前往海里
}

void TurtleEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 海龟的属性
    // 参考 MC 1.16.5 海龟属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    // 陆地上移动更慢
    // TODO: 在陆地上时减慢速度
}

} // namespace mc
