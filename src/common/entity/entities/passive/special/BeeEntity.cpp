#include "BeeEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

BeeEntity::BeeEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> BeeEntity::create(IWorld* /*world*/) {
    return std::make_unique<BeeEntity>(LegacyEntityType::Unknown, 0);
}

void BeeEntity::setHivePos(const BlockPos& pos) {
    m_hivePos = pos;
    m_hasHive = true;
}

void BeeEntity::setFlowerPos(const BlockPos& pos) {
    m_flowerPos = pos;
    m_hasFlower = true;
}

bool BeeEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是花朵
    // return itemStack.getItem().isIn(ItemTags::FLOWERS);
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> BeeEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小蜜蜂
    // auto baby = std::make_unique<BeeEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    (void)partner;
    return nullptr;
}

void BeeEntity::tick() {
    AnimalEntity::tick();

    // 更新愤怒计时器
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_attackTarget = nullptr;
            m_attacking = false;
            m_targetPlayerId = 0;
        }
    }

    // 螫刺后死亡处理
    if (m_hasStung) {
        // 螫刺后逐渐死亡
        // TODO: 每 tick 有概率死亡
    }

    // 水下计时
    // TODO: 检查是否在水中
    // if (isInWater()) {
    //     m_underWaterTimer++;
    //     if (m_underWaterTimer > 20) {
    //         // 开始溺水
    //     }
    // } else {
    //     m_underWaterTimer = 0;
    // }
}

void BeeEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 蜜蜂特有目标
    // 优先级 3: 食物诱惑（花朵）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isFlowerPredicate));

    // TODO: 蜜蜂特有目标
    // - BeeFindFlowerGoal: 寻找花朵
    // - BeePollinateGoal: 授粉
    // - BeeReturnToHiveGoal: 返回蜂巢
    // - BeeAttackGoal: 攻击目标
    // - BeeWanderGoal: 飞行漫步
}

void BeeEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 蜜蜂的属性
    // 参考 MC 1.16.5 蜜蜂属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);

    // 蜜蜂飞行速度较高
    // TODO: 设置飞行速度属性
}

} // namespace mc
