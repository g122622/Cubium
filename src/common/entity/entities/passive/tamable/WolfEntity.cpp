#include "WolfEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include <cmath>

namespace mc {

WolfEntity::WolfEntity(LegacyEntityType type, EntityId id)
    : TameableEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> WolfEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<WolfEntity>(LegacyEntityType::Unknown, 0);
}

bool WolfEntity::isTameItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是骨头
    // return itemStack.getItem() == Items::BONE;
    (void)itemStack;
    return false;
}

bool WolfEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 驯服后用肉类繁殖
    // TODO: 检查是否是肉类
    // return itemStack.getItem()->isFood() && itemStack.getItem() != Items::BONE;
    (void)itemStack;
    return false;
}

bool WolfEntity::isFoodItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是肉类（包括腐肉）
    // return itemStack.getItem() == Items::PORKCHOP
    //     || itemStack.getItem() == Items::COOKED_PORKCHOP
    //     || itemStack.getItem() == Items::BEEF
    //     || itemStack.getItem() == Items::COOKED_BEEF
    //     || itemStack.getItem() == Items::CHICKEN
    //     || itemStack.getItem() == Items::COOKED_CHICKEN
    //     || itemStack.getItem() == Items::RABBIT
    //     || itemStack.getItem() == Items::COOKED_RABBIT
    //     || itemStack.getItem() == Items::MUTTON
    //     || itemStack.getItem() == Items::COOKED_MUTTON
    //     || itemStack.getItem() == Items::ROTTEN_FLESH;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> WolfEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小狼
    // auto baby = std::make_unique<WolfEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    return nullptr;
}

f32 WolfEntity::getTailAngle() const {
    // 根据生命值计算尾巴角度
    // 参考 MC 1.16.5 WolfEntity.getTailAngle()
    if (isAngry()) {
        // 愤怒时尾巴竖起
        return 1.539f; // 约88度
    }

    // 根据生命值计算
    f32 healthRatio = health() / maxHealth();
    return TAIL_ANGLE_UNHEALTHY + (healthRatio * (TAIL_ANGLE_HEALTHY - TAIL_ANGLE_UNHEALTHY));
}

bool WolfEntity::isInWater() const {
    // TODO: 检查是否在水中
    // return isInWaterOrBubble();
    return false;
}

void WolfEntity::registerGoals() {
    // 调用父类方法
    TameableEntity::registerGoals();

    // 狼特有目标
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 1: 坐下目标（驯服后）
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 2: 繁殖（当处于爱心状态时）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 跟随主人（驯服后）
    m_goalSelector.addGoal(3, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 3.0f, 10.0f, 32.0f));

    // 优先级 4: 食物诱惑（骨头用于驯服）
    // TODO: 需要实现骨头诱惑
    // m_goalSelector.addGoal(4, new entity::ai::goal::TemptGoal(this, 1.2, isTameItemPredicate));

    // 优先级 5: 跟随父母（幼体行为）
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 6: 随机漫步
    m_goalSelector.addGoal(6, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 7: 看向玩家
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // TODO: 添加攻击目标
    // 优先级 1: 攻击目标（未驯服时攻击附近生物，驯服后保护主人）
    // m_targetSelector.addGoal(1, new HurtByTargetGoal(this));
    // m_targetSelector.addGoal(2, new OwnerHurtByTargetGoal(this));
    // m_targetSelector.addGoal(3, new OwnerHurtTargetGoal(this));
    // m_targetSelector.addGoal(4, new NonTamedTargetGoal(this, EntityClassification::Monster, true));
}

void WolfEntity::registerAttributes() {
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 狼的属性
    // 参考 MC 1.16.5 狼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);  // 驯服前8血
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0); // 2点攻击力

    // 驯服后会增加到20血，这里由 onTamed 处理
}

void WolfEntity::onTamed(bool tamed) {
    if (tamed) {
        // 驯服后增加生命值上限
        // 参考 MC 1.16.5 狼驯服后从8血变为20血
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);

        // 驯服后增加攻击力
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
    } else {
        // 放弃驯服后恢复
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
        setHealth(8.0f);
        m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    }
}

} // namespace mc
