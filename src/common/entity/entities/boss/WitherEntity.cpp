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
#include "../../../item/Items.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/MathConstants.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockState.hpp"
#include "../../../world/block/BlockTags.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../ai/controller/FlyingMovementController.hpp"
#include "../../ai/goal/Goal.hpp"
#include "../../ai/goal/GoalFlag.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../ai/pathfinding/PathNavigator.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../effect/EffectType.hpp"
#include "../../entities/item/ItemEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../entities/projectile/AbstractFireballEntity.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

// ========== 静态数据参数定义 ==========
DataParameter<i32> WitherEntity::HEAD_TARGET_1 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_2 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::HEAD_TARGET_3 = EntityDataManager::createKey<i32>();
DataParameter<i32> WitherEntity::INVULNERABILITY_TIME = EntityDataManager::createKey<i32>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = MobEntity::classInfo()）
// ============================================================================
const EntityClassInfo& WitherEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"WitherEntity", &MobEntity::classInfo()};
    return s_classInfo;
}

std::unique_ptr<Entity> WitherEntity::create(IWorld* world)
{
    return std::make_unique<WitherEntity>(EntityInstanceId(0));
}

WitherEntity::WitherEntity(EntityInstanceId id)
    : MobEntity(id)
{
    setExperienceValue(50);

    // 凋灵可以飞行（不受重力影响）
    setNoGravity(true);

    // 设置飞行移动控制器
    // maxTurn=10: 俯仰角每tick最大旋转10度
    // hoversInPlace=false: 空闲时恢复重力
    m_moveController = std::make_unique<ai::controller::FlyingMovementController>(this, 10, false);

    // 设置导航器可以游泳
    if (auto* nav = navigator()) {
        nav->setCanSwim(true);
    }

    registerData();
    registerAttributes();
    registerGoals();
}

std::optional<ResourceLocation> WitherEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WitherEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> WitherEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

bool WitherEntity::isInvulnerableTo(DamageSource& source) const
{
    // 凋灵免疫溺水伤害
    if (source.type() == DamageType::Drown) {
        return true;
    }

    // 凋灵免疫凋零伤害
    if (source.type() == DamageType::Wither) {
        return true;
    }

    // 无敌阶段免疫所有伤害（除了虚空伤害）
    if (getInvulTime() > 0 && source.type() != DamageType::OutOfWorld) {
        return true;
    }

    // 免疫其他凋灵的伤害
    Entity* trueSource = source.getTrueSource();
    if (trueSource != nullptr && trueSource != this) {
        // 检查攻击者是否也是凋灵
        if (trueSource->entityType() == entity::VanillaEntityTypeKeys::WITHER) {
            return true;
        }
    }

    // 充能状态（生命值≤一半）免疫箭矢伤害
    if (isCharged()) {
        Entity* immediateSource = source.directSource();
        if (immediateSource != nullptr) {
            // 检查是否是箭矢（包括普通箭、光灵箭、三叉戟等投射物）
            auto entityType = immediateSource->entityType();
            if (entityType == entity::VanillaEntityTypeKeys::ARROW ||
                entityType == entity::VanillaEntityTypeKeys::SPECTRAL_ARROW ||
                entityType == entity::VanillaEntityTypeKeys::TRIDENT) {
                return true;
            }
        }
    }

    // 亡灵生物不互相伤害（通过 CreatureAttribute 判断）
    // 注意：这个逻辑通常在 LivingEntity.attackEntityFrom() 中处理

    return MobEntity::isInvulnerableTo(source);
}

bool WitherEntity::hurt(DamageSource& source, f32 amount)
{
    // 先检查无敌
    if (isInvulnerableTo(source)) {
        return false;
    }

    // 如果伤害来源是凋灵（非玩家）且也是亡灵生物，不造成伤害
    Entity* trueSource = source.getTrueSource();
    if (trueSource != nullptr && trueSource != this) {
        // 检查攻击者是否是凋灵
        if (trueSource->entityType() == entity::VanillaEntityTypeKeys::WITHER) {
            return false;
        }
        // 检查攻击者是否是亡灵生物
        LivingEntity* livingSource = dynamic_cast<LivingEntity*>(trueSource);
        if (livingSource != nullptr && livingSource->getCreatureAttribute() == CreatureAttribute::Undead &&
            dynamic_cast<Player*>(trueSource) == nullptr) {
            // 亡灵生物（非玩家）攻击凋灵不造成伤害
            return false;
        }
    }

    // 调用父类处理实际伤害
    bool wasHurt = MobEntity::hurt(source, amount);

    if (wasHurt) {
        // 受伤后触发方块破坏计数器
        if (m_blockBreakCounter <= 0) {
            m_blockBreakCounter = BLOCK_BREAK_COOLDOWN;
        }

        // 增加头部空闲更新计数，使侧头更快发射
        for (i32 i = 0; i < 2; ++i) {
            m_idleHeadUpdates[i] += 3;
        }
    }

    return wasHurt;
}

