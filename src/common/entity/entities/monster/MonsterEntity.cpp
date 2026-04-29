#include "MonsterEntity.hpp"
#include "../../core/MobEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../combat/DifficultyHelper.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockPos.hpp"

namespace mc {

MonsterEntity::MonsterEntity(LegacyEntityType type, EntityId id)
    : CreatureEntity(type, id)
{
    // MC 1.16.5: 怪物默认经验值为 5
    setExperienceValue(5);

    // 注册 AI 目标
    registerGoals();
}

std::optional<ResourceLocation> MonsterEntity::getHurtSound(DamageSource& /*source*/) const {
    // MC 1.16.5: entity.hostile.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> MonsterEntity::getDeathSound() const {
    // MC 1.16.5: entity.hostile.death
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> MonsterEntity::getFallSound(i32 fallHeight) const {
    // MC 1.16.5: 根据摔落高度返回不同声音
    if (fallHeight > 4) {
        return makeSoundEventId("fall.big");
    } else {
        return makeSoundEventId("fall.small");
    }
}

bool MonsterEntity::hurt(DamageSource& source, f32 amount) {
    // MC 1.16.5: 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }
    return CreatureEntity::hurt(source, amount);
}

bool MonsterEntity::isInDaylight() const {
    // MC 1.16.5: 检查是否在日光下
    // 需要检查：1. 世界是否为白天 2. 天空是否可见 3. 实体是否暴露在阳光下
    const IWorld* worldPtr = world();
    if (!worldPtr) {
        return false;
    }

    // 检查是否为白天（dayTime < 12000 为白天）
    i64 time = worldPtr->dayTime();
    bool isDaytime = time < 12000;
    if (!isDaytime) {
        return false;
    }

    // TODO: 检查天空是否可见 (需要 IWorld::canSeeSky 方法)
    // TODO: 检查是否在水中或在雨中（这些情况下不会燃烧）
    return true;
}

bool MonsterEntity::shouldAttack(LivingEntity* target) const {
    // 默认实现：攻击所有活着的生物
    return target != nullptr && target->isAlive();
}

void MonsterEntity::tick() {
    // MC 1.16.5: 更新基于亮度的空闲时间
    updateIdleTimeBasedOnBrightness();

    CreatureEntity::tick();

    // 处理阳光燃烧
    handleDaylightBurning();
}

void MonsterEntity::registerGoals() {
    // 调用父类方法
    CreatureEntity::registerGoals();

    // 敌对生物基础 AI
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 目标选择器
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));

    // 注意: 具体的攻击目标选择需要子类添加
    // 例如僵尸会添加 NearestAttackableTargetGoal<Player>
}

void MonsterEntity::handleDaylightBurning() {
    if (m_burnsInDaylight && isInDaylight()) {
        // 在阳光下燃烧
        m_burnTime++;
        if (m_burnTime >= 20) {  // 1秒后开始燃烧
            // MC 1.16.5: 造成火焰伤害
            auto fireDamage = DamageSources::onFire();
            hurt(fireDamage, 1.0f);

            // TODO: 设置视觉上的燃烧效果
            // setOnFireFor(8); // 燃烧8秒
        }
    } else {
        m_burnTime = 0;
    }
}

void MonsterEntity::updateIdleTimeBasedOnBrightness() {
    // MC 1.16.5 MonsterEntity.func_213623_ec()
    // 如果亮度大于 0.5，增加空闲时间（用于 despawn 逻辑）
    // TODO: 需要获取亮度方法
    // f32 brightness = getBrightness();
    // if (brightness > 0.5f) {
    //     setIdleTime(idleTime() + 2);
    // }
}

// ========== 静态生成方法 ==========

bool MonsterEntity::isValidLightLevel(IWorld& world, const BlockPos& pos, math::Random& random) {
    // MC 1.16.5 MonsterEntity.isValidLightLevel()
    // 检查天空光照
    u8 skyLight = world.getSkyLight(pos);
    if (skyLight > random.nextInt(32)) {
        return false;
    }

    // 检查方块光照
    u8 light = world.getBlockLight(pos);
    // TODO: 如果在雷暴天气，使用更低的亮度阈值
    // if (world.isThundering()) {
    //     light = world.getNeighborAwareLightSubtracted(pos, 10);
    // }
    return light <= random.nextInt(8);
}

bool MonsterEntity::canMonsterSpawnInLight(
    LegacyEntityType /*type*/,
    IWorld& world,
    SpawnReason /*reason*/,
    const BlockPos& pos,
    math::Random& random)
{
    // MC 1.16.5 MonsterEntity.canMonsterSpawnInLight()
    // 检查难度（非和平模式）
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        return false;
    }

    // 检查光照等级
    if (!isValidLightLevel(world, pos, random)) {
        return false;
    }

    // TODO: 检查生成位置是否有效
    // return canSpawnOn(type, world, reason, pos, random);
    return true;
}

bool MonsterEntity::canMonsterSpawn(
    LegacyEntityType /*type*/,
    IWorld& world,
    SpawnReason /*reason*/,
    const BlockPos& /*pos*/,
    math::Random& /*random*/)
{
    // MC 1.16.5 MonsterEntity.canMonsterSpawn()
    // 检查难度（非和平模式）
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        return false;
    }

    // 不检查光照等级
    // TODO: 检查生成位置是否有效
    // return canSpawnOn(type, world, reason, pos, random);
    return true;
}

} // namespace mc
