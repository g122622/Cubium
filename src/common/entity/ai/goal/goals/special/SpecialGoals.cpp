#include "SpecialGoals.hpp"
#include "../../../../entities/passive/horse/AbstractHorseEntity.hpp"
#include "../../../../entities/passive/horse/LlamaEntity.hpp"
#include "../../../../entities/passive/water/DolphinEntity.hpp"
#include "../../../../entities/monster/basic/CreeperEntity.hpp"
#include "../../../../entities/monster/end/EndermanEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <cmath>

namespace mc {

// ==================== CreeperSwellGoal ====================

CreeperSwellGoal::CreeperSwellGoal(CreeperEntity* creeper)
    : Goal(GoalFlag::MOVE | GoalFlag::LOOK)
    , m_creeper(creeper)
{
    MC_ASSERT_NOT_NULL(creeper);
}

bool CreeperSwellGoal::shouldExecute() {
    if (!m_creeper) return false;

    // TODO: 获取攻击目标
    // m_attackTarget = m_creeper->getAttackTarget();
    // if (!m_attackTarget) return false;
    //
    // f32 distance = m_creeper->distanceTo(m_attackTarget);
    // return distance <= EXPLODE_DISTANCE;

    return false;
}

bool CreeperSwellGoal::shouldContinueExecuting() {
    if (!m_creeper || !m_attackTarget) return false;

    // TODO: 检查距离和是否存活
    // if (!m_attackTarget->isAlive()) return false;
    // f32 distance = m_creeper->distanceTo(m_attackTarget);
    // return distance <= EXPLODE_DISTANCE * 2.0f;

    return false;
}

void CreeperSwellGoal::startExecuting() {
    // TODO: 开始膨胀
    // m_creeper->setSwellState(1);
}

void CreeperSwellGoal::resetTask() {
    m_attackTarget = nullptr;
    // TODO: 停止膨胀
    // m_creeper->setSwellState(-1);
}

void CreeperSwellGoal::tick() {
    if (m_attackTarget) {
        // 看向目标
        // m_creeper->getLookController()->setLookAt(m_attackTarget);
    }
}

// ==================== EndermanTeleportGoal ====================

EndermanTeleportGoal::EndermanTeleportGoal(EndermanEntity* enderman)
    : Goal(GoalFlag::MOVE)
    , m_enderman(enderman)
{
    MC_ASSERT_NOT_NULL(enderman);
}

bool EndermanTeleportGoal::shouldExecute() {
    if (!m_enderman) return false;

    // TODO: 检查是否需要瞬移
    // - 受伤
    // - 在水中
    // - 在阳光下
    // - 有攻击者

    return false;
}

bool EndermanTeleportGoal::shouldContinueExecuting() {
    return false;
}

void EndermanTeleportGoal::startExecuting() {
    if (m_enderman) {
        m_enderman->teleport();
        m_teleportCooldown = TELEPORT_COOLDOWN;
    }
}

void EndermanTeleportGoal::tick() {
    if (m_teleportCooldown > 0) {
        m_teleportCooldown--;
    }
}

// ==================== LlamaFollowCaravanGoal ====================

LlamaFollowCaravanGoal::LlamaFollowCaravanGoal(LlamaEntity* llama, f32 speed)
    : Goal(GoalFlag::MOVE)
    , m_llama(llama)
    , m_speed(speed)
{
    MC_ASSERT_NOT_NULL(llama);
}

bool LlamaFollowCaravanGoal::shouldExecute() {
    if (!m_llama || m_llama->isInCaravan()) return false;

    // TODO: 寻找商队领袖
    // auto nearby = findNearbyLlamas();
    // for (auto llama : nearby) {
    //     if (llama->isInCaravan() || llama->getStrength() > m_llama->getStrength()) {
    //         m_caravanLeader = llama;
    //         return true;
    //     }
    // }

    return false;
}

bool LlamaFollowCaravanGoal::shouldContinueExecuting() {
    if (!m_llama || !m_caravanLeader) return false;
    // return m_caravanLeader->isAlive();
    return false;
}

void LlamaFollowCaravanGoal::startExecuting() {
    if (m_llama && m_caravanLeader) {
        m_llama->setInCaravan(true);
        m_llama->setCaravanLeader(m_caravanLeader);
    }
}

void LlamaFollowCaravanGoal::resetTask() {
    if (m_llama) {
        m_llama->setInCaravan(false);
        m_llama->setCaravanLeader(nullptr);
    }
    m_caravanLeader = nullptr;
}

void LlamaFollowCaravanGoal::tick() {
    if (m_llama && m_caravanLeader) {
        // TODO: 跟随领袖
        // f32 distance = m_llama->distanceTo(m_caravanLeader);
        // if (distance > MIN_DISTANCE) {
        //     m_llama->getNavigator()->moveTo(m_caravanLeader, m_speed);
        // }
    }
}

// ==================== RunAroundLikeCrazyGoal ====================

RunAroundLikeCrazyGoal::RunAroundLikeCrazyGoal(AbstractHorseEntity* horse, f32 speed)
    : Goal(GoalFlag::MOVE)
    , m_horse(horse)
    , m_speed(speed)
{
    MC_ASSERT_NOT_NULL(horse);
}

bool RunAroundLikeCrazyGoal::shouldExecute() {
    if (!m_horse) return false;

    // 未驯服且正在被骑乘时执行
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;

    // TODO: 获取骑乘者
    // m_rider = m_horse->getRider();
    // return m_rider != nullptr;

    return false;
}

bool RunAroundLikeCrazyGoal::shouldContinueExecuting() {
    if (!m_horse) return false;
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;
    return m_runTime < MAX_RUN_TIME;
}

void RunAroundLikeCrazyGoal::startExecuting() {
    m_runTime = 0;
}

void RunAroundLikeCrazyGoal::resetTask() {
    m_rider = nullptr;
    m_runTime = 0;
}

void RunAroundLikeCrazyGoal::tick() {
    if (!m_horse) return;

    m_runTime++;

    // 随机方向奔跑
    // TODO: 设置随机移动目标
    // math::Random rng(m_horse->ticksExisted());
    // f32 x = m_horse->x() + rng.nextFloat(-10.0f, 10.0f);
    // f32 z = m_horse->z() + rng.nextFloat(-10.0f, 10.0f);
    // m_horse->getNavigator()->moveTo(x, m_horse->y(), z, m_speed);

    // 尝试驯服
    if (m_rider) {
        math::Random rng(m_horse->ticksExisted());
        if (rng.nextFloat() < TAME_CHANCE) {
            // 增加驯服进度
            m_horse->increaseTemper(5);
        }
    }
}

// ==================== DolphinJumpGoal ====================

DolphinJumpGoal::DolphinJumpGoal(DolphinEntity* dolphin)
    : Goal(GoalFlag::MOVE | GoalFlag::JUMP)
    , m_dolphin(dolphin)
{
    MC_ASSERT_NOT_NULL(dolphin);
}

bool DolphinJumpGoal::shouldExecute() {
    if (!m_dolphin || m_jumpCooldown > 0) return false;

    // TODO: 检查是否在水中并接近水面
    // if (!m_dolphin->isInWater()) return false;
    //
    // math::Random rng(m_dolphin->ticksExisted());
    // return rng.nextFloat() < JUMP_CHANCE;

    return false;
}

bool DolphinJumpGoal::shouldContinueExecuting() {
    return m_isJumping;
}

void DolphinJumpGoal::startExecuting() {
    m_isJumping = true;

    // TODO: 设置跳跃速度
    // m_dolphin->setVelocity(m_dolphin->velocityX(), 0.8f, m_dolphin->velocityZ());
}

void DolphinJumpGoal::resetTask() {
    m_isJumping = false;
    m_jumpCooldown = MIN_COOLDOWN + (rand() % (MAX_COOLDOWN - MIN_COOLDOWN));
}

void DolphinJumpGoal::tick() {
    if (!m_dolphin) return;

    // TODO: 检查是否跳出水面
    // if (!m_dolphin->isInWater() && m_dolphin->isOnGround()) {
    //     resetTask();
    // }
}

} // namespace mc
