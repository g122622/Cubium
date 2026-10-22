/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do, subject to the following conditions:
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

#include "PandaGoals.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/passive/special/PandaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"

#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// PandaRollGoal
// ============================================================================

PandaRollGoal::PandaRollGoal(PandaEntity* panda)
    : m_panda(panda)
{
    // 互斥标志: MOVE, LOOK, JUMP
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump});
}

bool PandaRollGoal::shouldExecute()
{
    if (m_panda == nullptr) {
        return false;
    }

    // 条件1：幼年或顽皮性格
    if (!m_panda->isChild() && !m_panda->isPlayful()) {
        return false;
    }

    // 必须在地面
    if (!m_panda->onGround()) {
        return false;
    }

    // 条件2：检查熊猫是否可以执行动作（不在打喷嚏、吃东西、躺着、打滚等状态）
    if (!m_panda->canPerformAction()) {
        return false;
    }

    // 条件3：检查前方是否有悬崖或概率触发
    math::IRandom& rng = m_panda->getRandom();
    if (_isCliffInFront()) {
        // 如果前方是悬崖，100% 触发
        return true;
    } else if (m_panda->isPlayful()) {
        // 顽皮性格：1/60 概率
        return rng.nextInt(PLAYFUL_ROLL_CHANCE) == 0;
    } else {
        // 幼年熊猫（非顽皮）：1/500 概率
        return rng.nextInt(NORMAL_ROLL_CHANCE) == 0;
    }
}

bool PandaRollGoal::shouldContinueExecuting()
{
    // 打滚是一次性动作，由 rollCounter 控制持续时间
    // Goal 只负责触发，PandaEntity::updateRoll() 负责物理更新
    return false;
}

void PandaRollGoal::startExecuting()
{
    // 设置打滚状态，由 tick() 中的 updateRoll() 处理物理
    m_panda->setRolling(true);
    // 初始设置为 0，由 updateRoll() 递增
    m_panda->setRollTimer(0);
}

bool PandaRollGoal::_isCliffInFront() const
{
    if (m_panda == nullptr || m_panda->world() == nullptr) {
        return false;
    }

    IWorld* world = m_panda->world();

    // 计算熊猫朝向方向的前方位置
    const f32 yaw = m_panda->yaw();
    const f32 yawRad = math::toRadians(yaw);
    const f32 sinYaw = std::sin(yawRad);
    const f32 cosYaw = std::cos(yawRad);

    // 计算前方一格的偏移（当偏移超过0.5时才移动）
    i32 offsetX = 0;
    i32 offsetZ = 0;
    if (std::abs(sinYaw) > 0.5) {
        offsetX = static_cast<i32>(sinYaw / std::abs(sinYaw));
    }
    if (std::abs(cosYaw) > 0.5) {
        offsetZ = static_cast<i32>(cosYaw / std::abs(cosYaw));
    }

    // 检查前方一格下方是否是空气
    const BlockPos pandaPos(m_panda->position());
    const BlockPos checkPos(pandaPos.x + offsetX, pandaPos.y - 1, pandaPos.z + offsetZ);

    const BlockState* state = world->getBlockState(checkPos);
    return state != nullptr && state->isAir();
}

// ============================================================================
// PandaSneezeGoal
// ============================================================================

PandaSneezeGoal::PandaSneezeGoal(PandaEntity* panda)
    : m_panda(panda)
{
    // vanilla PandaSneezeGoal 未设 mutexFlags（canContinueToUse=false 一次性触发，由 tick 计时驱动）。
}

bool PandaSneezeGoal::shouldExecute()
{
    // 对齐 vanilla 1.21.11 Panda.PandaSneezeGoal.canUse（Panda.java:1111-1118）：
    //   if (isBaby() && canPerformAction()) {
    //       return isWeak() && random.nextInt(reducedTickDelay(500)) == 1
    //            ? true
    //            : random.nextInt(reducedTickDelay(6000)) == 1;
    //   }
    //   return false;
    if (m_panda == nullptr) {
        return false;
    }

    // 仅幼年熊猫且无其他动作时才考虑打喷嚏
    if (!m_panda->isChild() || !m_panda->canPerformAction()) {
        return false;
    }

    math::IRandom& rng = m_panda->getRandom();
    // 虚弱性格：1/500 概率（nextInt(reducedTickDelay(500))==1）；否则 1/6000 概率。
    // reducedTickDelay 减半补偿 GoalSelector 每 2 tick 评估一次（对齐 vanilla）。
    // 注：vanilla 用 ==1（非 ==0），原样对齐——仅随机数恰为 1 时触发。
    if (m_panda->isWeak()) {
        return rng.nextInt(reducedTickDelay(WEAK_SNEEZE_CHANCE)) == 1;
    }
    return rng.nextInt(reducedTickDelay(NORMAL_SNEEZE_CHANCE)) == 1;
}

bool PandaSneezeGoal::shouldContinueExecuting()
{
    // 一次性触发：goal 只负责启动 sneeze，由 PandaEntity::tick 的 m_sneezeTimer 递减驱动后续。
    return false;
}

