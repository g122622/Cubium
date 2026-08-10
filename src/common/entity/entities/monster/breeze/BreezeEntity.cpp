/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BreezeEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/BreezeGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileDeflection.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/entities/projectile/WindChargeEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace mc {

BreezeEntity::BreezeEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MonsterEntity(id, registry)
{
    registerGoals();
    registerAttributes();

    // Breeze 经验值为 10
    setExperienceValue(10);
}

std::unique_ptr<Entity> BreezeEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<BreezeEntity>(EntityInstanceId(0), registry);
}

void BreezeEntity::tick()
{
    // 根据当前 Pose 发射粒子、推进动画状态
    const EntityPose pose = this->pose();
    switch (pose) {
        case EntityPose::Shooting:
        case EntityPose::Inhaling:
        case EntityPose::Standing:
            resetJumpTrail();
            emitGroundParticles(IDLE_PARTICLES_AMOUNT + static_cast<i32>(getRandom().nextInt(1)));
            break;
        case EntityPose::Sliding:
            emitGroundParticles(SLIDE_PARTICLES_AMOUNT);
            break;
        case EntityPose::LongJumping:
            m_longJumpAnim.startIfStopped(static_cast<i32>(ticksExisted()));
            emitJumpTrailParticles();
            break;
        default:
            // 其他姿态不发射地面/轨迹粒子
            break;
    }

    // 空闲动画持续触发
    m_idleAnim.startIfStopped(static_cast<i32>(ticksExisted()));

    // 滑行 → 滑行回弹动画过渡
    updateSlideBackAnimation();

    // 呼啸音效随机播放
    if (m_soundTick == 0) {
        m_soundTick = getRandom().nextInt(WHIRL_SOUND_FREQUENCY_MIN, WHIRL_SOUND_FREQUENCY_MAX);
    } else {
        --m_soundTick;
    }
    if (m_soundTick == 0) {
        playWhirlSound();
    }

    MonsterEntity::tick();

    // 更新射击冷却
    if (m_shootCooldown > 0) {
        --m_shootCooldown;
    }

    // 更新长跳冷却
    if (m_jumpCooldown > 0) {
        --m_jumpCooldown;
    }

    // 更新射击许可计时器
    if (m_shootPermitTicks > 0) {
        --m_shootPermitTicks;
    }

    // 着陆时清除长跳状态
    if (m_isLongJumping && onGround()) {
        m_isLongJumping = false;
    }
}

void BreezeEntity::updateSlideBackAnimation()
{
    // 当旋风人不再处于滑行姿态但滑行动画仍在播放时，
    // 启动滑行回弹动画并停止滑行动画。
    if (pose() != EntityPose::Sliding && m_slideAnim.isStarted()) {
        m_slideBackAnim.start(static_cast<i32>(ticksExisted()));
        m_slideAnim.stop();
    }
}

void BreezeEntity::emitGroundParticles(i32 count)
{
    // 被骑乘时不发射地面粒子
    if (isRiding()) {
        return;
    }

    if (m_world == nullptr) {
        return;
    }

    // 计算实体碰撞箱中心点的地面位置
    const AxisAlignedBB bbox = boundingBox();
    const Vector3 center = bbox.center();
    const Vector3 groundPos(center.x, position().y, center.z);

    // 获取脚下方块状态（实体所站立的方块）
    const BlockPos belowPos(static_cast<i32>(std::floor(groundPos.x)),
        static_cast<i32>(std::floor(groundPos.y)) - 1,
        static_cast<i32>(std::floor(groundPos.z)));
    const BlockState* blockState = m_world->getBlockState(belowPos);
    if (blockState == nullptr || blockState->isAir()) {
        return;
    }

    // 发射 count 个 BLOCK 类型粒子，携带方块状态
    for (i32 i = 0; i < count; ++i) {
        m_world->addBlockParticle(particle::ParticleTypeId::Block, groundPos, Vector3(0.0, 0.0, 0.0), *blockState);
    }
}

void BreezeEntity::emitJumpTrailParticles()
{
    // 前 5 tick 发射轨迹粒子
    if (++m_jumpTrailStartedTick > JUMP_TRAIL_DURATION_TICKS) {
        return;
    }

    if (m_world == nullptr) {
        return;
    }

    // 获取实体当前穿过或脚下的方块状态
    const BlockPos currentPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));
    const BlockState* blockState = m_world->getBlockState(currentPos);
    if (blockState == nullptr || blockState->isAir()) {
        // 回退到脚下方块
        const BlockPos belowPos(static_cast<i32>(std::floor(x())),
            static_cast<i32>(std::floor(y())) - 1,
            static_cast<i32>(std::floor(z())));
        blockState = m_world->getBlockState(belowPos);
        if (blockState == nullptr || blockState->isAir()) {
            return;
        }
    }

    // 在实体前方稍上方位置发射 3 个 BLOCK 粒子
    const Vector3 lookAhead = position() + velocity() + Vector3(0.0f, 0.1f, 0.0f);
    for (i32 i = 0; i < JUMP_TRAIL_PARTICLES_AMOUNT; ++i) {
        m_world->addBlockParticle(particle::ParticleTypeId::Block, lookAhead, Vector3(0.0, 0.0, 0.0), *blockState);
    }
}