bool WitherEntity::canRangedAttack() const
{
    // 无敌阶段不能远程攻击
    return getInvulTime() <= 0;
}

std::string WitherEntity::getBossName() const
{
    // 返回自定义名称或默认名称
    if (hasCustomName()) {
        return customNameText();
    }
    return "Wither";
}

i32 WitherEntity::getWatchedTargetId(i32 head) const
{
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
    // 调用父类注册数据参数
    MobEntity::registerData();

    // 标记当前正在注册 WitherEntity 类的字段，使 registerParam 沿 WitherEntity 继承链
    // 分配 id（续接 MobEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册三个头的追踪目标实体ID
    // 初始值为0表示无目标
    m_dataManager.registerParam(HEAD_TARGET_1, 0);
    m_dataManager.registerParam(HEAD_TARGET_2, 0);
    m_dataManager.registerParam(HEAD_TARGET_3, 0);

    // 注册无敌时间数据参数
    m_dataManager.registerParam(INVULNERABILITY_TIME, 0);
}

void WitherEntity::launchWitherSkullToEntity(i32 head, LivingEntity* target)
{
    if (!target || !target->isAlive()) {
        return;
    }

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 计算发射位置
    f32 headX = _getHeadX(head);
    f32 headY = _getHeadY(head);
    f32 headZ = _getHeadZ(head);

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
    // 主头有 0.1% 概率发射蓝色凋灵之首，充能状态下主头总是发射蓝色
    bool isBlue = false;
    if (head == 0) {
        math::Random& rng = getRandom();
        if (isCharged() || rng.nextFloat() < 0.001f) {
            isBlue = true;
        }
    }

    // 创建凋灵之首实体
    auto skull = std::make_unique<WitherSkullEntity>(EntityInstanceId(0));
    skull->setTypeId(EntityTypeKeys::WITHER_SKULL);
    skull->setPosition(Vector3(headX, headY, headZ));
    skull->setShooter(this);
    // 蓝色凋灵之首的运动因子为 0.73，普通为 0.95
    // shoot 方法的 velocity 参数：1.5 * 运动因子
    f32 velocity = isBlue ? 1.095f : 1.5f; // 1.5 * 0.73 = 1.095, 1.5 * 1.0 = 1.5
    skull->shoot(dx, dy, dz, velocity, 0.0f);
    skull->setBlue(isBlue);

    // 生成实体
    worldPtr->spawnEntity(std::move(skull));

    // 播放发射音效
    playSound(SoundEvents::ENTITY_WITHER_SHOOT, 1.0f, 1.0f);
}

void WitherEntity::ignite()
{
    // 开始生成序列
    setInvulTime(INVULNERABILITY_TIME_CONST);
    setHealth(maxHealth() / 3.0f);
}

void WitherEntity::tick()
{
    // 注意：侧头角度的 prev 备份（m_prevHeadXRot/m_prevHeadYRot）在 aiStep() 中执行，
    // 对应 MC 1.21.11 WitherBoss.aiStep() 中 super.aiStep() 之后的备份逻辑。
    // tick() → MobEntity::tick() → LivingEntity::tick() → aiStep()，
    // 因此备份发生在 tick 链内部，时序正确。

    // 调用父类tick
    MobEntity::tick();

    // 更新AI任务
    _updateAITasks();

    // 生成粒子效果
    _spawnParticles();
}

void WitherEntity::aiStep()
{
    // 更新飞行追踪行为（在控制器更新之前执行，匹配MC Java调用顺序）
    // MC Java: WitherBoss.aiStep() 在 super.aiStep() 之前做飞行计算
    // 这样 FlyingMovementController.tick() 的旋转限制能正确覆盖
    _updateFlightBehavior();

    // 调用父类aiStep（LivingEntity::aiStep() 处理物理移动等）
    // 注意：MobEntity::tick() 中的控制器更新在 LivingEntity::tick() 之后执行
    // 所以调用链是：aiStep() -> LivingEntity::aiStep() -> 物理移动
    // 然后 MobEntity::tick() 继续 -> m_moveController->tick() -> m_lookController->tick()
    LivingEntity::aiStep();

    // 保存上一帧侧头角度（对应 MC 1.21.11 WitherBoss.aiStep() 中
    // super.aiStep() 之后的 yRotOHeads[i]=yRotHeads[i]; xRotOHeads[i]=xRotHeads[i];）
    // 必须在 _updateSideHeadRotations() 之前执行，确保 prev 保存的是上一 tick 的值。
    for (i32 i = 0; i < 2; ++i) {
        m_prevHeadYRot[i] = m_headYRot[i];
        m_prevHeadXRot[i] = m_headXRot[i];
    }

    // 更新侧头朝向（对应 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 循环）
    // 在 LivingEntity::aiStep() 之后执行，与 MC 调用顺序一致。
    _updateSideHeadRotations();
}

