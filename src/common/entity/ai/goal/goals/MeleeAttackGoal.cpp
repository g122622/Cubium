#include "MeleeAttackGoal.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../damage/DamageSource.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../controller/LookController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"

namespace mc::entity::ai::goal {

using namespace constants;

MeleeAttackGoal::MeleeAttackGoal(CreatureEntity* creature, f64 speed, bool useLongMemory)
    : m_creature(creature)
    , m_speed(speed)
    , m_useLongMemory(useLongMemory)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool MeleeAttackGoal::shouldExecute()
{
    if (!m_creature) return false;

    // MC 1.16.5: 游戏时间节流 - 每20 tick检查一次
    u32 ticksExisted = m_creature->ticksExisted();
    if (ticksExisted - m_lastCheckTime < constants::TARGET_CHECK_COOLDOWN) {
        return false;
    }
    m_lastCheckTime = ticksExisted;

    // 获取攻击目标
    LivingEntity* target = m_creature->attackTarget();
    if (!target || !target->isAlive()) {
        return false;
    }

    m_attackTarget = target;

    // MC 1.16.5: 尝试获取路径
    if (m_creature->navigator()) {
        if (m_creature->navigator()->moveTo(*target, 0)) {
            return true;
        }
    }

    // 如果路径失败，检查是否已经在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_creature->distanceSqTo(*target);
    return distSq <= attackReachSq;
}

bool MeleeAttackGoal::shouldContinueExecuting()
{
    if (!m_creature || !m_attackTarget) return false;

    // 检查目标是否存活
    if (!m_attackTarget->isAlive()) {
        return false;
    }

    // MC 1.16.5: 如果使用长期记忆，检查目标是否在家范围内
    // if (this.longMemory && !this.attacker.isWithinHomeDistanceFromPosition(livingentity.getPosition()))
    //     return false;
    if (m_useLongMemory) {
        // 只有 MobEntity 才有家范围概念
        MobEntity* mob = dynamic_cast<MobEntity*>(m_creature);
        if (mob && !mob->isWithinHomeDistanceFromPosition(BlockPos(m_attackTarget->position()))) {
            return false;
        }
    }

    // MC 1.16.5: 玩家模式检查
    // if (livingentity instanceof PlayerEntity) {
    //     PlayerEntity player = (PlayerEntity)livingentity;
    //     if (player.isSpectator() || player.isCreative()) {
    //         return false;
    //     }
    // }
    Player* player = dynamic_cast<Player*>(m_attackTarget);
    if (player && (player->isSpectator() || player->isCreative())) {
        return false;
    }

    // MC 1.16.5: 如果不使用长期记忆且没有路径，停止
    auto* nav = m_creature->navigator();
    if (!m_useLongMemory && nav && nav->noPath()) {
        return false;
    }

    // 检查距离
    f32 distSq = m_creature->distanceSqTo(*m_attackTarget);

    if (distSq > MELEE_ATTACK_STOP_DISTANCE_SQ) {
        return false; // 目标太远，停止追踪
    }

    // 如果使用长期记忆，继续追踪
    if (m_useLongMemory) {
        return true;
    }

    return shouldExecute();
}

void MeleeAttackGoal::startExecuting()
{
    m_attackCooldown = 0;
    m_pathRecalculateTimer = 0;
    m_failedPathFindingPenalty = 0;
    m_targetX = 0.0;
    m_targetY = 0.0;
    m_targetZ = 0.0;

    if (m_creature) {
        // MC 1.16.5: 设置激怒状态
        m_creature->setAggroed(true);

        if (auto* nav = m_creature->navigator()) {
            if (m_attackTarget) {
                static_cast<void>(nav->moveTo(*m_attackTarget, m_speed));
            }
        }
    }
}

void MeleeAttackGoal::resetTask()
{
    m_attackTarget = nullptr;
    m_failedPathFindingPenalty = 0;

    if (m_creature) {
        m_creature->clearNavigation();
    }
}

void MeleeAttackGoal::tick()
{
    if (!m_creature || !m_attackTarget) return;

    // MC 1.16.5: 使用 LookController 看向目标，参数为 (30.0F, 30.0F)
    if (auto* lookCtrl = m_creature->lookController()) {
        lookCtrl->setLookPositionWithEntity(*m_attackTarget, 30.0f, 30.0f);
    }

    f64 distSq = m_creature->distanceSqTo(*m_attackTarget);

    // MC 1.16.5: 路径重算逻辑
    m_pathRecalculateTimer = std::max(m_pathRecalculateTimer - 1, 0);

    bool shouldRecalcPath = false;

    // 检查是否需要重新计算路径
    if ((m_useLongMemory || m_creature->canSee(*m_attackTarget)) && m_pathRecalculateTimer <= 0 &&
        ((m_targetX == 0.0 && m_targetY == 0.0 && m_targetZ == 0.0) ||
            m_attackTarget->distanceSqTo(m_targetX, m_targetY, m_targetZ) >= 1.0 ||
            m_creature->getRandom().nextFloat() < 0.05f)) {

        shouldRecalcPath = true;
    }

    if (shouldRecalcPath) {
        // 更新目标位置
        m_targetX = m_attackTarget->x();
        m_targetY = m_attackTarget->y();
        m_targetZ = m_attackTarget->z();

        // MC 1.16.5: 随机重算间隔 (4-10)
        math::Random rng = m_creature->getRandom();
        m_pathRecalculateTimer = PATH_RECALCULATE_BASE + rng.nextInt(PATH_RECALCULATE_RANDOM);

        // MC 1.16.5: 添加路径失败惩罚
        if (m_canPenalize) {
            m_pathRecalculateTimer += m_failedPathFindingPenalty;
            m_failedPathFindingPenalty += PATH_FAILURE_PENALTY;
        }

        // MC 1.16.5: 根据距离调整重算间隔
        if (distSq > DISTANCE_FAR_THRESHOLD) { // > 32格距离
            m_pathRecalculateTimer += PATH_RECALC_FAR_BONUS;
        } else if (distSq > DISTANCE_MEDIUM_THRESHOLD) { // > 16格距离
            m_pathRecalculateTimer += PATH_RECALC_MEDIUM_BONUS;
        }

        // 移动到目标
        if (m_creature->navigator()) {
            if (!m_creature->navigator()->moveTo(*m_attackTarget, m_speed)) {
                m_pathRecalculateTimer += 15; // 路径失败惩罚
            }
        }
    }

    // 减少攻击冷却
    m_attackCooldown = std::max(m_attackCooldown - 1, 0);

    // 检查是否可以攻击
    checkAndPerformAttack(m_attackTarget, distSq);
}

bool MeleeAttackGoal::canAttack(LivingEntity* target) const
{
    if (!m_creature || !target) return false;

    // MC 1.16.5: 检查是否在攻击范围内
    f32 attackReachSq = getAttackReachSqr(target);
    f32 distSq = m_creature->distanceSqTo(*target);

    return distSq <= attackReachSq;
}

void MeleeAttackGoal::checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr)
{
    if (!m_creature || !target) return;

    f32 attackReachSq = getAttackReachSqr(target);

    if (distToEnemySqr <= static_cast<f64>(attackReachSq) && m_attackCooldown <= 0) {
        // 重置攻击冷却
        m_attackCooldown = ATTACK_COOLDOWN_TICKS;

        // MC 1.16.5: 执行攻击
        attackTarget(target);
    }
}

