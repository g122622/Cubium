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

#include "WitherEntity.hpp"
#include "../../../core/Constants.hpp"
#include "../../../core/Types.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../ai/goal/GoalFlag.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../ai/pathfinding/PathNavigator.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../entities/projectile/AbstractFireballEntity.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ========== 静态数据参数定义 ==========
// MC 1.16.5: FIRST_HEAD_TARGET, SECOND_HEAD_TARGET, THIRD_HEAD_TARGET
DataParameter<i32> WitherEntity::HEAD_TARGET_1 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_2 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_3 = EntityDataManager::createKey<i32>();

std::unique_ptr<Entity> WitherEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WitherEntity>(LegacyEntityType::Wither, EntityId(0));
}

WitherEntity::WitherEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
    // MC 1.16.5 WitherEntity 构造函数
    setExperienceValue(50);

    // MC 1.16.5: 凋灵可以飞行（不受重力影响）
    setNoGravity(true);

    // MC 1.16.5: 设置导航器可以游泳
    // 注意：SwimGoal 会在 MonsterEntity::registerGoals() 中自动设置 setCanSwim(true)
    // 这里显式设置以确保在导航器创建后立即生效
    if (auto* nav = navigator()) {
        nav->setCanSwim(true);
    }

    registerData();
    registerAttributes();
    registerGoals();
}

