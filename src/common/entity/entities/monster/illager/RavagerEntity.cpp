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

#include "RavagerEntity.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/RavagerGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/pathfinding/RavagerNodeProcessor.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <memory>

namespace mc {

RavagerEntity::RavagerEntity(EntityInstanceId id)
    : AbstractRaiderEntity(id)
{
    // 劫掠兽可以走上1格高的方块
    setStepHeight(1.0f);

    // 创建使用 RavagerNodeProcessor 的自定义导航器
    // 劫掠兽可以穿过树叶
    auto nodeProcessor = std::make_unique<entity::ai::pathfinding::RavagerNodeProcessor>();
    auto pathFinder = std::make_unique<entity::ai::pathfinding::PathFinder>(std::move(nodeProcessor));
    m_navigator = std::make_unique<entity::ai::pathfinding::PathNavigator>(std::move(pathFinder));
    m_navigator->setEntity(this);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> RavagerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<RavagerEntity>(EntityInstanceId(0));
}

void RavagerEntity::tick()
{
    AbstractRaiderEntity::tick();

    if (!isAlive()) return;

    // 更新速度属性（根据攻击状态调整）
    if (isMovementBlocked()) {
        // 禁止移动时速度为 0
        m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.0);
    } else {
        // 有攻击目标时速度更快
        f64 targetSpeed = attackTarget() != nullptr ? 0.35 : 0.3;
        f64 currentSpeed = m_attributes.getBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED);
        f64 newSpeed = math::lerp(0.1, currentSpeed, targetSpeed);
        m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, newSpeed);
    }

    // 碰撞时破坏树叶
    if (collidedHorizontally()) {
        _breakLeavesOnCollision();
    }

    // 更新咆哮状态
    if (m_roarTick > 0) {
        m_roarTick--;
        if (m_roarTick == 10) {
            // 咆哮第 10 tick 时执行伤害
            _roar();
        }
    }

    // 更新攻击动画
    if (m_attackTick > 0) {
        m_attackTick--;
    }

    // 更新眩晕状态
    if (m_stunTick > 0) {
        m_stunTick--;
        _spawnStunParticles();

        if (m_stunTick == 0) {
            // 眩晕结束时开始咆哮
            playSound(SoundEvents::ENTITY_RAVAGER_ROAR, 1.0f, 1.0f);
            m_roarTick = ROAR_DURATION;
        }
    }
}

bool RavagerEntity::isMovementBlocked() const
{
    // 攻击、眩晕或咆哮时不能移动
    return m_attackTick > 0 || m_stunTick > 0 || m_roarTick > 0;
}

bool RavagerEntity::canSee(const Entity& other) const
{
    // 眩晕或咆哮时不能看见目标
    if (m_stunTick > 0 || m_roarTick > 0) {
        return false;
    }
    return AbstractRaiderEntity::canSee(other);
}

bool RavagerEntity::attackEntityAsMob(LivingEntity& target)
{
    // 设置攻击动画
    m_attackTick = ATTACK_DURATION;

    // 播放攻击音效
    playSound(SoundEvents::ENTITY_RAVAGER_ATTACK, 1.0f, 1.0f);

    // 调用父类攻击方法
    return AbstractRaiderEntity::attackEntityAsMob(target);
}

void RavagerEntity::constructKnockBackVector(LivingEntity* target)
{
    // 如果正在咆哮，不做任何事
    if (m_roarTick > 0) {
        return;
    }

    math::Random& rng = getRandom();

    // 50% 概率眩晕或发射目标
    if (rng.nextDouble() < STUN_CHANCE) {
        // 眩晕
        m_stunTick = STUN_DURATION;
        playSound(SoundEvents::ENTITY_RAVAGER_STUNNED, 1.0f, 1.0f);

        // 眩晕时目标与劫掠兽碰撞
        if (target) {
            // 应用碰撞效果
            target->addVelocity(
                static_cast<f32>(x() - target->x()) * 0.1f, 0.0f, static_cast<f32>(z() - target->z()) * 0.1f);
            target->markHurt();
        }
    } else {
        // 发射目标
        _launchEntity(target);
    }
}

void RavagerEntity::_roar()
{
    if (!isAlive()) return;

    IWorld* worldPtr = world();
    if (!worldPtr) return;

    // 获取周围 4 格内的所有 LivingEntity
    AxisAlignedBB searchBox = boundingBox().grow(ROAR_RANGE);
    std::vector<Entity*> entities = worldPtr->getEntitiesInAABB(searchBox, this);

    for (Entity* entity : entities) {
        // 排除劫掠兽自己和掠夺者类实体
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (!living) continue;

        // 掠夺者类实体免疫咆哮伤害，但仍会被击退
        if (dynamic_cast<AbstractRaiderEntity*>(living)) {
            // 掠夺者免疫伤害，但仍会被击退
        } else {
            // 对非掠夺者造成伤害
            EntityDamageSource damageSource = DamageSources::mobAttack(this);
            living->hurt(damageSource, ROAR_DAMAGE);
        }

        // 击退所有实体
        _launchEntity(living);
    }

    // 生成咆哮粒子效果（客户端处理）
    // 这里播放音效作为替代
    playSound(SoundEvents::ENTITY_RAVAGER_ROAR, 1.0f, 1.0f);
}