void WitherEntity::_spawnParticles()
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 充能状态下生成额外的烟雾粒子
    // 3 个头都始终生成烟雾粒子（每 tick），充能时 1/4 概率额外 EntityEffect
    if (isCharged()) {
        // 每 2 tick 在每个头附近生成烟雾粒子
        if (ticksExisted() % 2 == 0) {
            for (i32 head = 0; head < 3; ++head) {
                f32 headX = _getHeadX(head);
                f32 headY = _getHeadY(head);
                f32 headZ = _getHeadZ(head);

                worldPtr->addParticle(particle::ParticleTypeId::Smoke,
                    Vector3(headX + (getRandom().nextDouble() - 0.5) * 0.3,
                        headY + (getRandom().nextDouble() - 0.5) * 0.3,
                        headZ + (getRandom().nextDouble() - 0.5) * 0.3),
                    Vector3(0.0, 0.0, 0.0));

                // 充能时 1/4 概率额外 EntityEffect 粒子（黄绿色 0.7, 0.7, 0.5）
                if (worldPtr->getRandom().nextInt(4) == 0) {
                    worldPtr->addParticle(particle::ParticleTypeId::EntityEffect,
                        Vector3(headX + (getRandom().nextDouble() - 0.5) * 0.6,
                            headY + (getRandom().nextDouble() - 0.5) * 0.6,
                            headZ + (getRandom().nextDouble() - 0.5) * 0.6),
                        Vector3(0.7f, 0.7f, 0.5f));
                }
            }
        }
    }

    // 无敌阶段生成紫色粒子（表示充能状态）
    // 颜色: (0.7, 0.7, 0.9)
    if (getInvulTime() > 0 && getInvulTime() % 8 == 0) {
        for (i32 i = 0; i < 3; ++i) {
            worldPtr->addParticle(particle::ParticleTypeId::EntityEffect,
                Vector3(x() + (getRandom().nextDouble() - 0.5) * width() * 2.0,
                    y() + getRandom().nextDouble() * height(),
                    z() + (getRandom().nextDouble() - 0.5) * width() * 2.0),
                Vector3(0.7f, 0.7f, 0.9f));
        }
    }
}

void WitherEntity::die(DamageSource& source)
{
    // 调用父类die()
    MobEntity::die(source);

    // 掉落下界之星
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 掉落 1 个下界之星，永不消失
    if (Items::NETHER_STAR != nullptr) {
        ItemStack netherStar(Items::NETHER_STAR, 1);
        math::Random& rng = getRandom();
        ItemEntity* itemEntity = ItemDropHelper::spawnItemAtEntity(this,
            netherStar,
            0.5f, // offsetY
            rng,
            ItemDropHelper::DEFAULT_PICKUP_DELAY);

        // 下界之星永不消失
        if (itemEntity != nullptr) {
            itemEntity->setLifetime(ItemEntity::INFINITE_LIFETIME);
        }
    }
}

bool WitherEntity::isPotionApplicable(const entity::effect::EffectInstance& effect) const
{
    // 凋灵免疫凋零效果
    if (effect.type() == entity::effect::EffectType::Wither) {
        return false;
    }
    return MobEntity::isPotionApplicable(effect);
}

bool WitherEntity::onLivingFall(f32 /*distance*/, f32 /*damageMultiplier*/)
{
    // 凋灵不受摔落伤害
    return false;
}

