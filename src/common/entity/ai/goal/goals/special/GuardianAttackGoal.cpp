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

#include "GuardianAttackGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"

namespace mc::entity::ai::goal {

GuardianAttackGoal::GuardianAttackGoal(GuardianEntity* guardian)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    , m_guardian(guardian)
    , m_isElder(false) // 将在 startExecuting 中检测
{
    MC_ASSERT_RELEASE(guardian != nullptr);
}

bool GuardianAttackGoal::shouldExecute()
{
    // 优先使用 targetSelector 设置的攻击目标
    LivingEntity* target = m_guardian->attackTarget();

    // 如果没有攻击目标，尝试自己搜索
    if (target == nullptr) {
        target = _selectTarget();
    }

    if (target == nullptr) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
        return false;
    }

    m_target = target;
    return true;
}

bool GuardianAttackGoal::shouldContinueExecuting()
{
    if (m_target == nullptr) {
        return false;
    }

    // 检查目标是否仍然有效
    if (!_isTargetValid(m_target)) {
        return false;
    }

    // 检查目标是否在范围内
    // 远古守卫者没有距离限制
    f64 distSq = m_guardian->distanceSqTo(*m_target);
    if (!m_isElder && distSq <= 9.0) { // 3.0 * 3.0 = 9.0
        // 目标太近，停止攻击
        return false;
    }

    // 检查是否能看到目标
    return m_guardian->canSee(*m_target);
}

void GuardianAttackGoal::startExecuting()
{
    // tickCounter = -10，准备阶段
    m_tickCounter = -10;

    // 检测是否为远古守卫者
    m_isElder = (m_guardian->entityType() == entity::VanillaEntityTypeKeys::ELDER_GUARDIAN);

    // 清除路径
    m_guardian->navigator()->clearPath();

    // 看向目标
    if (m_target != nullptr) {
        m_guardian->lookController()->setLookPosition(m_target->x(),
            m_target->y() + m_target->eyeHeight(),
            m_target->z(),
            90.0f, // 头部最大转动角度
            90.0f  // 身体最大转动角度
        );
    }
}

void GuardianAttackGoal::resetTask()
{
    m_target = nullptr;
    m_tickCounter = 0;

    // 清除目标实体ID
    m_guardian->setTargetEntity(0);
    // 清除攻击目标
    m_guardian->setAttackTarget(nullptr);
}

void GuardianAttackGoal::tick()
{
    if (m_target == nullptr) {
        return;
    }

    // 清除路径
    m_guardian->navigator()->clearPath();

    // 看向目标
    m_guardian->lookController()->setLookPosition(m_target->x(),
        m_target->y() + m_target->eyeHeight(),
        m_target->z(),
        90.0f, // 头部最大转动角度
        90.0f  // 身体最大转动角度
    );

    // 检查是否能看到目标
    if (!m_guardian->canSee(*m_target)) {
        // 失去目标，停止攻击
        m_guardian->setAttackTarget(nullptr);
        return;
    }

    // 递增计数器
    ++m_tickCounter;

    // 当 tickCounter == 0 时，设置目标实体ID并发送状态21
    if (m_tickCounter == 0) {
        // 设置目标实体ID
        m_guardian->setTargetEntity(m_target->id());

        // 广播实体状态事件（状态21 = GuardianAttack）
        // 触发客户端播放守卫者攻击音效
        if (!m_guardian->isSilent() && m_guardian->world() != nullptr) {
            m_guardian->world()->broadcastEntityStatus(
                m_guardian->id(), static_cast<u8>(network::EntityStatus::GuardianAttack));
        }
    } else if (m_tickCounter >= ATTACK_DURATION) {
        // 攻击完成，造成伤害。对齐 vanilla Guardian.GuardianAttackGoal.tick（Guardian.java:406-418）：
        //   先 indirectMagic 魔法伤害 f（不读 ATTACK_DAMAGE，绕过护甲），再 doHurtTarget 读
        //   ATTACK_DAMAGE 属性的近战伤害（走护甲/附魔管线）。
        f32 damage = LASER_DAMAGE;

        // 困难模式额外伤害（对齐 vanilla Guardian.java:408-410 HARD +2.0）
        if (m_guardian->world() != nullptr && m_guardian->world()->difficulty() == Difficulty::Hard) {
            damage += 2.0f;
        }

        // 远古守卫者额外伤害（对齐 vanilla Guardian.java:412-414 elder +2.0）
        if (m_isElder) {
            damage += ELDER_BONUS_DAMAGE;
        }

        // 魔法伤害：对齐 vanilla damageSources().indirectMagic(this.guardian, this.guardian)
        //   （Guardian.java:417）。带守卫者作为 causingEntity/directEntity，使伤害归属守卫者
        //   （影响死亡信息、反击 targetSelector 等）；indirectMagic 绕过护甲（setBypassesArmor）。
        //   此前用 magic()（无来源实体），伤害无归属，与 vanilla 偏差。
        auto magicDamage = DamageSources::indirectMagic(m_guardian, m_guardian);
        m_target->hurt(magicDamage, damage);

        // 近战伤害：对齐 vanilla doHurtTarget（Guardian.java:418），读 ATTACK_DAMAGE 属性。
        //   Guardian 6.0 / ElderGuardian 8.0（对齐 vanilla createAttributes）。走 mobAttack 完整
        //   近战管线（护甲/附魔/击退）。
        f32 attackDamage =
            static_cast<f32>(m_guardian->getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 0.0));
        if (attackDamage > 0.0f) {
            auto physicalDamage = DamageSources::mobAttack(m_guardian);
            m_target->hurt(physicalDamage, attackDamage);
        }

        // 清除攻击目标
        m_guardian->setAttackTarget(nullptr);
    }
}

