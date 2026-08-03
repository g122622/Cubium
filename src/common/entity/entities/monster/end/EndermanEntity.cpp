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
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/damage/DamageSource.hpp"
#include "entity/registry/VanillaEntityTypeKeys.hpp"
#include "util/math/MathConstants.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include <cmath>
#include <memory>
#include <optional>

namespace mc {

EndermanEntity::EndermanEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 末影人不在阳光下燃烧
    setBurnsInDaylight(false);

    // 末影人可以走上1格高的方块
    setStepHeight(1.0f);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 EndermanEntity::registerData），
    // 必须在派生类构造函数中显式调用，参考 WolfEntity 模式。
    registerData();
}

// 网络同步数据参数定义
entity::DataParameter<i32> EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM =
    entity::EntityDataManager::createKey<i32>();
entity::DataParameter<bool> EndermanEntity::DATA_SCREAMING_PARAM = entity::EntityDataManager::createKey<bool>();

const entity::EntityClassInfo& EndermanEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"EndermanEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

void EndermanEntity::registerData()
{
    // 调用父类方法，确保基类数据参数已注册
    MonsterEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册搬方块状态参数：stateId（0 = 未持有方块）
    // 对应 MC 1.21.11 EnderMan.defineSynchedData() 中的 DATA_CARRY_STATE
    m_dataManager.registerParam(DATA_CARRIED_BLOCK_STATE_ID_PARAM, static_cast<i32>(0));

    // 注册注视状态参数
    // 对应 MC 1.21.11 EnderMan.defineSynchedData() 中的 DATA_CREEPY
    m_dataManager.registerParam(DATA_SCREAMING_PARAM, false);
}

std::unique_ptr<Entity> EndermanEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EndermanEntity>(EntityInstanceId(0));
}

std::optional<ResourceLocation> EndermanEntity::getAmbientSound() const
{
    // 愤怒时返回 ambient，被注视时返回 scream
    if (isScreaming()) {
        return makeSoundEventId("scream");
    }
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> EndermanEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> EndermanEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> EndermanEntity::getStareSound() const
{
    return makeSoundEventId("stare");
}

std::optional<ResourceLocation> EndermanEntity::getTeleportSound() const
{
    return makeSoundEventId("teleport");
}

void EndermanEntity::setRevengeTarget(LivingEntity* target)
{
    setAttackTarget(target);
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
    if (angry) {
        // 对应 MC 1.21.11 EnderMan.setTarget(target) 中 target != null 分支：
        //   this.entityData.set(DATA_CREEPY, true);
        // 愤怒时设置 screaming 状态，使客户端模型呈现攻击姿态（张嘴）。
        setScreaming(true);
    } else {
        // 对应 MC 1.21.11 EnderMan.setTarget(null) 中 target == null 分支：
        //   this.entityData.set(DATA_CREEPY, false);
        // 同时清除愤怒时间和攻击目标。
        m_angerTime = 0;
        setAttackTarget(nullptr);
        setScreaming(false);
    }
}

const BlockState* EndermanEntity::getHeldBlockState() const
{
    const i32 stateId = m_dataManager.get<i32>(DATA_CARRIED_BLOCK_STATE_ID_PARAM);
    if (stateId <= 0) {
        return nullptr;
    }
    return ::mc::BlockRegistry::instance().getBlockState(static_cast<u32>(stateId));
}

void EndermanEntity::setHeldBlockState(const BlockState* state)
{
    const i32 stateId = (state != nullptr) ? static_cast<i32>(state->stateId()) : 0;
    m_dataManager.set(DATA_CARRIED_BLOCK_STATE_ID_PARAM, stateId);
}

bool EndermanEntity::teleport()
{
    if (m_teleportCooldown > 0) {
        return false;
    }

    // 末影人瞬移范围：64 格
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
    if (attackTarget() == nullptr || m_teleportCooldown > 0) {
        return false;
    }

    // 计算远离目标的方向向量
    Vector3 direction(m_position.x - attackTarget()->position().x, 0.0, m_position.z - attackTarget()->position().z);

    // 归一化方向向量
    f32 length = direction.length();
    if (length > 0.001f) {
        direction.x /= length;
        direction.z /= length;
    } else {
        // 如果长度太小，随机选择方向
        math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
        f32 angle = rng.nextFloat() * math::TWO_PI;
        direction.x = std::cos(angle);
        direction.z = std::sin(angle);
    }

    // 目标位置：远离目标 16 格
    math::Random rng(static_cast<u64>(m_id) ^ static_cast<u64>(m_ticksExisted));
    f32 targetX = m_position.x + static_cast<f32>(rng.nextDouble() - 0.5) * 8.0f - direction.x * 16.0f;
    f32 targetY = m_position.y + static_cast<f32>(rng.nextInt(16) - 8);
    f32 targetZ = m_position.z + static_cast<f32>(rng.nextDouble() - 0.5) * 8.0f - direction.z * 16.0f;

    // 尝试瞬移
    bool success = attemptTeleport(targetX, targetY, targetZ, true);

    if (success) {
        m_teleportCooldown = TELEPORT_COOLDOWN;
    }

    return success;
}

bool EndermanEntity::teleportAwayFromWater()
{
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

bool EndermanEntity::isInWaterOrRain() const
{
    // 对于末影人，气泡柱不会造成伤害，所以只检查水和雨
    return isInWater() || isInRain();
}

bool EndermanEntity::shouldAttackPlayer(const Player& player) const
{
    // 检查玩家是否正在注视末影人的眼睛

    // 1. 检查玩家是否戴着南瓜头
    // 戴着南瓜头的玩家不会激怒末影人
    if (player.isWearingPumpkin()) {
        return false;
    }

    // 2. 检查玩家是否正在注视末影人
    if (!player.isLookingAt(*this)) {
        return false;
    }

    // 3. 检查视线是否被方块阻挡
    // 注视检测已经包含了方向检测，这里只需要确认没有方块阻挡
    return player.canSee(*this);
}

void EndermanEntity::tick()
{
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
            setScreaming(false);
        }
    }

    // 检查水/雨伤害
    // 在水中或雨中受到伤害并瞬移
    if (isInWaterOrRain()) {
        // 每tick在水中受到1.0伤害
        auto damageSource = DamageSources::drown();
        hurt(damageSource, WATER_DAMAGE);
        teleportAwayFromWater();
    }

    // 注视检测由 EndermanFindPlayerGoal 和 EndermanStareGoal 处理
}

