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

#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../ai/goal/GoalFlag.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/special/EvokerGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../entities/villager/VillagerEntity.hpp"
#include "../../player/Player.hpp"
#include "../../projectile/OtherProjectiles.hpp"
#include <cmath>

namespace mc {

EvokerEntity::EvokerEntity(EntityId id)
    : SpellcastingIllagerEntity(id)
{
    registerAttributes();
}

std::unique_ptr<Entity> EvokerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<EvokerEntity>(EntityId(0));
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
    // MC 1.16.5 EvokerEntity.AttackSpellGoal.castSpell()
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
            spawnFangs(fangX, fangZ, minY, maxY, fangAngle, 0);
        }

        // 外圈：8个尖牙，半径2.5，延迟3
        for (i32 i = 0; i < 8; ++i) {
            f32 fangAngle = angle + static_cast<f32>(i) * math::PI * 2.0f / 8.0f + 1.2566371f;
            f32 fangX = x() + std::cos(fangAngle) * 2.5f;
            f32 fangZ = z() + std::sin(fangAngle) * 2.5f;
            spawnFangs(fangX, fangZ, minY, maxY, fangAngle, 3);
        }
    } else {
        // 远距离攻击：直线尖牙
        for (i32 i = 0; i < 16; ++i) {
            f32 distance = 1.25f * static_cast<f32>(i + 1);
            i32 delay = i; // 延迟递增
            f32 fangX = x() + std::cos(angle) * distance;
            f32 fangZ = z() + std::sin(angle) * distance;
            spawnFangs(fangX, fangZ, minY, maxY, angle, delay);
        }
    }
}

void EvokerEntity::spawnFangs(f32 posX, f32 posZ, f32 minY, f32 maxY, f32 angle, i32 warmupDelay)
{
    // MC 1.16.5 EvokerEntity.AttackSpellGoal.spawnFangs()
    if (m_world == nullptr) {
        return;
    }

    // 从上往下找到合适的生成位置
    BlockPos blockPos(static_cast<i32>(std::floor(posX)),
        static_cast<i32>(std::floor(maxY)),
        static_cast<i32>(std::floor(posZ)));

    bool foundSolidGround = false;
    f32 groundY = 0.0f;

    // 向下搜索直到找到固体方块或到达 minY
    while (blockPos.y >= static_cast<i32>(std::floor(minY)) - 1) {
        BlockPos belowPos(blockPos.x, blockPos.y - 1, blockPos.z);
        const BlockState* belowState = m_world->getBlockState(belowPos);

        if (belowState != nullptr && belowState->isSolid()) {
            // 找到固体地面
            foundSolidGround = true;

            // 检查当前位置是否有碰撞箱（如草、花等）
            const BlockState* currentState = m_world->getBlockState(blockPos);
            if (currentState != nullptr && !currentState->isAir()) {
                // 获取碰撞箱的上表面高度
                // 简化实现：假设完整方块的碰撞箱高度为1.0
                // 完整实现需要 VoxelShape
                groundY = static_cast<f32>(blockPos.y);
            } else {
                groundY = static_cast<f32>(blockPos.y);
            }
            break;
        }

        blockPos.y--;
    }

    if (foundSolidGround) {
        // 创建唤魔者尖牙实体
        auto fangs = std::make_unique<entity::EvokerFangsEntity>(EntityId(0));
        fangs->setPosition(posX, groundY, posZ);
        fangs->setRotation(angle * math::RAD_TO_DEG, 0.0f);
        fangs->setWarmupDelay(warmupDelay);
        fangs->setOwner(this);

        // 将实体添加到世界
        m_world->spawnEntity(std::move(fangs));
    }
}

void EvokerEntity::summonVex()
{
    // MC 1.16.5 EvokerEntity.SummonSpellGoal.castSpell()
    if (m_world == nullptr) {
        return;
    }

    // 召唤3个恼鬼
    for (i32 i = 0; i < 3; ++i) {
        // 在唤魔者周围随机位置生成
        math::Random& rng = m_world->getRandom();
        i32 offsetX = -2 + rng.nextInt(5);
        i32 offsetZ = -2 + rng.nextInt(5);
        BlockPos spawnPos(static_cast<i32>(x()) + offsetX,
            static_cast<i32>(y()) + 1,
            static_cast<i32>(z()) + offsetZ);

        // 创建恼鬼实体
        auto vex = std::make_unique<VexEntity>(EntityId(0));
        vex->setPosition(static_cast<f32>(spawnPos.x), static_cast<f32>(spawnPos.y), static_cast<f32>(spawnPos.z));
        vex->setRotation(0.0f, 0.0f);

        // 设置所有者
        vex->setOwner(this);

        // 设置有限生命（30-120秒）
        vex->setLimitedLife(true);
        vex->setLifeTime(20 * (30 + rng.nextInt(90)));

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
    // MC 1.16.5 EvokerEntity.registerGoals()
    // 优先级: 0 = 游泳, 1 = 施法时看向目标, 2 = 避开玩家, 4 = 召唤恼鬼, 5 = 尖牙攻击,
    //         6 = 唔噜噜法术（转换蓝色羊）, 8 = 随机漫步, 9 = 看向玩家, 10 = 看向生物

    goalSelector().addGoal(0, new entity::ai::goal::SwimGoal(this));
    goalSelector().addGoal(1, new entity::ai::goal::EvokerCastingSpellGoal(this));
    goalSelector().addGoal(2, new entity::ai::goal::AvoidEntityGoal(
                                 this, 8.0f, 0.6, 1.0,
                                 [](const LivingEntity* e) -> bool {
                                     return e != nullptr && e->typeId() == entity::EntityTypeIdNumber::PLAYER;
                                 }));
    goalSelector().addGoal(4, new entity::ai::goal::EvokerSummonSpellGoal(this));
    goalSelector().addGoal(5, new entity::ai::goal::EvokerAttackSpellGoal(this));
    goalSelector().addGoal(6, new entity::ai::goal::EvokerWololoSpellGoal(this));
    goalSelector().addGoal(8, new entity::ai::goal::RandomWalkingGoal(this, 0.6));
    goalSelector().addGoal(9, new entity::ai::goal::LookAtGoal(this, 3.0f, 1.0f,
                                 [](const LivingEntity* e) -> bool {
                                     return e != nullptr && e->typeId() == entity::EntityTypeIdNumber::PLAYER;
                                 }));
    goalSelector().addGoal(10, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f,
                                 [](const LivingEntity* e) -> bool {
                                     return e != nullptr && dynamic_cast<const MobEntity*>(e) != nullptr;
                                 }));

    // 目标选择器
    // MC 1.16.5: HurtByTargetGoal 会呼唤其他灾厄村民
    targetSelector().addGoal(1, new entity::ai::goal::HurtByTargetGoal(this));
    targetSelector().addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));
    targetSelector().addGoal(3, new entity::ai::goal::NearestAttackableTargetGoal<entity::VillagerEntity>(this, false));
    targetSelector().addGoal(3, new entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>(this, false));
}

void EvokerEntity::registerAttributes()
{
    SpellcastingIllagerEntity::registerAttributes();
    // MC 1.16.5 EvokerEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5f); // MC 1.16.5: 唤魔者移动速度
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0f);
}

} // namespace mc