void BreezeEntity::playWhirlSound()
{
    if (m_world == nullptr) {
        return;
    }

    // 音调和音量带随机扰动
    const f32 volume = 0.8f + 0.2f * getRandom().nextFloat();
    const f32 pitch = 0.7f + 0.4f * getRandom().nextFloat();
    m_world->playSound(SoundEvents::ENTITY_BREEZE_WHIRL, sound::SoundCategory::Hostile, position(), volume, pitch);
}

void BreezeEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 行为目标（参考 BreezeAi.FIGHT 行为优先级）
    m_goalSelector.addGoal(1, new entity::ai::goal::SwimGoal(this));
    m_goalSelector.addGoal(2, new entity::ai::goal::BreezeShootGoal(this));          // 射击风弹
    m_goalSelector.addGoal(3, new entity::ai::goal::BreezeLongJumpGoal(this));       // 长跳移动
    m_goalSelector.addGoal(4, new entity::ai::goal::BreezeShootWhenStuckGoal(this)); // 卡住时紧急射击
    m_goalSelector.addGoal(5, new entity::ai::goal::BreezeSlideGoal(this));          // 滑行移动
    m_goalSelector.addGoal(6, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.35));
    m_goalSelector.addGoal(7, new entity::ai::goal::LookAtGoal(this, 8.0F, 0.02F));
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));
    m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));
}

void BreezeEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, MAX_HEALTH);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, MOVEMENT_SPEED);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, FOLLOW_RANGE);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
}

std::optional<ResourceLocation> BreezeEntity::getAmbientSound() const
{
    // 对齐原版 Breeze.getAmbientSound：在地面播放 IDLE_GROUND，空中播放 IDLE_AIR。
    // sounds.json 中无 entity.breeze.ambient，仅有 idle_ground/idle_air。
    if (onGround()) {
        return SoundEvents::ENTITY_BREEZE_IDLE_GROUND;
    }
    return SoundEvents::ENTITY_BREEZE_IDLE_AIR;
}

bool BreezeEntity::canAttackType(const entity::EntityType& type) const
{
    // Breeze.canAttackType()：仅允许攻击玩家和铁傀儡
    // 旋风人采用白名单模式，其余所有实体类型都不能被攻击
    // 指针比较：type 必来自注册表，与 VanillaEntityTypeKeys::* 同源
    return &type == entity::VanillaEntityTypeKeys::PLAYER || &type == entity::VanillaEntityTypeKeys::IRON_GOLEM;
}

ProjectileDeflection BreezeEntity::deflection(const entity::ProjectileEntity& projectile) const
{
    // Breeze.deflection(Projectile)
    // 旋风人不偏转风弹（包括旋风人风弹和玩家风弹）
    if (dynamic_cast<const entity::WindChargeEntity*>(&projectile) != nullptr) {
        return ProjectileDeflection::None;
    }

    // 其他投射物：如果旋风人实体类型属于 DEFLECTS_PROJECTILES 标签，则反向偏转并播放音效
    if (EntityTypeTags::DEFLECTS_PROJECTILES().contains(getTypeId())) {
        // 播放旋风人偏转音效
        if (m_world != nullptr) {
            m_world->playSound(
                SoundEvents::ENTITY_BREEZE_DEFLECT, sound::SoundCategory::Hostile, position(), 1.0f, 1.0f);
        }
        return ProjectileDeflection::Reverse;
    }

    return ProjectileDeflection::None;
}