bool EndermanEntity::hurt(DamageSource& source, f32 amount)
{
    // 检查无敌状态
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 投射物伤害：尝试64次随机瞬移，成功则躲避伤害
    // 使用 isProjectile() 检测投射物伤害
    if (source.isProjectile()) {
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
        if (m_world != nullptr && !m_world->isClientSide()) {
            Entity* trueSource = source.getTrueSource();
            bool isLivingSource = (trueSource != nullptr && dynamic_cast<LivingEntity*>(trueSource) != nullptr);

            if (!isLivingSource) {
                // 使用 getRandom() 获取随机数生成器
                math::Random& rng = getRandom();
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

    // AI 目标优先级顺序：
    // 0: SwimGoal (父类已注册)
    // 1: EndermanStareGoal (注视玩家目标)
    // 2: MeleeAttackGoal (攻击目标)
    // 5: WaterAvoidingRandomWalkingGoal (避水随机行走)
    // 7: LookAtGoal (看向玩家，但会激怒末影人)
    // 8: LookRandomlyGoal (随机看向)
    // 10: PlaceBlockGoal (放置方块)
    // 11: TakeBlockGoal (拾取方块)
    //
    // 目标选择器：
    // 1: EndermanFindPlayerGoal (查找注视玩家)
    // 2: HurtByTargetGoal (被攻击反击)
    // 3: NearestAttackableTargetGoal<EndermiteEntity> (攻击末影螨)
    // 4: ResetAngerGoal (重置愤怒)

    // 优先级 1: 注视玩家目标（当被注视时停止移动并注视玩家）
    m_goalSelector.addGoal(1, new entity::ai::goal::EndermanStareGoal(this));

    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 优先级 5: 避水随机行走
    m_goalSelector.addGoal(5, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));

    // 优先级 7: 看向玩家（会激怒末影人）
    m_goalSelector.addGoal(
        7, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
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

    // 优先级 2: 被攻击后反击
    // MC 原版: targetSelector.addGoal(2, HurtByTargetGoal(this))
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));

    // 优先级 3: 攻击末影螨
    // 只攻击玩家生成的末影螨（通过末影珍珠传送生成）
    m_targetSelector.addGoal(3,
        new entity::ai::goal::NearestAttackableTargetGoal<EndermiteEntity>(this,
            true, // checkSight - 需要视线可见
            0,    // chance - 每 tick 检查
            [](const LivingEntity* entity) -> bool {
                // 只攻击玩家生成的末影螨
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

    // 优先级 4: 重置愤怒
    // 当 UNIVERSAL_ANGER 游戏规则启用时，检查并处理愤怒目标
    m_targetSelector.addGoal(4, new entity::ai::goal::ResetAngerGoal<EndermanEntity>(this, false));
}

void EndermanEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 末影人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 7.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 64.0);
}

// ========== 寻路权重 ==========

f32 EndermanEntity::getPathWeight(f32 /*x*/, f32 /*y*/, f32 /*z*/) const
{
    // 末影人不依赖光照偏好，返回0.0f（中性）
    // 对应 MC EnderMan.getWalkTargetValue 返回 0.0F
    return 0.0f;
}

} // namespace mc
