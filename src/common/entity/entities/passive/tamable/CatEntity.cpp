#include "CatEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/MobEntity.hpp"

namespace mc {

CatEntity::CatEntity(LegacyEntityType type, EntityId id)
    : TameableEntity(type, id)
{
    // 随机设置皮肤类型
    setRandomCatType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CatEntity>(LegacyEntityType::Unknown, 0);
}

void CatEntity::setRandomCatType()
{
    math::Random rng = getRandom();
    m_catType = static_cast<CatType>(rng.nextInt(0, 10));
}

bool CatEntity::isTameItem(const ItemStack& itemStack) const
{
    // 猫用生鳕鱼或生鲑鱼驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::COD || item == Items::SALMON;
}

bool CatEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 驯服后用生鱼繁殖
    return isTameItem(itemStack);
}

bool CatEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 同繁殖物品
    return isTameItem(itemStack);
}

std::unique_ptr<AnimalEntity> CatEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小猫
    auto baby = std::make_unique<CatEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void CatEntity::registerGoals()
{
    // 调用父类方法（已包含 SwimGoal, PanicGoal, BreedGoal, FollowParentGoal, RandomWalkingGoal, LookAtGoal,
    // LookRandomlyGoal）
    TameableEntity::registerGoals();

    // 猫特有目标
    // 注意：不要重复注册父类已注册的Goal

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 3: 跟随主人（驯服后）- 替换FollowParentGoal的行为
    m_goalSelector.addGoal(3, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 5.0f, 10.0f, 32.0f));

    // 优先级 4: 食物诱惑（生鱼用于驯服）
    // TODO: 需要实现鱼类诱惑
    // m_goalSelector.addGoal(4, new entity::ai::goal::TemptGoal(this, 0.6, isFishPredicate));

    // 优先级 6: 逃离玩家（未驯服时）
    // 未驯服的猫会逃离玩家
    // TODO: 需要 AvoidEntityGoal 支持
    // m_goalSelector.addGoal(6, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 16.0f, 0.8, 1.33));
}

void CatEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 猫的属性
    // 参考 MC 1.16.5 猫属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void CatEntity::onTamed(bool tamed)
{
    if (tamed) {
        // 驯服后增加生命值
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
        setHealth(10.0f);

        // 礼物计时器初始化
        m_giftTimer = GIFT_INTERVAL;
    } else {
        m_giftTimer = 0;
    }
}

std::optional<ResourceLocation> CatEntity::getAmbientSound() const
{
    // 驯服后的猫使用 ENTITY_CAT_AMBIENT
    // 未驯服的流浪猫使用 ENTITY_CAT_STRAY_AMBIENT
    if (isTamed()) {
        return SoundEvents::ENTITY_CAT_AMBIENT;
    }
    return SoundEvents::ENTITY_CAT_STRAY_AMBIENT;
}

std::optional<ResourceLocation> CatEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_CAT_HURT;
}

std::optional<ResourceLocation> CatEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_CAT_DEATH;
}

void CatEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_CAT_EAT, 1.0f, 1.0f);
}

} // namespace mc
