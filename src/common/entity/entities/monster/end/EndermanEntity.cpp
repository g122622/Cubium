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

#include "EndermanEntity.hpp"
#include "../../player/Player.hpp"
#include "../arthropod/EndermiteEntity.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/damage/DamageSource.hpp"
#include <cmath>

namespace mc {

EndermanEntity::EndermanEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // MC 1.16.5: 末影人不在阳光下燃烧
    setBurnsInDaylight(false);

    // MC 1.16.5: EndermanEntity 构造函数中设置 stepHeight = 1.0F
    // 末影人可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> EndermanEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EndermanEntity>(LegacyEntityType::Unknown, 0);
}

std::optional<ResourceLocation> EndermanEntity::getAmbientSound() const
{
    // MC 1.16.5: 愤怒时返回 ambient，被注视时返回 scream
    if (m_screaming) {
        return makeSoundEventId("scream");
    }
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> EndermanEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.enderman.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> EndermanEntity::getDeathSound() const
{
    // MC 1.16.5: entity.enderman.death
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> EndermanEntity::getStareSound() const
{
    // MC 1.16.5: entity.enderman.stare
    return makeSoundEventId("stare");
}

std::optional<ResourceLocation> EndermanEntity::getTeleportSound() const
{
    // MC 1.16.5: entity.enderman.teleport
    return makeSoundEventId("teleport");
}

void EndermanEntity::setRevengeTarget(LivingEntity* target)
{
    m_attackTarget = target;
    if (target != nullptr) {
        setAngry(true);
        m_angerTime = ANGER_DURATION;
        m_revengeTargetId = target->id();
        m_revengeTimer = ANGER_DURATION;
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* EndermanEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void EndermanEntity::setAngry(bool angry)
{
    m_angry = angry;
    if (!angry) {
        m_angerTime = 0;
        m_attackTarget = nullptr;
        m_screaming = false;
    }
}

void EndermanEntity::setHeldBlockState(const BlockState* state)
{
    m_heldBlockState = state;
    m_holdingBlock = (state != nullptr);
}

bool EndermanEntity::teleport()
{
    // MC 1.16.5 EndermanEntity.teleport()
    if (m_teleportCooldown > 0) {
        return false;
    }

    // 末影人瞬移范围：64 格
    // 参考 MC 1.16.5 EndermanEntity.teleportRandomly()
    bool success = randomTeleport(TELEPORT_RANGE, true, true);

    if (success) {
        m_teleportCooldown = TELEPORT_COOLDOWN;

        // 播放瞬移音效
        auto teleportSound = getTeleportSound();
        if (teleportSound) {
            playSound(*teleportSound, 1.0f, 1.0f);
        }
    }

    return success;
}

bool EndermanEntity::teleportToTarget()
{
    // MC 1.16.5 EndermanEntity.teleportTowards()
    if (m_attackTarget == nullptr || m_teleportCooldown > 0) {
        return false;
    }

    // 计算远离目标的方向向量
    Vector3 direction(m_position.x - m_attackTarget->position().x, 0.0, m_position.z - m_attackTarget->position().z);

    // 归一化方向向量
    f32 length = direction.length();
    if (length > 0.001f) {
        direction.x /= length;
        direction.z /= length;
    } else {
        // 如果长度太小，随机选择方向
        math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
        f32 angle = rng.nextFloat() * 6.28318530718f;
        direction.x = std::cos(angle);
        direction.z = std::sin(angle);
    }

    // 目标位置：远离目标 16 格
    math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
    f32 targetX = m_position.x + (rng.nextDouble() - 0.5) * 8.0 - direction.x * 16.0;
    f32 targetY = m_position.y + static_cast<f32>(rng.nextInt(16) - 8);
    f32 targetZ = m_position.z + (rng.nextDouble() - 0.5) * 8.0 - direction.z * 16.0;

    // 尝试瞬移
    bool success = attemptTeleport(targetX, targetY, targetZ, true);

    if (success) {
        m_teleportCooldown = TELEPORT_COOLDOWN;
    }

    return success;
}

bool EndermanEntity::teleportAwayFromWater()
{
    // MC 1.16.5: 瞬移避开水
    // 尝试多次瞬移，直到找到一个不在水中的位置
    for (i32 i = 0; i < 10; ++i) {
        if (teleport()) {
            // 检查是否还在水中
            if (!isInWater() && !isInLava()) {
                return true;
            }
        }
    }
    return false;
}

void EndermanEntity::placeHeldBlock()
{
    // MC 1.16.5 EndermanEntity.placeBlock()
    // 注意：实际的放置逻辑由 EndermanPlaceBlockGoal 处理
    // 这个方法作为一个 API 入口，可以被外部调用或测试
    if (!m_holdingBlock || m_heldBlockState == nullptr) {
        return;
    }

    // 委托给 AI 目标处理
    // 实际逻辑在 EndermanGoals.cpp 的 EndermanPlaceBlockGoal::tick() 中
    // 该方法保留作为外部接口
}

void EndermanEntity::pickUpBlock()
{
    // MC 1.16.5 EndermanEntity.takeBlock()
    // 注意：实际的拾取逻辑由 EndermanTakeBlockGoal 处理
    // 这个方法作为一个 API 入口，可以被外部调用或测试
    if (m_holdingBlock) {
        return;
    }

    // 委托给 AI 目标处理
    // 实际逻辑在 EndermanGoals.cpp 的 EndermanTakeBlockGoal::tick() 中
    // 该方法保留作为外部接口
}

bool EndermanEntity::isInWaterOrRain() const
{
    // MC 1.16.5: Entity.isInWaterOrRainOrBubbleColumn()
    // 对于末影人，气泡柱不会造成伤害，所以只检查水和雨
    // 参考 Entity.isWet() = isInWater() || isInRain()
    return isInWater() || isInRain();
}

bool EndermanEntity::shouldAttackPlayer(const Player& player) const
{
    // MC 1.16.5: EndermanEntity.shouldAttackPlayer()
    // 检查玩家是否正在注视末影人的眼睛

    // 1. 检查玩家是否戴着南瓜头
    // MC 1.16.5: ItemStack.isEnderMask()
    // 戴着南瓜头的玩家不会激怒末影人
    if (player.isWearingPumpkin()) {
        return false;
    }

    // 2. 检查玩家是否正在注视末影人
    if (!player.isLookingAt(*this)) {
        return false;
    }

    // 3. 检查视线是否被方块阻挡
    // MC 1.16.5: player.canEntityBeSeen(this)
    // 但在 shouldAttackPlayer 中，先检查注视再检查视线
    // 注视检测已经包含了方向检测，这里只需要确认没有方块阻挡
    return player.canSee(*this);
}

void EndermanEntity::tick()
{
    // MC 1.16.5 EndermanEntity.tick()
    MonsterEntity::tick();

    // 更新瞬移冷却
    if (m_teleportCooldown > 0) {
        m_teleportCooldown--;
    }

    // 更新复仇计时器
    if (m_revengeTimer > 0) {
        m_revengeTimer--;
        if (m_revengeTimer <= 0) {
            m_revengeTargetId = std::nullopt;
        }
    }

    // 更新愤怒时间
    if (m_angerTime > 0) {
        m_angerTime--;
        if (m_angerTime <= 0) {
            m_angry = false;
            m_screaming = false;
        }
    }

    // 检查水/雨伤害
    // MC 1.16.5: 在水中或雨中受到伤害并瞬移
    if (isInWaterOrRain()) {
        // MC 1.16.5: 每tick在水中受到1.0伤害
        auto damageSource = DamageSources::drown();
        hurt(damageSource, WATER_DAMAGE);
        teleportAwayFromWater();
    }

    // 注视检测由 EndermanFindPlayerGoal 和 EndermanStareGoal 处理
}

bool EndermanEntity::hurt(DamageSource& source, f32 amount)
{
    // MC 1.16.5 EndermanEntity.attackEntityFrom()

    // 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 投射物伤害：尝试64次随机瞬移，成功则躲避伤害
    // MC 1.16.5: if (source instanceof IndirectEntityDamageSource)
    // 使用 isProjectile() 检测投射物伤害
    if (source.isProjectile()) {
        // MC 1.16.5: for(int i = 0; i < 64; ++i) { if (this.teleportRandomly()) { return true; } }
        for (i32 i = 0; i < TELEPORT_PROJECTILE_ATTEMPTS; ++i) {
            if (teleport()) {
                return true; // 成功瞬移后不受伤
            }
        }
        return false; // 64次都失败，不受伤
    }

    // 调用父类的 hurt 方法处理实际伤害
    bool hurtResult = MonsterEntity::hurt(source, amount);

    if (hurtResult) {
        // 非生物伤害（摔落、窒息、岩浆等）：90%概率随机瞬移
        // MC 1.16.5: if (!this.world.isRemote && !(source.getTrueSource() instanceof LivingEntity) && this.rand.nextInt(10) != 0)
        if (m_world != nullptr && !m_world->isClientSide()) {
            Entity* trueSource = source.getTrueSource();
            bool isLivingSource = (trueSource != nullptr && dynamic_cast<LivingEntity*>(trueSource) != nullptr);

            if (!isLivingSource) {
                // 使用 getRandom() 获取随机数生成器
                math::Random rng = getRandom();
                // nextInt(10) != 0 意味着 90% 概率（10次中有9次）
                if (rng.nextInt(10) != 0) {
                    teleport();
                }
            }
        }
    }

    return hurtResult;
}

void EndermanEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // MC 1.16.5 EndermanEntity.registerGoals()
    // 优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: EndermanStareGoal (注视玩家目标)
    // 2: MeleeAttackGoal (攻击目标)
    // 5: WaterAvoidingRandomWalkingGoal (避水随机行走) - TODO
    // 7: LookAtGoal (看向玩家，但会激怒末影人)
    // 8: LookRandomlyGoal (随机看向)
    // 10: PlaceBlockGoal (放置方块)
    // 11: TakeBlockGoal (拾取方块)
    //
    // 目标选择器：
    // 1: EndermanFindPlayerGoal (查找注视玩家)
    // 2: HurtByTargetGoal (被攻击反击)
    // 3: NearestAttackableTargetGoal<EndermiteEntity> (攻击末影螨)
    // 4: ResetAngerGoal (重置愤怒) - TODO

    // 优先级 1: 注视玩家目标（当被注视时停止移动并注视玩家）
    m_goalSelector.addGoal(1, new entity::ai::goal::EndermanStareGoal(this));

    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 优先级 7: 看向玩家（会激怒末影人）
    m_goalSelector.addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->legacyType() == LegacyEntityType::Player;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 优先级 10: 放置方块目标
    m_goalSelector.addGoal(10, new entity::ai::goal::EndermanPlaceBlockGoal(this));

    // 优先级 11: 拾取方块目标
    m_goalSelector.addGoal(11, new entity::ai::goal::EndermanTakeBlockGoal(this));

    // 目标选择器
    // 优先级 1: 查找正在注视末影人的玩家
    m_targetSelector.addGoal(1, new entity::ai::goal::EndermanFindPlayerGoal(this));

    // 优先级 2: HurtByTargetGoal 已在父类 MonsterEntity::registerGoals() 中注册

    // 优先级 3: 攻击末影螨
    // MC 1.16.5: NearestAttackableTargetGoal<>(this, EndermiteEntity.class, 10, true, false, predicate)
    // 只攻击玩家生成的末影螨（通过末影珍珠传送生成）
    m_targetSelector.addGoal(3,
        new entity::ai::goal::NearestAttackableTargetGoal<EndermiteEntity>(
            this,
            true,  // checkSight - 需要视线可见
            0,     // chance - 每 tick 检查
            [](const LivingEntity* entity) -> bool {
                // MC 1.16.5: field_213627_bA - 只攻击玩家生成的末影螨
                if (entity == nullptr || !entity->isAlive()) {
                    return false;
                }
                const EndermiteEntity* endermite = dynamic_cast<const EndermiteEntity*>(entity);
                if (endermite == nullptr) {
                    return false;
                }
                // 只有玩家生成的末影螨才会被末影人攻击
                return endermite->isSpawnedByPlayer();
            }));

    // 优先级 4: ResetAngerGoal (重置愤怒) - TODO
}

void EndermanEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // MC 1.16.5 EndermanEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

} // namespace mc
