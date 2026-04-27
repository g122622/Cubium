#include "LookAtGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../controller/LookController.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== LookAtGoal ====================

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance)
    : LookAtGoal(mob, maxDistance, DEFAULT_LOOK_CHANCE)
{
}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance)
    : m_mob(mob)
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

LookAtGoal::LookAtGoal(MobEntity* mob, f32 maxDistance, f32 chance, EntityFilter filter)
    : m_mob(mob)
    , m_filter(std::move(filter))
    , m_maxDistance(maxDistance)
    , m_chance(chance)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtGoal::shouldExecute() {
    if (!m_mob) return false;

    // MC 1.16.5: 检查概率
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() >= m_chance) {
        return false;
    }

    // MC 1.16.5: 首先检查攻击目标
    if (m_mob->attackTarget() != nullptr) {
        m_lookTarget = m_mob->attackTarget();
        return true;
    }

    // MC 1.16.5: 寻找最近的实体
    m_lookTarget = findTarget();
    return m_lookTarget != nullptr;
}

bool LookAtGoal::shouldContinueExecuting() {
    if (!m_lookTarget) return false;

    // MC 1.16.5: 检查目标是否存活
    if (!m_lookTarget->isAlive()) return false;

    // MC 1.16.5: 使用距离平方比较
    f64 distSq = m_mob->distanceSqTo(*m_lookTarget);
    f64 maxDistSq = static_cast<f64>(m_maxDistance) * static_cast<f64>(m_maxDistance);

    if (distSq > maxDistSq) {
        return false;
    }

    // MC 1.16.5: 检查剩余时间
    return m_lookTime > 0;
}

void LookAtGoal::startExecuting() {
    if (!m_mob) return;

    // MC 1.16.5: 设置看向时间 (40 + random.nextInt(40))
    math::Random rng = m_mob->getRandom();
    m_lookTime = LOOK_AT_MIN_TIME + rng.nextInt(LOOK_AT_MAX_TIME - LOOK_AT_MIN_TIME);
}

void LookAtGoal::resetTask() {
    m_lookTarget = nullptr;
}

void LookAtGoal::tick() {
    if (!m_mob || !m_lookTarget) return;

    // MC 1.16.5: 使用 LookController 看向目标眼睛位置
    // getLookController().setLookPosition(posX, getPosYEye(), posZ)
    if (auto* lookCtrl = m_mob->lookController()) {
        f64 eyeY = m_lookTarget->y() + m_lookTarget->eyeHeight();
        lookCtrl->setLookPosition(m_lookTarget->x(), eyeY, m_lookTarget->z());
    }

    --m_lookTime;
}

LivingEntity* LookAtGoal::findTarget() {
    if (!m_mob || !m_mob->world()) return nullptr;

    IWorld* world = m_mob->world();

    // MC 1.16.5: 查找最近的 LivingEntity
    // 使用 boundingBox.grow(maxDistance, 3.0D, maxDistance) 扩展范围
    auto entities = EntityUtils::findEntities<LivingEntity>(
        world,
        m_mob->position(),
        m_maxDistance + 3.0f,  // MC 1.16.5: boundingBox.grow(maxDistance, 3.0D, maxDistance)
        m_mob,
        [this](const LivingEntity* entity) {
            // 检查是否在最大距离内
            f64 distSq = m_mob->distanceSqTo(*entity);
            f64 maxDistSq = static_cast<f64>(m_maxDistance) * static_cast<f64>(m_maxDistance);
            if (distSq > maxDistSq) return false;

            // 执行自定义过滤条件
            if (m_filter && !m_filter(entity)) return false;

            return true;
        }
    );

    // 找最近的
    LivingEntity* closest = nullptr;
    f32 closestDistSq = m_maxDistance * m_maxDistance * 4.0f;  // 扩大搜索范围

    for (auto* entity : entities) {
        f32 distSq = static_cast<f32>(m_mob->distanceSqTo(*entity));
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closest = entity;
        }
    }

    return closest;
}

// ==================== LookRandomlyGoal ====================

LookRandomlyGoal::LookRandomlyGoal(MobEntity* mob)
    : m_mob(mob)
{
    // MC 1.16.5: EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK)
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool LookRandomlyGoal::shouldExecute() {
    if (!m_mob) return false;

    // MC 1.16.5: 默认概率执行 (0.02f)
    math::Random rng = m_mob->getRandom();
    return rng.nextFloat() < RANDOM_LOOK_CHANCE;
}

bool LookRandomlyGoal::shouldContinueExecuting() {
    // MC 1.16.5: return this.idleTime >= 0;
    return m_idleTime >= 0;
}

void LookRandomlyGoal::startExecuting() {
    if (!m_mob) return;

    // MC 1.16.5: 选择随机方向
    // double d0 = (Math.PI * 2D) * this.idleEntity.getRNG().nextDouble();
    // this.lookX = Math.cos(d0);
    // this.lookZ = Math.sin(d0);
    math::Random rng = m_mob->getRandom();
    f64 angle = math::TWO_PI * rng.nextDouble();

    m_lookX = std::cos(angle);
    m_lookZ = std::sin(angle);

    // MC 1.16.5: this.idleTime = 20 + this.idleEntity.getRNG().nextInt(20);
    m_idleTime = RANDOM_LOOK_MIN_TIME + rng.nextInt(RANDOM_LOOK_MAX_TIME - RANDOM_LOOK_MIN_TIME);
}

void LookRandomlyGoal::resetTask() {
    m_idleTime = 0;
}

void LookRandomlyGoal::tick() {
    if (!m_mob) return;

    // MC 1.16.5: --this.idleTime;
    --m_idleTime;

    // MC 1.16.5: getLookController().setLookPosition(
    //     getPosX() + lookX, getPosYEye(), getPosZ() + lookZ
    // );
    if (auto* lookCtrl = m_mob->lookController()) {
        f64 eyeY = m_mob->y() + m_mob->eyeHeight();
        lookCtrl->setLookPosition(
            m_mob->x() + m_lookX,
            eyeY,
            m_mob->z() + m_lookZ
        );
    }
}

} // namespace mc::entity::ai::goal