void WitherEntity::_updateAITasks()
{
    // 无敌阶段处理
    i32 invulTime = getInvulTime();
    if (invulTime > 0) {
        invulTime--;
        setInvulTime(invulTime);

        // 每10tick恢复10点生命值
        if (invulTime % 10 == 0) {
            heal(10.0f);
        }

        // 无敌阶段结束时的爆炸
        if (invulTime == 0) {
            _explodeOnSpawn();
        }
    }

    // 更新头部目标
    _updateHeadTargets();

    // 方块破坏逻辑
    if (m_blockBreakCounter > 0) {
        m_blockBreakCounter--;
        if (m_blockBreakCounter == 0) {
            _breakNearbyBlocks();
        }
    }

    // 生命值一半以下时持续恢复（每秒1HP）
    if (isCharged() && ticksExisted() % 20 == 0) {
        heal(1.0f);
    }

    // 侧头空闲攻击逻辑
    // 仅在 NORMAL 和 HARD 难度下触发
    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        auto difficulty = worldPtr->difficulty();
        if (difficulty == Difficulty::Normal || difficulty == Difficulty::Hard) {
            for (i32 i = 1; i < 3; ++i) {
                // 空闲攻击计数器递增已在上面的 _updateHeadTargets() 中处理
                // 当空闲次数超过15时，发射随机蓝色凋灵之首
                if (m_idleHeadUpdates[i - 1] > 15) {
                    // 在凋灵周围 10x5x10 范围内随机选一个坐标
                    math::Random& rng = getRandom();
                    f64 targetX = rng.nextDouble() * 20.0 - 10.0 + x();
                    f64 targetY = rng.nextDouble() * 10.0 - 5.0 + y();
                    f64 targetZ = rng.nextDouble() * 20.0 - 10.0 + z();

                    // 发射蓝色凋灵之首到随机位置
                    launchWitherSkullToPosition(i, targetX, targetY, targetZ, true);
                    m_idleHeadUpdates[i - 1] = 0;
                }
            }
        }
    }
}

