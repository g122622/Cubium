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

#include "EvokerEntity.hpp"
#include "VexEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/special/EvokerGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace mc {

// ============================================================================
// 继承链标识（parent = SpellcastingIllagerEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& EvokerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"EvokerEntity", &SpellcastingIllagerEntity::classInfo()};
    return s_classInfo;
}

EvokerEntity::EvokerEntity(EntityInstanceId id)
    : SpellcastingIllagerEntity(id)
{
    registerAttributes();
}

std::unique_ptr<Entity> EvokerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EvokerEntity>(EntityInstanceId(0));
}

void EvokerEntity::startCasting(i32 spellType)
{
    setSpellType(SpellcastingIllagerEntity::spellTypeFromId(spellType));
    setSpellTicks(CASTING_DURATION);
}

void EvokerEntity::finishCasting()
{
    switch (spellType()) {
        case SpellType::Fangs:
            castFangsAttack();
            m_fangsCooldown = FANGS_COOLDOWN;
            break;
        case SpellType::SummonVex:
            summonVex();
            m_summonCooldown = SUMMON_COOLDOWN;
            break;
        case SpellType::None:
        case SpellType::Wololo:
        case SpellType::Disappear:
        case SpellType::Blindness:
        default:
            break;
    }

    clearSpellcasting();
}

void EvokerEntity::castFangsAttack()
{
    LivingEntity* target = attackTarget();
    if (target == nullptr || m_world == nullptr) {
        return;
    }

    // 计算目标位置的范围
    f32 dx = target->x() - x();
    f32 dz = target->z() - z();
    f32 distSq = dx * dx + dz * dz;

    // 计算朝向目标的角度
    f32 angle = std::atan2(dz, dx);

    // 获取 Y 范围
    f32 minY = std::min(target->y(), y());
    f32 maxY = std::max(target->y(), y()) + 1.0f;

    if (distSq < 9.0f) {
        // 近距离攻击：两圈尖牙
        // 内圈：5个尖牙，半径1.5，延迟0
        for (i32 i = 0; i < 5; ++i) {
            f32 fangAngle = angle + static_cast<f32>(i) * math::PI * 0.4f;
            f32 fangX = x() + std::cos(fangAngle) * 1.5f;
            f32 fangZ = z() + std::sin(fangAngle) * 1.5f;
            _spawnFangs(fangX, fangZ, minY, maxY, fangAngle, 0);
        }

        // 外圈：8个尖牙，半径2.5，延迟3
        for (i32 i = 0; i < 8; ++i) {
            f32 fangAngle = angle + static_cast<f32>(i) * math::PI * 2.0f / 8.0f + 1.2566371f;
            f32 fangX = x() + std::cos(fangAngle) * 2.5f;
            f32 fangZ = z() + std::sin(fangAngle) * 2.5f;
            _spawnFangs(fangX, fangZ, minY, maxY, fangAngle, 3);
        }
    } else {
        // 远距离攻击：直线尖牙
        for (i32 i = 0; i < 16; ++i) {
            f32 distance = 1.25f * static_cast<f32>(i + 1);
            i32 delay = i; // 延迟递增
            f32 fangX = x() + std::cos(angle) * distance;
            f32 fangZ = z() + std::sin(angle) * distance;
            _spawnFangs(fangX, fangZ, minY, maxY, angle, delay);
        }
    }
}

void EvokerEntity::_spawnFangs(f32 posX, f32 posZ, f32 minY, f32 maxY, f32 angle, i32 warmupDelay)
{
    if (m_world == nullptr) {
        return;
    }

    // 从上往下搜索合适的生成位置
    BlockPos blockPos(
        static_cast<i32>(std::floor(posX)), static_cast<i32>(std::floor(maxY)), static_cast<i32>(std::floor(posZ)));

    bool foundSolidGround = false;
    f32 shapeMaxY = 0.0f; // 碰撞箱的上表面高度（方块局部坐标，0~1）

    while (blockPos.y >= static_cast<i32>(std::floor(minY)) - 1) {
        BlockPos belowPos(blockPos.x, blockPos.y - 1, blockPos.z);
        const BlockState* belowState = m_world->getBlockState(belowPos);

        // 检查下方方块是否有向上的实心面（等价于 MC 的 isFaceSturdy(level, pos, Direction.UP)）
        if (belowState != nullptr && belowState->isSolidSide(*m_world, belowPos, Direction::Up)) {
            foundSolidGround = true;

            // 检查当前位置是否有非空方块（如台阶、地毯等），获取其碰撞箱的最大Y值
            const BlockState* currentState = m_world->getBlockState(blockPos);
            if (currentState != nullptr && !currentState->isAir()) {
                const CollisionShape& collisionShape = currentState->getCollisionShape();
                if (!collisionShape.isEmpty()) {
                    // 遍历碰撞箱的所有AABB，取最大Y值
                    for (const auto& box : collisionShape.boxes()) {
                        shapeMaxY = std::max(shapeMaxY, box.maxY);
                    }
                }
            }

            break;
        }

        blockPos.y--;
    }

    if (foundSolidGround) {
        // 尖牙的Y坐标 = 方块Y坐标 + 碰撞箱上表面高度
        f32 groundY = static_cast<f32>(blockPos.y) + shapeMaxY;

        // 创建唤魔者尖牙实体
        auto fangs = std::make_unique<entity::EvokerFangsEntity>(EntityInstanceId(0));
        fangs->setPosition(posX, groundY, posZ);
        fangs->setRotation(angle * math::RAD_TO_DEG, 0.0f);
        fangs->setWarmupDelay(warmupDelay);
        fangs->setOwner(this);

        // 将实体添加到世界
        m_world->spawnEntity(std::move(fangs));

        // 触发实体放置游戏事件（通知幽匿感测体等振动监听器）
        BlockPos fangBlockPos(static_cast<i32>(std::floor(posX)),
            static_cast<i32>(std::floor(groundY)),
            static_cast<i32>(std::floor(posZ)));
        m_world->gameEvent(gameevent::GameEvents::ENTITY_PLACE, fangBlockPos, gameevent::GameEvent::Context::of(this));
    }
}