void PandaSneezeGoal::startExecuting()
{
    // 对齐 vanilla PandaSneezeGoal.start：panda.sneeze(true)。
    // sneeze(true) 设 m_sneezing=true + m_sneezeTimer=SNEEZE_DURATION，tick 递减驱动预喷嚏音效
    // （timer==19，对齐 vanilla sneezeCounter==1）与 _onSneezeComplete（timer 到 0，对齐 afterSneeze）。
    m_panda->sneeze(true);
}

// ============================================================================
// PandaAttackGoal
// ============================================================================

PandaAttackGoal::PandaAttackGoal(PandaEntity* panda, f64 speed, bool useLongMemory)
    : MeleeAttackGoal(panda, speed, useLongMemory)
    , m_panda(panda)
{
    // 互斥标志继承自 MeleeAttackGoal（Move, Look）。
}

bool PandaAttackGoal::shouldExecute()
{
    // 对齐 vanilla 1.21.11 Panda.PandaAttackGoal.canUse（Panda.java:768-770）：
    //   return this.panda.canPerformAction() && super.canUse();
    // canPerformAction() 门控：打喷嚏/吃东西/躺/打滚时不发起攻击。
    // super.canUse() 由 MeleeAttackGoal::shouldExecute 完成（取 attackTarget、节流、寻路）。
    if (m_panda == nullptr) {
        return false;
    }
    if (!m_panda->canPerformAction()) {
        return false;
    }
    return MeleeAttackGoal::shouldExecute();
}

// ============================================================================
// PandaLieOnBackGoal
// ============================================================================

PandaLieOnBackGoal::PandaLieOnBackGoal(PandaEntity* panda)
    : m_panda(panda)
{}

bool PandaLieOnBackGoal::shouldExecute()
{
    // 对齐 vanilla PandaLieOnBackGoal.canUse：
    //   cooldown < tickCount && isLazy() && canPerformAction() && 1/400
    if (m_panda == nullptr || m_cooldown >= static_cast<i32>(m_panda->ticksExisted())) {
        return false;
    }
    return m_panda->isLazy() && m_panda->canPerformAction() && m_panda->getRandom().nextInt(400) == 1;
}

bool PandaLieOnBackGoal::shouldContinueExecuting()
{
    // 对齐 vanilla canContinueToUse：不在水中 && (懒惰 || 1/600 != 1) ? 1/2000 != 1 : false
    if (m_panda == nullptr || m_panda->isInWater()) {
        return false;
    }
    const bool stayLazy = m_panda->isLazy() || m_panda->getRandom().nextInt(600) != 1;
    return stayLazy && m_panda->getRandom().nextInt(2000) != 1;
}

void PandaLieOnBackGoal::startExecuting()
{
    m_panda->setOnBack(true);
}

void PandaLieOnBackGoal::resetTask()
{
    m_panda->setOnBack(false);
    m_cooldown = static_cast<i32>(m_panda->ticksExisted()) + 200;
}

// ============================================================================
// PandaLookAtPlayerGoal
// ============================================================================

PandaLookAtPlayerGoal::PandaLookAtPlayerGoal(PandaEntity* panda, f32 lookDistance)
    : LookAtGoal(panda, lookDistance, DEFAULT_LOOK_CHANCE)
    , m_panda(panda)
{
    // vanilla PandaLookAtPlayerGoal 只看向玩家（lookAtType = Player.class）。
    m_filter = [](const LivingEntity* entity) -> bool { return dynamic_cast<const Player*>(entity) != nullptr; };
}

void PandaLookAtPlayerGoal::setLookTarget(LivingEntity* target)
{
    m_lookTarget = target;
}

bool PandaLookAtPlayerGoal::shouldExecute()
{
    // 对齐 vanilla PandaLookAtPlayerGoal.canUse:918-946：已锁定目标时跳过搜索，
    // 但仍需要满足 canPerformAction 与目标有效。
    if (m_panda == nullptr) {
        return false;
    }
    if (m_panda->getRandom().nextFloat() >= m_chance) {
        return false;
    }

    LivingEntity* locked = m_lookTarget.get();
    if (locked == nullptr) {
        // 只搜索玩家（对齐 serverlevel.getNearestPlayer），而非任意 LivingEntity。
        locked = EntityUtils::findClosestEntity<Player>(
            m_panda->world(), m_panda->position(), m_maxDistance, m_panda, [](Player* player) -> bool {
                return player != nullptr && player->isAlive() && !player->isSpectator();
            });
        if (locked == nullptr) {
            return false;
        }
        m_lookTarget = locked;
    }

    return m_panda->canPerformAction();
}

bool PandaLookAtPlayerGoal::shouldContinueExecuting()
{
    if (!m_lookTarget) {
        return false;
    }
    return LookAtGoal::shouldContinueExecuting();
}

void PandaLookAtPlayerGoal::tick()
{
    // 对齐 vanilla：引用失效（目标被移除）时不 tick，避免解引用悬垂指针。
    if (!m_lookTarget) {
        return;
    }
    LookAtGoal::tick();
}

// ============================================================================
// PandaSitGoal
// ============================================================================