void WitherEntity::_updateHeadTargets()
{
    // 更新三个头的目标
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
        math::Random& rng = getRandom();
        m_nextHeadUpdate[i - 1] = static_cast<i32>(ticksExisted()) + 10 + rng.nextInt(10);

        // 获取当前追踪目标
        i32 currentTargetId = getWatchedTargetId(i);
        if (currentTargetId > 0) {
            // 检查当前目标是否仍然有效
            Entity* currentTarget = worldPtr->getEntity(static_cast<EntityInstanceId>(currentTargetId));
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
        // 搜索范围 20x8x20，目标条件：非亡灵生物、可攻击、可见
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

            // 检查是否是创造模式玩家（创造模式和旁观者模式的玩家不能被作为目标）
            if (living->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
                Player* player = dynamic_cast<Player*>(living);
                if (player != nullptr && (player->isCreative() || player->isSpectator())) {
                    continue;
                }
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

f32 WitherEntity::_getHeadX(i32 head) const
{
    // 对应 MC 1.21.11 WitherBoss.getHeadX(int)：
    //   head <= 0: return this.getX()
    //   else: f = (yBodyRot + 180*(head-1)) * PI/180; return getX + cos(f) * 1.3 * getScale()
    // 此处 getScale() 对凋灵恒为 1.0（无幼体凋灵）。
    if (head <= 0) {
        return static_cast<f32>(x());
    }
    f32 angleDeg = renderYawOffset() + 180.0f * static_cast<f32>(head - 1);
    f32 angleRad = angleDeg * math::DEG_TO_RAD;
    return static_cast<f32>(x() + std::cos(angleRad) * 1.3);
}

f32 WitherEntity::_getHeadY(i32 head) const
{
    // 对应 MC 1.21.11 WitherBoss.getHeadY(int)：
    //   head <= 0: return getY + 3.0 * getScale()
    //   else:      return getY + 2.2 * getScale()
    // 此处 getScale() 对凋灵恒为 1.0。
    f32 yOffset = (head <= 0) ? 3.0f : 2.2f;
    return static_cast<f32>(y() + yOffset);
}

f32 WitherEntity::_getHeadZ(i32 head) const
{
    // 对应 MC 1.21.11 WitherBoss.getHeadZ(int)：
    //   head <= 0: return this.getZ()
    //   else: f = (yBodyRot + 180*(head-1)) * PI/180; return getZ + sin(f) * 1.3 * getScale()
    // 此处 getScale() 对凋灵恒为 1.0。
    if (head <= 0) {
        return static_cast<f32>(z());
    }
    f32 angleDeg = renderYawOffset() + 180.0f * static_cast<f32>(head - 1);
    f32 angleRad = angleDeg * math::DEG_TO_RAD;
    return static_cast<f32>(z() + std::sin(angleRad) * 1.3);
}

f32 WitherEntity::_rotLerp(f32 current, f32 target, f32 maxStep)
{
    // 对应 MC 1.21.11 WitherBoss.rotlerp(current, target, maxStep)：
    //   diff = Mth.wrapDegrees(target - current)
    //   diff = clamp(diff, -maxStep, maxStep)
    //   return current + diff
    // 使用 math::clampedRotate（与 MC rotlerp 完全等价）。
    return math::clampedRotate(current, target, maxStep);
}

void WitherEntity::_updateSideHeadRotations()
{
    // 对应 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 循环：
    //   int k = getAlternativeTarget(j + 1);
    //   Entity entity1 = (k > 0) ? level.getEntity(k) : null;
    //   if (entity1 != null) {
    //       double d9 = getHeadX(j+1), d1 = getHeadY(j+1), d3 = getHeadZ(j+1);
    //       double d4 = entity1.getX() - d9;     // 目标相对头部的 X 偏移
    //       double d5 = entity1.getEyeY() - d1;  // 目标眼睛 Y 偏移
    //       double d6 = entity1.getZ() - d3;     // 目标相对头部的 Z 偏移
    //       double d7 = sqrt(d4*d4 + d6*d6);     // 水平距离
    //       float f1 = atan2(d6, d4) * 180/PI - 90;  // 目标偏航角
    //       float f2 = -(atan2(d5, d7) * 180/PI);    // 目标俯仰角
    //       xRotHeads[j] = rotlerp(xRotHeads[j], f2, 40);  // pitch 最大 40°/tick
    //       yRotHeads[j] = rotlerp(yRotHeads[j], f1, 10);  // yaw 最大 10°/tick
    //   } else {
    //       yRotHeads[j] = rotlerp(yRotHeads[j], yBodyRot, 10);  // 无目标时回正到身体朝向
    //   }
    IWorld* worldPtr = world();
    f32 bodyRot = renderYawOffset();

    for (i32 j = 0; j < 2; ++j) {
        i32 targetId = getWatchedTargetId(j + 1);
        Entity* targetEntity = nullptr;
        if (targetId > 0 && worldPtr != nullptr) {
            targetEntity = worldPtr->getEntity(static_cast<EntityInstanceId>(targetId));
        }

        if (targetEntity != nullptr) {
            f64 headX = _getHeadX(j + 1);
            f64 headY = _getHeadY(j + 1);
            f64 headZ = _getHeadZ(j + 1);

            f64 dx = targetEntity->x() - headX;
            // MC 1.21.11: entity1.getEyeY() = entity1.getY() + entity1.getEyeHeight()
            f64 dy = targetEntity->getEyeY() - headY;
            f64 dz = targetEntity->z() - headZ;

            f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

            // 偏航角：atan2(dz, dx) * 180/PI - 90
            f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * (180.0 / math::PI) - 90.0);
            // 俯仰角：-(atan2(dy, horizontalDist) * 180/PI)
            f32 targetPitch = static_cast<f32>(-(std::atan2(dy, horizontalDist) * (180.0 / math::PI)));

            m_headXRot[j] = _rotLerp(m_headXRot[j], targetPitch, 40.0f);
            m_headYRot[j] = _rotLerp(m_headYRot[j], targetYaw, 10.0f);
        } else {
            // 无目标：偏航角逐步回正到身体朝向（俯仰角保持不变，与 MC 一致）
            m_headYRot[j] = _rotLerp(m_headYRot[j], bodyRot, 10.0f);
        }
    }
}

void WitherEntity::_breakNearbyBlocks()
{
    // 破坏凋灵周围 3x4x3 范围内的方块（排除 WITHER_IMMUNE 标签中的方块）

    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 检查 mobGriefing 游戏规则
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }

    // 破坏范围 x: -1 到 1, y: 0 到 3, z: -1 到 1
    i32 baseX = static_cast<i32>(std::floor(x()));
    i32 baseY = static_cast<i32>(std::floor(y()));
    i32 baseZ = static_cast<i32>(std::floor(z()));

    bool anyBlockBroken = false;

    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dy = 0; dy <= 3; ++dy) {
            for (i32 dz = -1; dz <= 1; ++dz) {
                BlockPos pos(baseX + dx, baseY + dy, baseZ + dz);

                // 获取方块状态
                const BlockState* state = worldPtr->getBlockState(pos);
                if (state == nullptr || state->isAir()) {
                    continue;
                }

                // 检查方块是否可以被凋灵破坏
                // 判断条件为 !isAir() && !is(WITHER_IMMUNE)
                // 注：MC 1.21.11 中已不存在 BlockState.canEntityDestroy() 方法，
                // 凋灵和末影龙的方块破坏完全依赖标签系统。
                const Block& block = state->getBlock();

                // 检查是否在 WITHER_IMMUNE 标签中
                if (BlockTags::WITHER_IMMUNE().contains(block)) {
                    continue;
                }

                // 将方块设置为空气，并掉落物品
                const BlockState* airState =
                    VanillaBlocks::AIR != nullptr ? &VanillaBlocks::AIR->defaultState() : nullptr;

                if (airState != nullptr) {
                    // 设置为空气方块，flags=3 表示通知邻居并更新客户端
                    // 调用 spawnAfterBreak 以支持虫蚀方块等特殊行为（凋灵破坏方块不使用工具，不产生经验）
                    const BlockState* oldState = state;
                    worldPtr->setBlockState(pos, airState, 3);
                    block.spawnAfterBreak(*worldPtr, pos, *oldState, nullptr, false);
                    anyBlockBroken = true;
                }
            }
        }
    }

    // 如果有方块被破坏，播放破坏音效
    if (anyBlockBroken) {
        playSound(SoundEvents::ENTITY_WITHER_BREAK_BLOCK, 1.0f, 1.0f);
    }
}