std::optional<ResourceLocation> WitherEntity::getAmbientSound() const
{
    // MC 1.16.5: entity.wither.ambient
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WitherEntity::getHurtSound(DamageSource& /*source*/) const
{
    // MC 1.16.5: entity.wither.hurt
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WitherEntity::getDeathSound() const
{
    // MC 1.16.5: entity.wither.death
    return makeSoundEventId("death");
}

bool WitherEntity::isInvulnerableTo(DamageSource& source) const
{
    // MC 1.16.5 WitherEntity.attackEntityFrom()
    // 凋灵免疫溺水伤害
    if (source.type() == DamageType::Drown) {
        return true;
    }

    // 凋灵免疫凋零伤害
    if (source.type() == DamageType::Wither) {
        return true;
    }

    // 无敌阶段免疫所有伤害（除了虚空伤害）
    if (m_invulTime > 0 && source.type() != DamageType::OutOfWorld) {
        return true;
    }

    // MC 1.16.5: 免疫其他凋灵的伤害
    Entity* trueSource = source.getTrueSource();
    if (trueSource != nullptr && trueSource != this) {
        // 检查攻击者是否也是凋灵
        if (trueSource->legacyType() == LegacyEntityType::Wither) {
            return true;
        }
    }

    // MC 1.16.5: 充能状态（生命值≤一半）免疫箭矢伤害
    if (isCharged()) {
        Entity* immediateSource = source.directSource();
        if (immediateSource != nullptr) {
            // 检查是否是箭矢（包括普通箭、光灵箭、三叉戟等投射物）
            // MC 1.16.5: if (entity instanceof AbstractArrowEntity)
            LegacyEntityType entityType = immediateSource->legacyType();
            if (entityType == LegacyEntityType::Arrow ||
                entityType == LegacyEntityType::SpectralArrow ||
                entityType == LegacyEntityType::Trident) {
                return true;
            }
        }
    }

    // MC 1.16.5: 亡灵生物不互相伤害（通过 CreatureAttribute 判断）
    // 注意：这个逻辑通常在 LivingEntity.attackEntityFrom() 中处理

    return MobEntity::isInvulnerableTo(source);
}

bool WitherEntity::canRangedAttack() const
{
    // 无敌阶段不能远程攻击
    return m_invulTime <= 0;
}

std::string WitherEntity::getBossName() const
{
    // MC 1.16.5: 返回自定义名称或默认名称
    // 参考 WitherEntity 构造函数中 bossInfo 使用 getDisplayName()
    if (hasCustomName()) {
        return customNameText();
    }
    return "Wither";
}

i32 WitherEntity::getWatchedTargetId(i32 head) const
{
    // MC 1.16.5: 从数据管理器获取头部目标
    // head: 0=主头, 1=左头, 2=右头
    switch (head) {
        case 0:
            return m_dataManager.get<i32>(HEAD_TARGET_1);
        case 1:
            return m_dataManager.get<i32>(HEAD_TARGET_2);
        case 2:
            return m_dataManager.get<i32>(HEAD_TARGET_3);
        default:
            return 0;
    }
}

void WitherEntity::updateWatchedTargetId(i32 head, i32 targetId)
{
    // MC 1.16.5: 更新数据管理器中的头部目标
    // head: 0=主头, 1=左头, 2=右头
    switch (head) {
        case 0:
            m_dataManager.set(HEAD_TARGET_1, targetId);
            break;
        case 1:
            m_dataManager.set(HEAD_TARGET_2, targetId);
            break;
        case 2:
            m_dataManager.set(HEAD_TARGET_3, targetId);
            break;
        default:
            break;
    }
}

void WitherEntity::registerData()
{
    // MC 1.16.5 WitherEntity.registerData()
    // 调用父类注册数据参数
    MobEntity::registerData();

    // 注册三个头的追踪目标实体ID
    // 初始值为0表示无目标
    m_dataManager.registerParam(HEAD_TARGET_1, 0);
    m_dataManager.registerParam(HEAD_TARGET_2, 0);
    m_dataManager.registerParam(HEAD_TARGET_3, 0);
}

void WitherEntity::launchWitherSkullToEntity(i32 head, LivingEntity* target)
{
    // MC 1.16.5: launchWitherSkullToEntity()
    if (!target || !target->isAlive()) {
        return;
    }

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 计算发射位置
    f32 headX = getHeadX(head);
    f32 headY = getHeadY(head);
    f32 headZ = getHeadZ(head);

    // 计算发射方向
    f32 dx = static_cast<f32>(target->x()) - headX;
    f32 dy = static_cast<f32>(target->y()) + target->eyeHeight() / 2.0f - headY;
    f32 dz = static_cast<f32>(target->z()) - headZ;

    f32 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > 0.0f) {
        dx /= dist;
        dy /= dist;
        dz /= dist;
    }

    // 决定是否发射蓝色凋灵之首
    // MC 1.16.5: 主头有 0.1% 概率发射蓝色凋灵之首，充能状态下主头总是发射蓝色
    bool isBlue = false;
    if (head == 0) {
        math::Random rng = getRandom();
        if (isCharged() || rng.nextFloat() < 0.001f) {
            isBlue = true;
        }
    }

    // 创建凋灵之首实体
    auto skull = std::make_unique<WitherSkullEntity>(LegacyEntityType::WitherSkull, EntityId(0));
    skull->setPosition(Vector3(headX, headY, headZ));
    skull->setShooter(this);
    // MC 1.16.5: 蓝色凋灵之首的运动因子为 0.73，普通为 0.95
    // shoot 方法的 velocity 参数：1.5 * 运动因子
    f32 velocity = isBlue ? 1.095f : 1.5f; // 1.5 * 0.73 = 1.095, 1.5 * 1.0 = 1.5
    skull->shoot(dx, dy, dz, velocity, 0.0f);
    skull->setBlue(isBlue);

    // 生成实体
    worldPtr->spawnEntity(std::move(skull));

    // 播放发射音效
    // MC 1.16.5: world.playEvent(1024, getPosition(), 0)
    playSound(SoundEvents::ENTITY_WITHER_SHOOT, 1.0f, 1.0f);
}

void WitherEntity::ignite()
{
    // MC 1.16.5: 开始生成序列
    m_invulTime = INVULNERABILITY_TIME;
    setHealth(maxHealth() / 3.0f);
}

void WitherEntity::tick()
{
    // MC 1.16.5 WitherEntity.tick()

    // 更新上一帧头部角度
    for (i32 i = 0; i < 2; ++i) {
        m_prevHeadXRot[i] = m_headXRot[i];
        m_prevHeadYRot[i] = m_headYRot[i];
    }

    // 调用父类tick
    MobEntity::tick();

    // 更新AI任务
    updateAITasks();
}

void WitherEntity::updateAITasks()
{
    // MC 1.16.5 WitherEntity.updateAITasks()

    // 无敌阶段处理
    if (m_invulTime > 0) {
        m_invulTime--;

        // 每10tick恢复10点生命值
        if (m_invulTime % 10 == 0) {
            heal(10.0f);
        }

        // 无敌阶段结束时的爆炸
        if (m_invulTime == 0) {
            explodeOnSpawn();
        }
    }

    // 更新头部目标
    updateHeadTargets();

    // 方块破坏逻辑
    if (m_blockBreakCounter > 0) {
        m_blockBreakCounter--;
        if (m_blockBreakCounter == 0) {
            breakNearbyBlocks();
        }
    }

    // 生命值一半以下时持续恢复
    if (isCharged() && ticksExisted() % 20 == 0) {
        heal(1.0f);
    }
}

void WitherEntity::updateHeadTargets()
{
    // MC 1.16.5: 更新三个头的目标
    // 主头追踪 getAttackTarget()
    // 侧头每20tick更新一次目标，追踪最近的非亡灵生物

    // 主头：直接追踪攻击目标
    LivingEntity* attackTarget = this->attackTarget();
    if (attackTarget != nullptr && attackTarget->isAlive()) {
        updateWatchedTargetId(0, static_cast<i32>(attackTarget->id()));
    } else {
        updateWatchedTargetId(0, 0);
    }

    // 侧头：周期性更新目标
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    for (i32 i = 1; i < 3; ++i) {
        // 检查更新时间
        if (static_cast<i32>(ticksExisted()) < m_nextHeadUpdate[i - 1]) {
            continue;
        }

        // 设置下次更新时间：10-20 tick后
        math::Random rng = getRandom();
        m_nextHeadUpdate[i - 1] = static_cast<i32>(ticksExisted()) + 10 + rng.nextInt(10);

        // 获取当前追踪目标
        i32 currentTargetId = getWatchedTargetId(i);
        if (currentTargetId > 0) {
            // 检查当前目标是否仍然有效
            Entity* currentTarget = worldPtr->getEntity(static_cast<EntityId>(currentTargetId));
            if (currentTarget != nullptr && currentTarget->isAlive()) {
                LivingEntity* livingTarget = dynamic_cast<LivingEntity*>(currentTarget);
                if (livingTarget != nullptr) {
                    // 检查距离和视线
                    f32 distSq = distanceSqTo(*livingTarget);
                    if (distSq <= 900.0f && canSee(*livingTarget)) { // 30格距离
                        // 目标有效，发射凋灵之首
                        launchWitherSkullToEntity(i, livingTarget);
                        // 设置攻击冷却：40-60 tick
                        m_nextHeadUpdate[i - 1] = static_cast<i32>(ticksExisted()) + 40 + rng.nextInt(20);
                        continue;
                    }
                }
            }
            // 目标无效，清除
            updateWatchedTargetId(i, 0);
        }

        // 无目标，搜索新目标
        // MC 1.16.5: 搜索范围 20x8x20，目标条件：非亡灵生物、可攻击、可见
        AxisAlignedBB searchBox = boundingBox().expand(20.0f, 8.0f, 20.0f);
        std::vector<Entity*> nearbyEntities = worldPtr->getEntitiesInAABB(searchBox, this);

        LivingEntity* bestTarget = nullptr;
        f32 bestDistSq = std::numeric_limits<f32>::max();

        for (Entity* entity : nearbyEntities) {
            if (entity == nullptr || entity == this) {
                continue;
            }

            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living == nullptr) {
                continue;
            }

            // 排除亡灵生物
            if (living->getCreatureAttribute() == CreatureAttribute::Undead) {
                continue;
            }

            // 检查可攻击性
            if (!living->isAlive()) {
                continue;
            }

            // 检查视线
            if (!canSee(*living)) {
                continue;
            }

            // 检查是否是创造模式玩家（创造模式玩家不可被攻击）
            if (living->legacyType() == LegacyEntityType::Player) {
                // TODO: 检查玩家的游戏模式
                // 暂时跳过这个检查
            }

            f32 distSq = distanceSqTo(*living);
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                bestTarget = living;
            }
        }

        if (bestTarget != nullptr) {
            updateWatchedTargetId(i, static_cast<i32>(bestTarget->id()));
        }
    }
}