void RavagerEntity::_launchEntity(Entity* entity)
{
    if (!entity) return;

    // 计算发射方向
    f64 dx = entity->x() - x();
    f64 dz = entity->z() - z();
    f64 distSq = dx * dx + dz * dz;

    // 避免除零
    if (distSq < 0.001) {
        distSq = 0.001;
    }

    f64 invDist = 1.0 / std::sqrt(distSq);
    f64 vx = dx * invDist * LAUNCH_POWER;
    f64 vz = dz * invDist * LAUNCH_POWER;

    entity->addVelocity(static_cast<f32>(vx), LAUNCH_Y_POWER, static_cast<f32>(vz));
    // 标记受伤（发射改变了实体速度，需要同步到客户端）
    // 对应 MC Java 中 Ravager.roar() 和 constructKnockBackVector() 里 hurtMarked = true
    entity->markHurt();
}

void RavagerEntity::_spawnStunParticles()
{
    // 眩晕时生成粒子效果
    // 1/6 概率生成 ENTITY_EFFECT 粒子
    // 颜色常量 STUNNED_COLOR = 8356754 即 RGB(127, 131, 146) -> R=0.498, G=0.514, B=0.573
    math::Random& rng = getRandom();
    if (rng.nextInt(6) != 0) return;

    IWorld* worldPtr = world();
    if (!worldPtr) return;

    // 计算粒子位置（在实体身体上方偏移）
    f32 renderYawOffsetRad = math::toRadians(renderYawOffset());
    f64 offsetX =
        -static_cast<f64>(width()) * std::sin(static_cast<f64>(renderYawOffsetRad)) + (rng.nextDouble() * 0.6 - 0.3);

    f64 offsetY = y() + static_cast<f64>(height()) - 0.3;

    f64 offsetZ =
        static_cast<f64>(width()) * std::cos(static_cast<f64>(renderYawOffsetRad)) + (rng.nextDouble() * 0.6 - 0.3);

    // 生成灰色效果粒子，颜色通过 velocity 向量传递 (R, G, B)
    worldPtr->addParticle(particle::ParticleTypeId::EntityEffect,
        Vector3(x() + offsetX, offsetY, z() + offsetZ),
        Vector3(0.498f, 0.514f, 0.573f));
}

void RavagerEntity::_breakLeavesOnCollision()
{
    IWorld* worldPtr = world();
    if (!worldPtr || !m_canBreakBlocks) return;

    // 检查 mobGriefing 游戏规则
    if (!worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return;
    }

    AxisAlignedBB searchBox = boundingBox().grow(0.2);

    // 计算方块范围
    i32 minX = math::floorTo<i32>(searchBox.minX);
    i32 maxX = math::floorTo<i32>(searchBox.maxX);
    i32 minY = math::floorTo<i32>(searchBox.minY);
    i32 maxY = math::floorTo<i32>(searchBox.maxY);
    i32 minZ = math::floorTo<i32>(searchBox.minZ);
    i32 maxZ = math::floorTo<i32>(searchBox.maxZ);

    bool brokeAny = false;

    for (i32 bx = minX; bx <= maxX; bx++) {
        for (i32 by = minY; by <= maxY; by++) {
            for (i32 bz = minZ; bz <= maxZ; bz++) {
                BlockPos pos(bx, by, bz);
                const BlockState* state = worldPtr->getBlockState(pos);

                if (!state) continue;

                // 只破坏树叶 (LeavesBlock)
                if (BlockTags::LEAVES().contains(*state)) {
                    // 设置为空气，掉落物品
                    // 调用 spawnAfterBreak 以支持特殊方块行为（掠夺者破坏不使用工具，不产生经验）
                    const Block& brokenBlock = state->getBlock();
                    const BlockState* airState = BlockRegistry::instance().airState();
                    worldPtr->setBlockState(pos, airState, 3);
                    brokenBlock.spawnAfterBreak(*worldPtr, pos, *state, nullptr, false);
                    brokeAny = true;
                }
            }
        }
    }

    // 如果没有破坏方块且在地面上，跳跃
    if (!brokeAny && onGround()) {
        jump();
    }
}

void RavagerEntity::registerGoals()
{
    AbstractRaiderEntity::registerGoals();

    // 优先级 0: 游泳
    goalSelector().addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 4: 近战攻击（使用 RavagerAttackGoal）
    goalSelector().addGoal(4, std::make_unique<entity::ai::goal::RavagerAttackGoal>(this));

    // 优先级 5: 避水随机行走
    goalSelector().addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 0.4));

    // 优先级 6: 看向玩家
    goalSelector().addGoal(6,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 6.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 10: 看向生物
    // 注: 原版看向 MobEntity.class，我们暂时看向所有 LivingEntity
    goalSelector().addGoal(10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 目标选择器
    // 优先级 2: 被攻击后反击，呼叫同伴
    // MC 原版: HurtByTargetGoal(this, Raider.class).setAlertOthers()
    // 劫掠兽不会反击其他灾厄村民
    targetSelector().addGoal(
        2, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 3: 攻击玩家
    targetSelector().addGoal(3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

    // 优先级 4: 攻击村民（排除幼年村民）
    // MC 原版: NearestAttackableTargetGoal<>(this, AbstractVillager.class, true, (p_199899_, p_376378_) ->
    // !p_199899_.isBaby())
    targetSelector().addGoal(4,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(
            this, true, 0, [](const LivingEntity* entity) -> bool {
                auto* villager = dynamic_cast<const entity::AbstractVillagerEntity*>(entity);
                return villager != nullptr && !villager->isChild();
            }));

    // 优先级 4: 攻击铁傀儡
    targetSelector().addGoal(
        4, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));
}

void RavagerEntity::registerAttributes()
{
    AbstractRaiderEntity::registerAttributes();

    // 注册 ATTACK_KNOCKBACK 属性（不在基类中注册）
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackKnockback());

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 100.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, 0.75);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ATTACK_DAMAGE);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_KNOCKBACK, 1.5);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 32.0);

    // 设置经验值
    setExperienceValue(20);
}

} // namespace mc