void WitherEntity::_explodeOnSpawn()
{
    // 生成时创建7.0威力的爆炸
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 检查 mobGriefing 游戏规则决定爆炸模式
    world::explosion::ExplosionMode mode =
        worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)
        ? world::explosion::ExplosionMode::Destroy
        : world::explosion::ExplosionMode::None;

    // 创建爆炸
    // 爆炸半径 7.0，不生成火焰
    worldPtr->createExplosion(position(),
        game::explosion::WITHER_SPAWN_RADIUS, // 7.0f
        mode,
        false, // 不生成火焰
        this   // 爆炸源
    );

    // 播放全局音效
    // 这需要通过世界广播给所有玩家
    playSound(SoundEvents::ENTITY_WITHER_SPAWN, 1.0f, 1.0f);
}

void WitherEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 /*charge*/)
{
    // 主头发射凋灵之首
    launchWitherSkullToEntity(0, target);
}

void WitherEntity::registerGoals()
{
    MobEntity::registerGoals();

    // 优先级 0: 无敌阶段什么都不做
    // DoNothingGoal 阻止移动、跳跃和看向
    m_goalSelector.addGoal(0, new WitherDoNothingGoal(this));

    // 优先级 2: 远程攻击（主头发射凋灵之首）
    // 使用 IRangedAttackMob 接口的 attackEntityWithRangedAttack
    m_goalSelector.addGoal(2,
        new entity::ai::goal::RangedAttackGoal(this,
            1.0,  // 移动速度倍率
            40,   // 最小攻击间隔 (ticks)
            60,   // 最大攻击间隔 (ticks)
            20.0f // 攻击半径
            ));

    // 优先级 5: 避水随机飞行
    // 由于 WitherEntity 继承自 MobEntity 而非 CreatureEntity，
    // 不能直接使用 WaterAvoidingRandomFlyingGoal（它要求 CreatureEntity*），
    // 因此使用专用 WitherRandomFlyGoal 实现类似效果。
    m_goalSelector.addGoal(5, new WitherRandomFlyGoal(this));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(
        6, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            // 只看向玩家
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 7: 随机看向
    m_goalSelector.addGoal(7, new entity::ai::goal::LookRandomlyGoal(this));

    // ========== 目标选择器 ==========

    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this));

    // 优先级 2: 攻击非亡灵生物
    // NOT_UNDEAD 谓词：排除亡灵生物
    m_targetSelector.addGoal(2,
        new entity::ai::goal::NearestAttackableTargetGoal<MobEntity>(this,
            false, // checkSight
            0,     // chance (每tick检查)
            [](const LivingEntity* entity) -> bool {
                if (entity == nullptr || !entity->isAlive()) {
                    return false;
                }
                // 排除亡灵生物
                return entity->getCreatureAttribute() != CreatureAttribute::Undead;
            }));
}

// ========== WitherRandomFlyGoal 实现 ==========

WitherRandomFlyGoal::WitherRandomFlyGoal(WitherEntity* wither)
    : ai::Goal(EnumSet<ai::GoalFlag>{ai::GoalFlag::Move})
    , m_wither(wither)
{}

bool WitherRandomFlyGoal::shouldExecute()
{
    if (m_wither == nullptr || m_wither->isBeingRidden()) {
        return false;
    }

    // 无敌阶段不执行随机飞行
    if (m_wither->isInvulnerablePhase()) {
        return false;
    }

    // 执行概率 0.001
    math::Random& rng = m_wither->getRandom();
    if (rng.nextFloat() >= 0.001f) {
        return false;
    }

    // 尝试生成飞行目标位置
    return _generateFlightTarget();
}

bool WitherRandomFlyGoal::shouldContinueExecuting()
{
    if (m_wither == nullptr || m_wither->isBeingRidden()) {
        return false;
    }

    if (m_wither->isInvulnerablePhase()) {
        return false;
    }

    // 检查导航路径是否仍在执行
    auto* nav = m_wither->navigator();
    if (nav && nav->hasPath() && !nav->isDone()) {
        return true;
    }

    // 检查移动控制器是否仍在移动
    auto* moveCtrl = m_wither->moveController();
    if (moveCtrl && moveCtrl->isUpdating()) {
        return true;
    }

    return false;
}