void EvokerEntity::summonVex()
{
    if (m_world == nullptr) {
        return;
    }

    // 召唤3个恼鬼
    for (i32 i = 0; i < 3; ++i) {
        // 在唤魔者周围随机位置生成
        math::Random& rng = m_world->getRandom();
        i32 offsetX = -2 + rng.nextInt(5);
        i32 offsetZ = -2 + rng.nextInt(5);
        BlockPos spawnPos(static_cast<i32>(x()) + offsetX, static_cast<i32>(y()) + 1, static_cast<i32>(z()) + offsetZ);

        // 创建恼鬼实体
        auto vex = std::make_unique<VexEntity>(EntityInstanceId(0));
        vex->setPosition(static_cast<f32>(spawnPos.x), static_cast<f32>(spawnPos.y), static_cast<f32>(spawnPos.z));
        vex->setRotation(0.0f, 0.0f);

        // 设置所有者
        vex->setOwner(this);

        // 设置有限生命（30-120秒）
        vex->setLimitedLife(true);
        vex->setLifeTime(20 * (30 + rng.nextInt(90)));

        // 对恼鬼调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
        entity::combat::DifficultyInstance difficultyInstance =
            entity::combat::DifficultyInstance::at(*m_world, spawnPos);
        vex->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::MobSummons);

        // 将实体添加到世界
        m_world->spawnEntity(std::move(vex));
    }
}

void EvokerEntity::tick()
{
    const bool wasSpellcasting = isSpellcasting();
    SpellcastingIllagerEntity::tick();

    if (wasSpellcasting && !isSpellcasting()) {
        finishCasting();
    }

    if (m_fangsCooldown > 0) {
        --m_fangsCooldown;
    }
    if (m_summonCooldown > 0) {
        --m_summonCooldown;
    }
}

void EvokerEntity::registerGoals()
{
    // 优先级: 0 = 游泳, 1 = 施法时看向目标, 2 = 避开玩家, 4 = 召唤恼鬼, 5 = 尖牙攻击,
    //         6 = 唔噜噜法术（转换蓝色羊）, 8 = 随机漫步, 9 = 看向玩家, 10 = 看向生物

    goalSelector().addGoal(0, new entity::ai::goal::SwimGoal(this));
    goalSelector().addGoal(1, new entity::ai::goal::EvokerCastingSpellGoal(this));
    goalSelector().addGoal(
        2, new entity::ai::goal::AvoidEntityGoal(this, 8.0f, 0.6, 1.0, [](const LivingEntity* e) -> bool {
            return e != nullptr && e->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));
    goalSelector().addGoal(4, new entity::ai::goal::EvokerSummonSpellGoal(this));
    goalSelector().addGoal(5, new entity::ai::goal::EvokerAttackSpellGoal(this));
    goalSelector().addGoal(6, new entity::ai::goal::EvokerWololoSpellGoal(this));
    goalSelector().addGoal(8, new entity::ai::goal::RandomWalkingGoal(this, 0.6));
    goalSelector().addGoal(9, new entity::ai::goal::LookAtGoal(this, 3.0f, 1.0f, [](const LivingEntity* e) -> bool {
        return e != nullptr && e->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
    }));
    goalSelector().addGoal(10, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* e) -> bool {
        return e != nullptr && dynamic_cast<const MobEntity*>(e) != nullptr;
    }));

    // 目标选择器：HurtByTargetGoal - 唤魔者不会反击其他灾厄村民
    // MC 原版: HurtByTargetGoal(this, Raider.class) — 不调用 setAlertOthers
    targetSelector().addGoal(
        1, new entity::ai::goal::HurtByTargetGoal(this, false, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));
    // 优先级 2: 攻击玩家（穿透墙壁追踪15秒）
    // MC 原版: NearestAttackableTargetGoal<>(this, Player.class, true).setUnseenMemoryTicks(300)
    {
        auto* playerTarget = new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true);
        playerTarget->setUnseenMemoryTicks(300);
        targetSelector().addGoal(2, playerTarget);
    }
    // 优先级 3: 攻击村民（穿透墙壁感知）
    // MC 原版: NearestAttackableTargetGoal<>(this, AbstractVillager.class, false).setUnseenMemoryTicks(300)
    // 注意：checkSight=false 时 unseenMemoryTicks 不生效，但与 MC 原版保持一致
    {
        auto* villagerTarget =
            new entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>(this, false);
        villagerTarget->setUnseenMemoryTicks(300);
        targetSelector().addGoal(3, villagerTarget);
    }
    // 优先级 3: 攻击铁傀儡（穿透墙壁感知）
    // MC 原版: NearestAttackableTargetGoal<>(this, IronGolem.class, false)
    targetSelector().addGoal(3, new entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>(this, false));
}

void EvokerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0f);
}

} // namespace mc