void BreezeEntity::shootWindCharge()
{
    if (m_shootCooldown > 0) {
        return;
    }

    if (m_world == nullptr || m_attackTarget == nullptr) {
        return;
    }

    // 计算从旋风人到目标的方向向量
    // 旋风人发射位置：身体中心偏上0.3格（对应 MC 1.21.11 Breeze.getFiringYPosition()）
    const f32 firingY = y() + height() * 0.5f + 0.3f;
    const Vector3 firingPos(x(), firingY, z());

    // 方向向量
    // 对齐 MC 1.21.11 Shoot.tick()：
    //   d1 = livingentity.getY(livingentity.isPassenger() ? 0.8 : 0.3) - breeze.getFiringYPosition();
    // 其中 Entity.getY(partialY) = position.y + height * partialY。
    // - 非骑乘目标：partialY = 0.3（瞄准躯干下部），补偿风弹无重力补偿的抛物线下坠
    // - 骑乘目标：partialY = 0.8（瞄准接近头部），避开载具碰撞盒遮挡
    // 项目中 MC 的 isPassenger() 对应 isRiding()（本实体正在骑乘其他实体）。
    const f64 targetPartialY = m_attackTarget->isRiding() ? 0.8 : 0.3;
    const f64 targetY = m_attackTarget->getY(targetPartialY);
    const f32 dx = static_cast<f32>(static_cast<f64>(m_attackTarget->x()) - static_cast<f64>(firingPos.x));
    const f32 dy = static_cast<f32>(targetY - static_cast<f64>(firingPos.y));
    const f32 dz = static_cast<f32>(static_cast<f64>(m_attackTarget->z()) - static_cast<f64>(firingPos.z));

    // 创建风弹弹射物实体（通过发射者类型自动判定为旋风人风弹）
    // ECS 迁移：实体构造需要 registry 句柄，m_world 在攻击路径已确保非空
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    auto entity = std::make_unique<entity::WindChargeEntity>(EntityInstanceId(0), *registry);
    entity->setWorld(m_world);
    entity->setPosition(firingPos.x, firingPos.y, firingPos.z);
    entity->setShooter(this);

    entity::WindChargeEntity* projectile = entity.get();
    m_world->spawnEntity(std::move(entity));

    // 设置射击参数：速度 0.7，散布根据难度计算
    // MC 1.21.11 Shoot.tick(): 5 - difficulty.getId() * 4
    // 各难度散布值：Peaceful=5, Easy=1, Normal=-3, Hard=-7
    // Normal/Hard 为负数，ProjectileEntity::shoot 内部取绝对值处理，
    // 散布效果等效于 3 / 7（难度越高散布越大）
    constexpr f32 PROJECTILE_VELOCITY = 0.7f;
    const f32 inaccuracy = entity::combat::DifficultyHelper::getBreezeWindChargeInaccuracy(m_world->difficulty());
    projectile->shoot(dx, dy, dz, PROJECTILE_VELOCITY, inaccuracy);

    // 播放旋风人射击音效
    m_world->playSound(
        SoundEvents::ENTITY_BREEZE_WIND_CHARGE_BURST, sound::SoundCategory::Hostile, firingPos, 1.0f, 1.0f);

    // 设置射击冷却
    m_shootCooldown = 20; // 1秒冷却
}

void BreezeEntity::die(DamageSource& source)
{
    // 调用父类 die()，处理死亡动画和经验掉落
    MonsterEntity::die(source);

    // Breeze 掉落逻辑：
    // 战利品表 minecraft:entities/breeze 定义：
    //   - 条件: killed_by_player
    //   - 物品: Breeze Rod
    //   - 基础数量: 1-2 (uniform 1.0~2.0)
    //   - 抢夺加成: 每级额外 1-2 (uniform 1.0~2.0)
    //
    // 当前项目尚未实现通用的实体战利品表掉落流程，
    // 因此直接在此硬编码掉落逻辑。

    // 仅在被玩家击杀时掉落
    if (!source.isPlayerSource() && !source.isEntitySource()) {
        return;
    }

    // 尝试从伤害来源获取玩家实体
    Player* killer = nullptr;
    Entity* attacker = source.getEntity();
    if (attacker != nullptr) {
        killer = dynamic_cast<Player*>(attacker);
    }
    // 对于间接伤害（如箭矢），获取真正的来源
    if (killer == nullptr) {
        Entity* trueSource = source.getTrueSource();
        if (trueSource != nullptr) {
            killer = dynamic_cast<Player*>(trueSource);
        }
    }

    if (killer == nullptr) {
        return;
    }

    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 确保物品已注册
    if (Items::BREEZE_ROD == nullptr) {
        return;
    }

    // 计算抢夺等级
    ItemStack weapon = killer->getHeldItem(Hand::MainHand);
    i32 lootingLevel = item::enchant::EnchantmentHelper::getLootingLevel(weapon);

    // 计算掉落数量
    // 基础: 1-2 个狂风杖
    // 抢夺加成: 每级额外 1-2 个
    math::Random& rng = getRandom();
    i32 count = 1 + rng.nextInt(2); // 基础 1-2
    for (i32 i = 0; i < lootingLevel; ++i) {
        count += 1 + rng.nextInt(2); // 每级额外 1-2
    }

    // 掉落狂风杖
    ItemStack breezeRod(Items::BREEZE_ROD, count);
    ItemDropHelper::spawnItemAtEntity(this, breezeRod, 0.5f, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
}

} // namespace mc