void WitherRandomFlyGoal::startExecuting()
{
    // 使用导航器移动到目标位置
    if (auto* nav = m_wither->navigator()) {
        static_cast<void>(nav->moveTo(
            static_cast<f64>(m_targetX), static_cast<f64>(m_targetY), static_cast<f64>(m_targetZ), m_speed));
    }
    m_hasTarget = true;
}

void WitherRandomFlyGoal::resetTask()
{
    m_wither->clearNavigation();
    m_hasTarget = false;
}

void WitherRandomFlyGoal::tick()
{
    // 无额外 tick 逻辑，导航器和移动控制器自动处理飞行
}

bool WitherRandomFlyGoal::_generateFlightTarget()
{
    // 生成策略：在凋灵前方 PI/2 弧度锥形范围内随机选一个空气位置，避开水和岩浆
    // 由于 WitherEntity 继承自 MobEntity 而非 CreatureEntity，
    // 不能使用 RandomPositionGenerator（它要求 CreatureEntity*），
    // 因此实现专用飞行目标生成逻辑。

    IWorld* worldPtr = m_wither->world();
    if (worldPtr == nullptr) {
        return false;
    }

    math::Random& rng = m_wither->getRandom();
    f64 srcX = m_wither->x();
    f64 srcY = m_wither->y();
    f64 srcZ = m_wither->z();

    // 获取凋灵面朝方向的水平分量
    f32 yawRad = m_wither->yaw() * math::DEG_TO_RAD;
    f32 lookX = -std::sin(yawRad);
    f32 lookZ = std::cos(yawRad);

    // 尝试最多 10 次生成有效位置
    for (i32 attempt = 0; attempt < 10; ++attempt) {
        // 在 PI/2 弧度（90度）锥形范围内随机偏移
        f32 angleOffset = (2.0f * rng.nextFloat() - 1.0f) * (math::PI / 2.0f);
        f32 baseAngle = std::atan2(lookZ, lookX);
        f32 finalAngle = baseAngle + angleOffset;

        // 水平距离：使用 sqrt 分布保证均匀分布（范围 0~8）
        f32 horizontalDist = std::sqrt(rng.nextFloat()) * 8.0f * math::SQRT2;

        // 垂直偏移范围 -2 到 +7
        f32 yOffset = rng.nextFloat() * 9.0f - 2.0f;

        f64 targetX = srcX + static_cast<f64>(horizontalDist * std::cos(finalAngle));
        f64 targetY = srcY + static_cast<f64>(yOffset);
        f64 targetZ = srcZ + static_cast<f64>(horizontalDist * std::sin(finalAngle));

        // 转换为方块坐标
        i32 blockX = math::floorTo<i32>(targetX);
        i32 blockY = math::floorTo<i32>(targetY);
        i32 blockZ = math::floorTo<i32>(targetZ);

        // 检查世界边界
        if (blockY < mc::world::MIN_BUILD_HEIGHT || blockY >= mc::world::MAX_BUILD_HEIGHT) {
            continue;
        }

        BlockPos pos(blockX, blockY, blockZ);

        // 检查目标方块是否为空气或非固体（飞行目标不需要可行走）
        const BlockState* state = worldPtr->getBlockState(pos);
        if (state != nullptr && !state->isAir()) {
            // 如果不是空气，检查上方是否为空气（向上寻找空间）
            bool foundAir = false;
            for (i32 up = 1; up <= 4; ++up) {
                BlockPos abovePos(blockX, blockY + up, blockZ);
                const BlockState* aboveState = worldPtr->getBlockState(abovePos);
                if (aboveState == nullptr || aboveState->isAir()) {
                    targetY = static_cast<f64>(blockY + up) + 0.5;
                    foundAir = true;
                    break;
                }
            }
            if (!foundAir) {
                continue;
            }
        }

        // 检查是否在水或岩浆中
        BlockPos checkPos(math::floorTo<i32>(targetX), math::floorTo<i32>(targetY), math::floorTo<i32>(targetZ));
        if (worldPtr->isWaterAt(checkPos) || worldPtr->isLavaAt(checkPos)) {
            continue;
        }

        m_targetX = static_cast<f32>(targetX);
        m_targetY = static_cast<f32>(targetY);
        m_targetZ = static_cast<f32>(targetZ);
        return true;
    }

    return false;
}

// ========== WitherDoNothingGoal 实现 ==========

WitherDoNothingGoal::WitherDoNothingGoal(WitherEntity* wither)
    : ai::Goal(EnumSet<ai::GoalFlag>{ai::GoalFlag::Move, ai::GoalFlag::Jump, ai::GoalFlag::Look})
    , m_wither(wither)
{}