PandaSitGoal::PandaSitGoal(PandaEntity* panda)
    : m_panda(panda)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool PandaSitGoal::_hasNearbyEdibleItem(f32 inflate) const
{
    if (m_panda == nullptr || m_panda->world() == nullptr) {
        return false;
    }
    const AxisAlignedBB searchBox = m_panda->boundingBox().expand(inflate, inflate, inflate);
    for (const Entity* entity : m_panda->world()->getEntitiesInAABB(searchBox, m_panda)) {
        if (entity == nullptr) {
            continue;
        }
        const auto* itemEntity = dynamic_cast<const ItemEntity*>(entity);
        if (itemEntity == nullptr || !itemEntity->isAlive()) {
            continue;
        }
        // 对齐 vanilla Panda.canPickUpAndEat：只认可繁殖/可吃物品（竹子）。
        if (m_panda->isBreedingItem(itemEntity->getItemStack())) {
            return true;
        }
    }
    return false;
}

bool PandaSitGoal::shouldExecute()
{
    // 对齐 vanilla PandaSitGoal.canUse（Panda.java:1043-1060）。
    if (m_panda == nullptr || m_cooldown > static_cast<i32>(m_panda->ticksExisted())) {
        return false;
    }
    // 不满状态（繁殖前找不到竹子）期间不坐下，对齐 vanilla PandaSitGoal.canUse 的
    // getUnhappyCounter() > 0 门控。
    if (m_panda->isChild() || m_panda->isInWater() || !m_panda->canPerformAction() || m_panda->unhappyCounter() > 0) {
        return false;
    }
    if (!m_panda->getMainHandItem().isEmpty()) {
        return true;
    }
    return _hasNearbyEdibleItem(6.0f);
}

bool PandaSitGoal::shouldContinueExecuting()
{
    // 对齐 vanilla canContinueToUse：不在水中 && (懒惰 || 1/600 != 1) ? 1/2000 != 1 : false
    if (m_panda == nullptr || m_panda->isInWater()) {
        return false;
    }
    const bool staySitting = m_panda->isLazy() || m_panda->getRandom().nextInt(600) != 1;
    return staySitting && m_panda->getRandom().nextInt(2000) != 1;
}

void PandaSitGoal::startExecuting()
{
    if (m_panda->getMainHandItem().isEmpty()) {
        // 主手为空：走向最近的可吃物品（vanilla 取 inflate(8) 列表的首个元素）。
        if (m_panda->world() != nullptr) {
            const AxisAlignedBB searchBox = m_panda->boundingBox().expand(8.0f, 8.0f, 8.0f);
            for (Entity* entity : m_panda->world()->getEntitiesInAABB(searchBox, m_panda)) {
                const auto* itemEntity = dynamic_cast<const ItemEntity*>(entity);
                if (itemEntity != nullptr && itemEntity->isAlive() &&
                    m_panda->isBreedingItem(itemEntity->getItemStack())) {
                    (void)m_panda->navigator()->moveTo(*itemEntity, 1.2);
                    break;
                }
            }
        }
    } else {
        m_panda->tryToSit();
    }
    m_cooldown = 0;
}

void PandaSitGoal::resetTask()
{
    // 对齐 vanilla PandaSitGoal.stop（Panda.java:1088-1099）：把手里的食物丢回世界、
    // 清空主手，并按性格设置 10~60 / 10~160 秒冷却，最后站起。
    if (m_panda == nullptr) {
        return;
    }
    const ItemStack& held = m_panda->getMainHandItem();
    if (!held.isEmpty()) {
        const ItemStack dropped = held;
        if (m_panda->world() != nullptr) {
            ItemDropHelper::spawnItemAtEntity(m_panda, dropped, 0.5f, m_panda->getRandom());
        }
        m_panda->setMainHandItem(ItemStack{});
        const i32 seconds =
            m_panda->isLazy() ? m_panda->getRandom().nextInt(50) + 10 : m_panda->getRandom().nextInt(150) + 10;
        m_cooldown = static_cast<i32>(m_panda->ticksExisted()) + seconds * 20;
    }
    m_panda->sit(false);
}

void PandaSitGoal::tick()
{
    // 对齐 vanilla PandaSitGoal.tick:1071-1075。
    if (m_panda != nullptr && !m_panda->isSitting() && !m_panda->getMainHandItem().isEmpty()) {
        m_panda->tryToSit();
    }
}

// ============================================================================
// PandaAvoidGoal
// ============================================================================

PandaAvoidGoal::PandaAvoidGoal(
    PandaEntity* panda, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, AvoidEntityGoal::EntityPredicate predicate)
    : AvoidEntityGoal(panda, avoidDistance, farSpeed, nearSpeed, std::move(predicate))
    , m_panda(panda)
{}

bool PandaAvoidGoal::shouldExecute()
{
    // 对齐 vanilla PandaAvoidGoal.canUse：isWorried() && canPerformAction() && super.canUse()
    if (m_panda == nullptr || !m_panda->isWorried() || !m_panda->canPerformAction()) {
        return false;
    }
    return AvoidEntityGoal::shouldExecute();
}

} // namespace mc::entity::ai::goal