LivingEntity* GuardianAttackGoal::_selectTarget() const
{
    if (m_guardian->world() == nullptr) {
        return nullptr;
    }

    // 守卫者攻击玩家和鱿鱼
    // 目标类型筛选: 只攻击 Player 或 Squid
    // 距离筛选: 必须距离大于 3 格（距离平方 > 9.0）
    // 注意：主要的目标选择逻辑应该由 targetSelector 中的 NearestAttackableTargetGoal 提供
    // 此方法作为备用逻辑，直接搜索最近的有效目标

    IWorld* world = m_guardian->world();

    // 获取跟随范围（搜索范围）
    f64 followRange = m_guardian->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
    f32 searchRange = static_cast<f32>(followRange);

    // 使用 EntityUtils 搜索最近的 LivingEntity
    LivingEntity* nearestTarget = EntityUtils::findClosestEntity<LivingEntity>(world,
        m_guardian->position(),
        searchRange,
        m_guardian, // 排除自己
        [this](LivingEntity* candidate) {
            // 1. 检查目标是否存活
            if (!candidate || !candidate->isAlive()) {
                return false;
            }

            // 2. 类型筛选: 对齐 Java 1.21.11 Guardian.GuardianAttackSelector（Guardian.java:436-438）
            //    攻击玩家、鱿鱼、发光鱿鱼、美西螈（vanilla instanceof Squid 涵盖 GlowSquid，
            //    Cubium 扁平枚举须显式列举 GLOW_SQUID + AXOLOTL）。与 GuardianEntity 主谓词保持一致。
            const entity::EntityType* type = candidate->entityType();
            bool isPlayer = (type == entity::VanillaEntityTypeKeys::PLAYER);
            bool isSquid =
                (type == entity::VanillaEntityTypeKeys::SQUID || type == entity::VanillaEntityTypeKeys::GLOW_SQUID);
            bool isAxolotl = (type == entity::VanillaEntityTypeKeys::AXOLOTL);
            if (!isPlayer && !isSquid && !isAxolotl) {
                return false;
            }

            // 3. 玩家特殊检查: 创造模式和观察者模式不能被攻击
            if (isPlayer) {
                Player* player = dynamic_cast<Player*>(candidate);
                if (player != nullptr && (player->isCreative() || player->isSpectator())) {
                    return false;
                }
            }

            // 4. 距离筛选: 必须距离 > 3 格
            f64 distSq = m_guardian->distanceSqTo(*candidate);
            if (distSq <= 9.0) { // 3.0 * 3.0 = 9.0
                return false;
            }

            // 5. 视线检查
            if (!m_guardian->canSee(*candidate)) {
                return false;
            }

            return true;
        });

    return nearestTarget;
}

bool GuardianAttackGoal::_isTargetValid(LivingEntity* target) const
{
    if (target == nullptr) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
        return false;
    }

    // 检查目标是否在同一世界
    if (target->world() != m_guardian->world()) {
        return false;
    }

    return true;
}

} // namespace mc::entity::ai::goal