f32 WitherEntity::getHeadX(i32 head) const
{
    // MC 1.16.5: 计算头的X坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(x());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(x() + 0.5 * std::cos(yaw() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(x() - 0.5 * std::cos(yaw() * math::DEG_TO_RAD));
    }
}

f32 WitherEntity::getHeadY(i32 head) const
{
    // MC 1.16.5: 计算头的Y坐标
    return static_cast<f32>(y() + 2.0);
}

f32 WitherEntity::getHeadZ(i32 head) const
{
    // MC 1.16.5: 计算头的Z坐标
    if (head == 0) {
        // 主头
        return static_cast<f32>(z());
    } else if (head == 1) {
        // 左头
        return static_cast<f32>(z() + 0.5 * std::sin(yaw() * math::DEG_TO_RAD));
    } else {
        // 右头
        return static_cast<f32>(z() - 0.5 * std::sin(yaw() * math::DEG_TO_RAD));
    }
}

void WitherEntity::breakNearbyBlocks()
{
    // MC 1.16.5: 破坏凋灵周围的方块
    // 检查 mobGriefing 游戏规则
    IWorld* worldPtr = world();
    if (!worldPtr || !worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }

    // MC 1.16.5: 破坏范围 3x4x3（以凋灵为中心）
    // 破坏凋灵周围 1 格范围，向上 3 格
    // 即: x: -1 到 1, y: 0 到 3, z: -1 到 1
    i32 baseX = static_cast<i32>(std::floor(x()));
    i32 baseY = static_cast<i32>(std::floor(y()));
    i32 baseZ = static_cast<i32>(std::floor(z()));

    // MC 1.16.5: 破坏凋灵免疫标签之外的方块
    // 简化实现：使用爆炸来破坏方块
    // 实际 MC 1.16.5 会检查 BlockTags.WITHER_IMMUNE 标签
    // 这里暂时简化为不实现单独的方块破坏逻辑，因为已经有爆炸系统了
    // TODO: 完整实现需要 BlockTags 系统和 destroyBlock 方法
}

