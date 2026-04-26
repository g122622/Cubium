#include "WolfEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
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
    // 狼用骨头驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::BONE;
}

bool WolfEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 驯服后用肉类繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::PORKCHOP
        || item == Items::COOKED_PORKCHOP
        || item == Items::BEEF
        || item == Items::COOKED_BEEF
        || item == Items::CHICKEN
        || item == Items::COOKED_CHICKEN
        || item == Items::RABBIT
        || item == Items::COOKED_RABBIT
        || item == Items::MUTTON
        || item == Items::COOKED_MUTTON;
        // TODO: 添加 ROTTEN_FLESH（腐肉）当 Items 中定义后
}

bool WolfEntity::isFoodItem(const ItemStack& itemStack) const {
    // 同繁殖物品
    return isBreedingItem(itemStack);
}

std::unique_ptr<AnimalEntity> WolfEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // 创建小狼
    auto baby = std::make_unique<WolfEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void WolfEntity::tick() {
    TameableEntity::tick();

    if (!isAlive()) {
        return;
    }

    const f32 dx = x() - prevX();
    const f32 dz = z() - prevZ();
    const f32 horizontalDistance = std::sqrt(dx * dx + dz * dz);

    if (horizontalDistance > 0.0f) {
        m_stepSoundDistance += horizontalDistance * 0.6f;
        if (m_stepSoundDistance > m_nextStepSoundDistance && onGround() && !isInWater()) {
            m_nextStepSoundDistance = std::floor(m_stepSoundDistance) + 1.0f;
            playStepSound();
        }
    }

    const bool inWater = isInWater();
    if (m_wasInWater && !inWater && onGround()) {
        playShakingSound();
    }
    m_wasInWater = inWater;
}

std::optional<ResourceLocation> WolfEntity::getAmbientSound() const {
    math::Random random = getRandom();

    if (isAngry()) {
        return makeSoundEventId("growl");
    }

    if (random.nextInt(3) == 0) {
        if (isTamed() && health() < 10.0f) {
            return makeSoundEventId("whine");
        }

        return makeSoundEventId("pant");
    }

    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WolfEntity::getHurtSound(DamageSource& /*source*/) const {
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WolfEntity::getDeathSound() const {
    return makeSoundEventId("death");
}

void WolfEntity::playStepSound() {
    auto soundEvent = makeSoundEventId("step");
    if (!soundEvent.has_value()) {
        return;
    }

    playSound(*soundEvent, 0.15f, 1.0f);
}

void WolfEntity::playShakingSound() {
    auto soundEvent = makeSoundEventId("shake");
    if (!soundEvent.has_value()) {
        return;
    }

    math::Random random = getRandom();
    playSound(*soundEvent, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
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
    // 调用父类实现检查是否在水中
    return TameableEntity::isInWater();
}

void WolfEntity::registerGoals() {
    // 调用父类方法（已包含 SwimGoal, PanicGoal, BreedGoal, FollowParentGoal, RandomWalkingGoal, LookAtGoal, LookRandomlyGoal）
    TameableEntity::registerGoals();

    // 狼特有目标
    // 注意：不要重复注册父类已注册的Goal

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 3: 跟随主人（驯服后）- 替换FollowParentGoal的行为
    m_goalSelector.addGoal(3, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 3.0f, 10.0f, 32.0f));

    // 优先级 4: 食物诱惑（骨头用于驯服）
    // TODO: 需要实现骨头诱惑
    // m_goalSelector.addGoal(4, new entity::ai::goal::TemptGoal(this, 1.2, isTameItemPredicate));

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