bool WitherDoNothingGoal::shouldExecute()
{
    // 只在无敌阶段执行
    return m_wither != nullptr && m_wither->isInvulnerablePhase();
}

void WitherEntity::registerAttributes()
{
    MobEntity::registerAttributes();

    // 凋灵属性
    m_attributes.registerAttribute(*entity::attribute::Attributes::flyingSpeed());
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 300.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 4.0);
}

void WitherEntity::launchWitherSkullToPosition(i32 head, f64 targetX, f64 targetY, f64 targetZ, bool isBlue)
{
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 计算发射位置
    f32 headX = _getHeadX(head);
    f32 headY = _getHeadY(head);
    f32 headZ = _getHeadZ(head);

    // 计算发射方向
    f64 dx = targetX - headX;
    f64 dy = targetY - headY;
    f64 dz = targetZ - headZ;
    f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (dist > 0.0) {
        dx /= dist;
        dy /= dist;
        dz /= dist;
    }

    // 创建凋灵之首实体
    auto skull = std::make_unique<WitherSkullEntity>(EntityInstanceId(0));
    skull->setTypeId(EntityTypeKeys::WITHER_SKULL);
    skull->setPosition(Vector3(headX, headY, headZ));
    skull->setShooter(this);
    // 蓝色凋灵之首的运动因子为 0.73，普通为 0.95
    f32 velocity = isBlue ? 1.095f : 1.5f; // 1.5 * 0.73 = 1.095, 1.5 * 1.0 = 1.5
    skull->shoot(static_cast<f32>(dx), static_cast<f32>(dy), static_cast<f32>(dz), velocity, 0.0f);
    skull->setBlue(isBlue);

    // 生成实体
    worldPtr->spawnEntity(std::move(skull));

    // 播放发射音效
    playSound(SoundEvents::ENTITY_WITHER_SHOOT, 1.0f, 1.0f);
}

void WitherEntity::_updateFlightBehavior()
{
    // Y轴阻尼：保留60%的Y轴速度（两端均执行）
    Vector3 velocity = this->velocity();
    velocity.y *= 0.6;

    // 当主头有目标时，执行追踪飞行逻辑（仅服务端）
    IWorld* worldPtr = world();
    if (worldPtr && !worldPtr->isClientSide()) {
        i32 targetId = getWatchedTargetId(0);
        if (targetId > 0) {
            Entity* targetEntity = worldPtr->getEntity(static_cast<EntityInstanceId>(targetId));
            if (targetEntity != nullptr) {
                f64 dY = velocity.y;

                // Y轴追踪逻辑：
                // 非充能时：凋灵低于目标Y+5.0时上升
                // 充能时：凋灵低于目标Y时即上升
                if (y() < targetEntity->y() || (!isCharged() && y() < targetEntity->y() + 5.0)) {
                    // 确保Y速度不为负
                    dY = std::max(0.0, dY);
                    // 附加推力：0.3 - 当前Y速度 * 0.6
                    // 这形成渐进上升，收敛于约0.5/tick的上升速度
                    dY += 0.3 - dY * 0.6;
                }

                velocity.y = dY;

                // 水平追踪逻辑：
                // 只有水平距离大于3格时才追踪
                Vector3 horizontalDir(
                    static_cast<f32>(targetEntity->x() - x()), 0.0f, static_cast<f32>(targetEntity->z() - z()));
                f64 horizontalDistSq = static_cast<f64>(horizontalDir.x) * horizontalDir.x +
                    static_cast<f64>(horizontalDir.z) * horizontalDir.z;
                if (horizontalDistSq > 9.0) {
                    // 归一化水平方向
                    f64 horizontalDist = std::sqrt(horizontalDistSq);
                    f32 normX = static_cast<f32>(horizontalDir.x / horizontalDist);
                    f32 normZ = static_cast<f32>(horizontalDir.z / horizontalDist);
                    // 追踪推力 = 目标方向 * 0.3 - 当前水平速度 * 0.6
                    velocity.x += normX * 0.3f - velocity.x * 0.6f;
                    velocity.z += normZ * 0.3f - velocity.z * 0.6f;
                }
            }
        }
    }

    setVelocity(velocity);

    // 当有水平速度时，自动面向运动方向（两端均执行）
    velocity = this->velocity();
    f64 horizontalSpeedSq = static_cast<f64>(velocity.x) * velocity.x + static_cast<f64>(velocity.z) * velocity.z;
    if (horizontalSpeedSq > 0.05) {
        setRotation(static_cast<f32>(std::atan2(velocity.z, velocity.x) * (180.0 / math::PI) - 90.0), pitch());
    }
}

} // namespace entity
} // namespace mc
