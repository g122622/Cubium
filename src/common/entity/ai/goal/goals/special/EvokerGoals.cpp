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

#include "EvokerGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/illager/EvokerEntity.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <vector>

namespace mc {
namespace entity::ai::goal {

// ============================================================================
// EvokerSpellGoal - 施法目标基类
// ============================================================================
//
// flag 对齐原版 SpellcasterIllager.SpellcasterUseSpellGoal：原版施法 goal 不设置任何
// Goal.Flag（默认空 EnumSet）。施法 goal 必须无 flag，否则会与 EvokerCastingSpellGoal
// (优先级1, 占 Move+Look) 互斥抢占——施法 goal startExecuting 调 startCasting 设
// isSpellcasting()=true，CastingSpellGoal 随即 shouldExecute=true 启动，若两者共享 flag
// (如 Look)，CastingSpellGoal 启动会 reset 施法 goal（_startGoal 抢占共享 flag 的运行中 goal），
// 施法 goal 的 warmup 计数被 resetTask 清掉，castSpell 永不执行。
//   无 flag 后施法 goal 不被任何 goal 抢占，warmup 持续递减到 0 执行 castSpell 生成尖牙/召唤恼鬼。
// 施法期间的"停步 + 看向目标"由 EvokerCastingSpellGoal(优先级1, Move+Look) 接管：其优先级1
// 高于 AvoidEntityGoal(2)，AvoidEntity 无法抢占 MOVE，唤魔者施法时停步不被 flee 打断，对齐原版
// SpellcasterCastingSpellGoal(MOVE+LOOK) 语义。施法 warmup 期间看向目标由本 goal tick 主动调
// lookController()->setLookPositionWithEntity 实现，不依赖 Look flag。

EvokerSpellGoal::EvokerSpellGoal(EvokerEntity* evoker)
    : Goal()
    , m_evoker(evoker)
{
    MC_ASSERT(evoker != nullptr);
}

bool EvokerSpellGoal::shouldExecute()
{
    if (m_evoker == nullptr) {
        return false;
    }

    // 有攻击目标时才能施法
    LivingEntity* target = m_evoker->attackTarget();
    if (target == nullptr || !target->isAlive()) {
        return false;
    }

    // 冷却中不能施法
    if (m_spellCooldown > 0) {
        return false;
    }

    // 正在施法时不能开始新施法
    return !m_evoker->isSpellcasting();
}

bool EvokerSpellGoal::shouldContinueExecuting()
{
    if (m_evoker == nullptr) {
        return false;
    }

    // 施法准备阶段继续检查目标有效性
    if (m_spellWarmup > 0) {
        LivingEntity* target = m_evoker->attackTarget();
        return target != nullptr && target->isAlive();
    }

    // 施法中不能被打断
    return m_evoker->isSpellcasting();
}

void EvokerSpellGoal::startExecuting()
{
    m_spellWarmup = getCastWarmupTime();
    m_evoker->startCasting(getSpellTypeId());
}

void EvokerSpellGoal::resetTask()
{
    m_spellCooldown = getCastingInterval();
    m_evoker->clearSpellcasting();
}

void EvokerSpellGoal::tick()
{
    if (m_evoker == nullptr) {
        return;
    }

    LivingEntity* target = m_evoker->attackTarget();

    // 施法准备阶段：看向目标
    if (m_spellWarmup > 0) {
        m_spellWarmup--;
        if (target != nullptr) {
            // 看向目标
            m_evoker->lookController()->setLookPositionWithEntity(*target, 10.0f, 10.0f);
        }
        // 当 warmup 结束时，执行施法
        if (m_spellWarmup == 0) {
            castSpell();
        }
        return;
    }

    // 冷却递减
    if (m_spellCooldown > 0) {
        m_spellCooldown--;
    }
}

// ============================================================================
// EvokerAttackSpellGoal - 尖牙攻击目标
// ============================================================================

EvokerAttackSpellGoal::EvokerAttackSpellGoal(EvokerEntity* evoker)
    : EvokerSpellGoal(evoker)
{}

void EvokerAttackSpellGoal::castSpell()
{
    m_target = m_evoker->attackTarget();
    if (m_target != nullptr) {
        m_evoker->castFangsAttack();
    }
}

// ============================================================================
// EvokerSummonSpellGoal - 召唤恼鬼目标
// ============================================================================

EvokerSummonSpellGoal::EvokerSummonSpellGoal(EvokerEntity* evoker)
    : EvokerSpellGoal(evoker)
{}

bool EvokerSummonSpellGoal::shouldExecute()
{
    if (!EvokerSpellGoal::shouldExecute()) {
        return false;
    }

    // 检查周围恼鬼数量
    i32 vexCount = _countNearbyVexes();

    // 只有当周围恼鬼少于8个时才召唤
    if (m_evoker->world() != nullptr) {
        math::Random& rng = m_evoker->world()->getRandom();
        return rng.nextInt(8) + 1 > vexCount;
    }

    return vexCount < 8;
}

void EvokerSummonSpellGoal::castSpell()
{
    m_evoker->summonVex();
}

i32 EvokerSummonSpellGoal::_countNearbyVexes() const
{
    if (m_evoker == nullptr || m_evoker->world() == nullptr) {
        return 0;
    }

    IWorld* world = m_evoker->world();

    // 获取唤魔者的碰撞箱并向各方向扩展16格
    AxisAlignedBB searchBox = m_evoker->boundingBox().grow(16.0f);

    // 获取范围内的所有实体
    std::vector<Entity*> entities = world->getEntitiesInAABB(searchBox, m_evoker);

    // 统计恼鬼数量
    i32 vexCount = 0;
    for (Entity* entity : entities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        // 检查是否为恼鬼实体
        if (entity->entityType() == entity::VanillaEntityTypeKeys::VEX) {
            vexCount++;
        }
    }

    return vexCount;
}

// ============================================================================
// EvokerCastingSpellGoal - 施法时看向目标
//
// 施法期间让唤魔者看向目标，并占用 MOVE+LOOK flag 使唤魔者施法时停步。
// flag 对齐原版 SpellcasterIllager.SpellcasterCastingSpellGoal(MOVE+LOOK)：原版施法 goal
// (SpellcasterUseSpellGoal) 不占 flag，由本 goal 在 isCastingSpell() 期间接管 MOVE 停步。
// 优先级1 高于 AvoidEntityGoal(2)，施法期间 AvoidEntity 无法抢占 MOVE，唤魔者不被 flee 打断。
// ============================================================================

EvokerCastingSpellGoal::EvokerCastingSpellGoal(EvokerEntity* evoker)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_evoker(evoker)
{
    MC_ASSERT(evoker != nullptr);
}

bool EvokerCastingSpellGoal::shouldExecute()
{
    return m_evoker != nullptr && m_evoker->isSpellcasting();
}

bool EvokerCastingSpellGoal::shouldContinueExecuting()
{
    return shouldExecute();
}

void EvokerCastingSpellGoal::tick()
{
    if (m_evoker == nullptr) {
        return;
    }

    LivingEntity* target = m_evoker->attackTarget();
    if (target != nullptr) {
        m_evoker->lookController()->setLookPositionWithEntity(*target, 10.0f, 10.0f);
    }
}

// ============================================================================
// EvokerWololoSpellGoal - 唔噜噜法术（将蓝色羊变成红色羊）
//
// flag 对齐原版 SpellcasterIllager.SpellcasterUseSpellGoal：原版 Wololo 施法 goal 不占 flag
// （与 EvokerSpellGoal 同理，避免被 EvokerCastingSpellGoal 抢占 reset 致 warmup 丢失）。
// ============================================================================

EvokerWololoSpellGoal::EvokerWololoSpellGoal(EvokerEntity* evoker)
    : Goal()
    , m_evoker(evoker)
{
    MC_ASSERT(evoker != nullptr);
}

bool EvokerWololoSpellGoal::shouldExecute()
{
    if (m_evoker == nullptr) {
        return false;
    }

    // 有攻击目标时不执行 Wololo
    LivingEntity* attackTarget = m_evoker->attackTarget();
    if (attackTarget != nullptr && attackTarget->isAlive()) {
        return false;
    }

    // 正在施法时不执行
    if (m_evoker->isSpellcasting()) {
        return false;
    }

    // 冷却中不执行
    if (m_spellCooldown > 0) {
        return false;
    }

    // 寻找蓝色羊
    m_wololoTarget = _findBlueSheep();
    return m_wololoTarget != nullptr;
}

void EvokerWololoSpellGoal::startExecuting()
{
    m_spellWarmup = CAST_WARMUP_TIME;
    m_evoker->startCasting(static_cast<i32>(SpellcastingIllagerEntity::SpellType::Wololo));
}

void EvokerWololoSpellGoal::resetTask()
{
    m_spellCooldown = CASTING_INTERVAL;
    m_wololoTarget = nullptr;
    m_evoker->clearSpellcasting();
}

void EvokerWololoSpellGoal::tick()
{
    if (m_evoker == nullptr) {
        return;
    }

    // 准备阶段：看向目标羊
    if (m_spellWarmup > 0) {
        m_spellWarmup--;
        if (m_wololoTarget != nullptr && m_wololoTarget->isAlive()) {
            m_evoker->lookController()->setLookPositionWithEntity(*m_wololoTarget, 10.0f, 10.0f);
        }
        // 当 warmup 结束时，执行施法
        if (m_spellWarmup == 0) {
            // 将蓝色羊变成红色羊
            if (m_wololoTarget != nullptr && m_wololoTarget->isAlive()) {
                m_wololoTarget->setFleeceColor(DyeColor::Red);
            }
        }
        return;
    }

    // 冷却递减
    if (m_spellCooldown > 0) {
        m_spellCooldown--;
    }
}

SheepEntity* EvokerWololoSpellGoal::_findBlueSheep() const
{
    if (m_evoker == nullptr || m_evoker->world() == nullptr) {
        return nullptr;
    }

    IWorld* world = m_evoker->world();

    // 获取唤魔者的碰撞箱并向各方向扩展（X/Z 16格，Y 4格）
    AxisAlignedBB searchBox = m_evoker->boundingBox().expand(SEARCH_RANGE, 4.0f, SEARCH_RANGE);

    // 获取范围内的所有实体
    std::vector<Entity*> entities = world->getEntitiesInAABB(searchBox, m_evoker);

    // 收集所有蓝色羊
    std::vector<SheepEntity*> blueSheep;
    for (Entity* entity : entities) {
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }
        // 检查是否为羊实体
        if (entity->entityType() == entity::VanillaEntityTypeKeys::SHEEP) {
            SheepEntity* sheep = static_cast<SheepEntity*>(entity);
            // 检查羊毛颜色是否为蓝色
            if (sheep->getFleeceColor() == DyeColor::Blue) {
                blueSheep.push_back(sheep);
            }
        }
    }

    if (blueSheep.empty()) {
        return nullptr;
    }

    // 随机选择一只蓝色羊
    math::Random& rng = world->getRandom();
    return blueSheep[rng.nextInt(static_cast<i32>(blueSheep.size()))];
}

} // namespace entity::ai::goal
} // namespace mc