void MeleeAttackGoal::attackTarget(LivingEntity* target)
{
    if (!m_creature || !target) return;

    // MC 1.16.5: 挥动手臂动画
    m_creature->swingArm();

    // MC 1.16.5: 调用实体本身的攻击方法
    // 使用伤害属性
    using namespace mc::entity::attribute;
    f32 damage = static_cast<f32>(m_creature->getAttributeValue(Attributes::ATTACK_DAMAGE, 1.0));

    // 创建伤害来源
    EntityDamageSource damageSource(DamageType::MobAttack, m_creature);

    // 应用伤害
    target->hurt(damageSource, damage);

    // 触发攻击声音（由具体生物决定是否播放）
    m_creature->playAttackSound(*target);

    // 应用击退
    f32 knockbackStrength = static_cast<f32>(m_creature->getAttributeValue(Attributes::ATTACK_KNOCKBACK, 1.0));

    if (knockbackStrength > 0.0f) {
        // 计算击退方向
        f32 dx = static_cast<f32>(target->x() - m_creature->x());
        f32 dz = static_cast<f32>(target->z() - m_creature->z());
        f32 distSq = dx * dx + dz * dz;

        if (distSq > 0.000001f) {
            f32 invDist = mc::math::fastInverseSqrt(distSq);
            dx *= invDist;
            dz *= invDist;
        }

        // 应用击退速度
        f32 knockbackX = dx * knockbackStrength * 0.5f;
        f32 knockbackY = 0.1f * knockbackStrength;
        f32 knockbackZ = dz * knockbackStrength * 0.5f;

        target->addVelocity(knockbackX, knockbackY, knockbackZ);
    }
    // 注意：攻击冷却已在checkAndPerformAttack中设置，此处不再重复设置
}

f32 MeleeAttackGoal::getAttackReachSqr(LivingEntity* target) const
{
    // MC 1.16.5: (this.attacker.getWidth() * 2.0F) * (this.attacker.getWidth() * 2.0F) + target.getWidth()
    // 注意：原版公式是 (width * 2)^2 + targetWidth，而不是 width^2 + targetWidth
    f32 attackerWidth = m_creature->width();
    f32 targetWidth = target->width();
    f32 reachWidth = attackerWidth * 2.0f;
    return reachWidth * reachWidth + targetWidth;
}

} // namespace mc::entity::ai::goal