void WitherEntity::explodeOnSpawn()
{
    // MC 1.16.5: 生成时创建7.0威力的爆炸
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // MC 1.16.5: 检查 mobGriefing 游戏规则决定爆炸模式
    world::explosion::ExplosionMode mode = worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
        ? world::explosion::ExplosionMode::Destroy
        : world::explosion::ExplosionMode::None;

    // 创建爆炸
    // MC 1.16.5: 爆炸半径 7.0，不生成火焰
    worldPtr->createExplosion(
        position(),
        game::explosion::WITHER_SPAWN_RADIUS, // 7.0f
        mode,
        false, // 不生成火焰
        this    // 爆炸源
    );

    // MC 1.16.5: 播放全局音效
    // world.playBroadcastSound(1023, getPosition(), 0)
    // 这需要通过世界广播给所有玩家
    playSound(SoundEvents::ENTITY_WITHER_SPAWN, 1.0f, 1.0f);
}

void WitherEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // MC 1.16.5: 主头发射凋灵之首
    launchWitherSkullToEntity(0, target);
}

void WitherEntity::registerGoals()
{
    MobEntity::registerGoals();

    // MC 1.16.5 WitherEntity.registerGoals()

    // 优先级 0: 无敌阶段什么都不做
    // DoNothingGoal 阻止移动、跳跃和看向
    m_goalSelector.addGoal(0, new WitherDoNothingGoal(this));

    // 优先级 2: 远程攻击（主头发射凋灵之首）
    // 使用 IRangedAttackMob 接口的 attackEntityWithRangedAttack
    m_goalSelector.addGoal(2, new entity::ai::goal::RangedAttackGoal(
        this,
        1.0,   // 移动速度倍率
        40,    // 最小攻击间隔 (ticks)
        60,    // 最大攻击间隔 (ticks)
        20.0f  // 攻击半径
    ));

    // 优先级 5: 避水随机行走
    // MC 1.16.5: WaterAvoidingRandomWalkingGoal(this, 1.0)
    // 注意: 凋灵可以飞行，这个目标主要用于地面移动
    // 暂时不添加，因为凋灵需要特殊的飞行移动逻辑

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->legacyType() == LegacyEntityType::Player;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

    // ========== 目标选择器 ==========
    // MC 1.16.5: targetSelector

    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this));

    // 优先级 2: 攻击非亡灵生物
    // MC 1.16.5: NearestAttackableTargetGoal<MobEntity>(this, MobEntity.class, 0, false, false, NOT_UNDEAD)
    // NOT_UNDEAD 谓词：排除亡灵生物
    m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<MobEntity>(
        this,
        false,  // checkSight
        0,      // chance (每tick检查)
        [](const LivingEntity* entity) -> bool {
            if (entity == nullptr || !entity->isAlive()) {
                return false;
            }
            // 排除亡灵生物
            return entity->getCreatureAttribute() != CreatureAttribute::Undead;
        }
    ));
}

// ========== WitherDoNothingGoal 实现 ==========

WitherDoNothingGoal::WitherDoNothingGoal(WitherEntity* wither)
    : ai::Goal(EnumSet<ai::GoalFlag>{ai::GoalFlag::Move, ai::GoalFlag::Jump, ai::GoalFlag::Look})
    , m_wither(wither)
{
}

bool WitherDoNothingGoal::shouldExecute()
{
    // MC 1.16.5: 只在无敌阶段执行
    return m_wither != nullptr && m_wither->isInvulnerablePhase();
}

void WitherEntity::registerAttributes()
{
    MobEntity::registerAttributes();

    // MC 1.16.5 WitherEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 300.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0); // MC 1.16.5: 40 而非 64
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 4.0);         // MC 1.16.5: 4 点护甲
}

} // namespace entity
} // namespace mc
