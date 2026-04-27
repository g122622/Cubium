#include "AnimalEntity.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../core/EntityDataManager.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

namespace mc {

AnimalEntity::AnimalEntity(LegacyEntityType type, EntityId id)
    : AgeableEntity(type, id)
{
    // 注册属性
    registerAttributes();

    // MC 1.16.5: 设置路径优先级
    // setPathPriority(PathNodeType.DANGER_FIRE, 16.0F);
    // setPathPriority(PathNodeType.DAMAGE_FIRE, -1.0F);
}

bool AnimalEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 默认检查是否为小麦
    // 子类应该重写此方法来定义特定的繁殖物品
    return !itemStack.isEmpty() && itemStack.getItem() == Items::WHEAT;
}

bool AnimalEntity::canMateWith(const AnimalEntity& other) const {
    // MC 1.16.5: 检查双方都是成体、都处于爱心状态、是同类
    if (this == &other) {
        return false;
    }

    // 使用实体类型比较（避免 RTTI 开销）
    if (legacyType() != other.legacyType()) {
        return false;
    }

    // 检查双方都是成体且都处于爱心状态
    return isInLove() && other.isInLove();
}

bool AnimalEntity::canBreed() const {
    // MC 1.16.5: 年龄为0且不处于爱心状态
    return getGrowingAge() == 0 && !isInLove();
}

void AnimalEntity::setInLove(u64 playerUuid) {
    // MC 1.16.5: 设置爱心状态持续 600 ticks（30秒）
    // 调用 AgeableEntity::setInLove() 设置计时器
    AgeableEntity::setInLove(playerUuid);

    // 记录喂食玩家的 UUID
    m_loveCause = playerUuid;

    // MC 1.16.5: 广播状态更新（用于客户端粒子效果）
    // world->setEntityState(this, static_cast<u8>(18));
}

void AnimalEntity::resetInLove() {
    // 清空爱心计时器
    resetLove();
    m_loveCause = 0;
}

i32 AnimalEntity::getExperiencePoints() const {
    // MC 1.16.5: 返回 1-3 经验
    math::Random rng = getRandom();
    return 1 + rng.nextInt(3);
}

void AnimalEntity::tick() {
    AgeableEntity::tick();

    updateInLove();

    // MC 1.16.5: 成体时清空爱心状态
    // updateAITasks() 中会检查年龄并清空爱心
}

void AnimalEntity::registerGoals() {
    // 调用父类方法
    AgeableEntity::registerGoals();

    // 基础动物 AI 目标
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.25));

    // 优先级 2: 繁殖（当处于爱心状态时）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 食物诱惑（当玩家手持食物时）
    // 子类需要设置食物检测谓词
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, foodPredicate));

    // 优先级 4: 跟随父母（幼体行为）
    m_goalSelector.addGoal(4, new entity::ai::goal::FollowParentGoal(this, 1.1));

    // 优先级 5: 随机漫步
    m_goalSelector.addGoal(5, new entity::ai::goal::RandomWalkingGoal(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, new entity::ai::goal::LookAtGoal(this, 6.0f));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));
}

void AnimalEntity::registerAttributes() {
    // 调用父类方法
    AgeableEntity::registerAttributes();

    // 动物的基础属性
    // 参考 MC 1.16.5 AnimalEntity 默认属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

void AnimalEntity::updateInLove() {
    // 快速路径：使用 AgeableEntity 的爱心计时器
    // AgeableEntity::updateLove() 已经处理了爱心计时器递减
    // 这里只需要处理粒子效果

    i32 loveTimer = getLoveTimer();
    if (loveTimer <= 0) {
        return;
    }

    // MC 1.16.5: 成体时如果有年龄（繁殖冷却），清空爱心状态
    if (getGrowingAge() != 0) {
        resetLove();
        m_loveCause = 0;
        return;
    }

    // 每10tick生成爱心粒子
    if ((loveTimer % 10) == 0) {
        spawnHeartParticles();
    }
}

void AnimalEntity::spawnHeartParticles() {
    // MC 1.16.5: 生成心形粒子
    if (!m_world) {
        return;
    }

    mc::math::Random rng = getRandom();
    f32 ox = (rng.nextFloat() * 2.0f - 1.0f) * width();
    f32 oy = height() + 0.5f + rng.nextFloat() * 0.5f;
    f32 oz = (rng.nextFloat() * 2.0f - 1.0f) * width();

    Vector3 pos(x() + ox, y() + oy, z() + oz);
    Vector3 vel(0.0f, 0.0f, 0.0f);

    m_world->addParticle(
        client::renderer::trident::particle::ParticleTypeId::Heart,
        pos, vel);
}

bool AnimalEntity::hurt(DamageSource& source, f32 amount) {
    // MC 1.16.5: 受伤时清空爱心状态（不重置繁殖冷却）
    resetInLove();

    return AgeableEntity::hurt(source, amount);
}

} // namespace mc
