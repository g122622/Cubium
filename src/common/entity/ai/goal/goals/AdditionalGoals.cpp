#include "AdditionalGoals.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/Random.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ============================================================================
// EatGrassGoal
// ============================================================================

EatGrassGoal::EatGrassGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE, GoalFlag::LOOK})
    , m_creature(creature)
{
    MC_ASSERT(creature != nullptr);
}

bool EatGrassGoal::shouldExecute() {
    if (!m_creature || !m_creature->isAlive()) {
        return false;
    }

    // 随机概率决定是否吃草
    // TODO: 检查当前位置是否有草
    // 暂时返回false，等待世界实现
    return false;
}

bool EatGrassGoal::shouldContinueExecuting() {
    return m_eatAnimationTick > 0;
}

void EatGrassGoal::startExecuting() {
    m_eatAnimationTick = EAT_DURATION;
    // 停止移动
    m_creature->setVelocity(0.0f, 0.0f, 0.0f);
}

void EatGrassGoal::resetTask() {
    m_eatAnimationTick = 0;
}

void EatGrassGoal::tick() {
    if (m_eatAnimationTick > 0) {
        m_eatAnimationTick--;

        // 在吃草结束时修改方块
        if (m_eatAnimationTick == 4) {
            // TODO: 将草地变为泥土
            // TODO: 恢复羊的羊毛
        }
    }
}

// ============================================================================
// FlyGoal
// ============================================================================

FlyGoal::FlyGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE})
    , m_creature(creature)
    , m_speed(speed)
{
    MC_ASSERT(creature != nullptr);
}

bool FlyGoal::shouldExecute() {
    if (!m_creature || !m_creature->isAlive()) {
        return false;
    }

    // 随机决定是否飞行
    // TODO: 检查是否在地面
    return false;
}

void FlyGoal::startExecuting() {
    findFlightTarget();
    m_flightTime = 0;
}

void FlyGoal::tick() {
    m_flightTime++;

    // 检查是否到达目标或超时
    if (m_flightTime > MAX_FLIGHT_TIME) {
        findFlightTarget();
        m_flightTime = 0;
    }

    // TODO: 设置移动目标
}

bool FlyGoal::findFlightTarget() {
    // 随机选择飞行目标
    math::Random rng;
    f32 angle = rng.nextFloat() * MathUtils::PI * 2.0f;
    f32 distance = rng.nextFloat() * 10.0f + 5.0f;
    f32 height = rng.nextFloat() * (FLIGHT_HEIGHT_MAX - FLIGHT_HEIGHT_MIN) + FLIGHT_HEIGHT_MIN;

    m_targetPos = Vector3(
        m_creature->x() + std::cos(angle) * distance,
        m_creature->y() + height,
        m_creature->z() + std::sin(angle) * distance
    );

    return true;
}

// ============================================================================
// SleepGoal
// ============================================================================

SleepGoal::SleepGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE, GoalFlag::LOOK, GoalFlag::JUMP})
    , m_creature(creature)
{
}

bool SleepGoal::shouldExecute() {
    return isSleepTime() && findBed();
}

bool SleepGoal::shouldContinueExecuting() {
    return isSleepTime();
}

void SleepGoal::startExecuting() {
    // TODO: 移动到床边
}

void SleepGoal::resetTask() {
    // TODO: 离开床
}

bool SleepGoal::isSleepTime() const {
    // TODO: 检查游戏时间
    return false;
}

bool SleepGoal::findBed() {
    // TODO: 寻找附近的床
    return false;
}

// ============================================================================
// WorkAtPoiGoal
// ============================================================================

WorkAtPoiGoal::WorkAtPoiGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE})
    , m_creature(creature)
{
}

bool WorkAtPoiGoal::shouldExecute() {
    return isWorkTime();  // && hasWorkStation();
}

bool WorkAtPoiGoal::shouldContinueExecuting() {
    return isWorkTime();
}

void WorkAtPoiGoal::startExecuting() {
    m_workTime = 0;
    // TODO: 移动到工作站点
}

void WorkAtPoiGoal::resetTask() {
    m_workTime = 0;
}

void WorkAtPoiGoal::tick() {
    m_workTime++;

    // 每隔一段时间执行工作
    if (m_workTime % 100 == 0) {
        performWork();
    }
}

bool WorkAtPoiGoal::isWorkTime() const {
    // TODO: 检查游戏时间（白天工作时间）
    return false;
}

void WorkAtPoiGoal::performWork() {
    // TODO: 根据职业执行工作
}

// ============================================================================
// FindShelterGoal
// ============================================================================

FindShelterGoal::FindShelterGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE})
    , m_creature(creature)
    , m_speed(speed)
{
}

bool FindShelterGoal::shouldExecute() {
    if (!m_creature || !m_creature->isAlive()) {
        return false;
    }

    // 检查是否下雨或夜晚
    // TODO: 检查天气和时间
    return false;  // 暂时返回false
}

bool FindShelterGoal::shouldContinueExecuting() {
    // 继续直到到达遮蔽处或超时
    return m_timeout > 0 && m_shelterPos.x != 0;
}

void FindShelterGoal::startExecuting() {
    findShelterPosition();
    m_timeout = TIMEOUT_MAX;
}

void FindShelterGoal::resetTask() {
    m_timeout = 0;
    m_shelterPos = BlockCoord(0, 0, 0);
}

void FindShelterGoal::tick() {
    m_timeout--;

    // TODO: 移动到遮蔽处
}

bool FindShelterGoal::findShelterPosition() {
    // TODO: 在附近寻找阴影或遮蔽处
    return false;
}

// ============================================================================
// FleeSunGoal
// ============================================================================

FleeSunGoal::FleeSunGoal(CreatureEntity* creature, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE})
    , m_creature(creature)
    , m_speed(speed)
{
}

bool FleeSunGoal::shouldExecute() {
    if (!m_creature || !m_creature->isAlive()) {
        return false;
    }

    // 检查是否在阳光下
    // TODO: 检查天空光照
    return false;
}

bool FleeSunGoal::shouldContinueExecuting() {
    return shouldExecute();
}

void FleeSunGoal::startExecuting() {
    findShadePosition();
}

void FleeSunGoal::resetTask() {
    m_targetPos = BlockCoord(0, 0, 0);
}

bool FleeSunGoal::findShadePosition() {
    // TODO: 寻找附近的阴影位置
    return false;
}

// ============================================================================
// ReturnToHomeGoal
// ============================================================================

ReturnToHomeGoal::ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius)
    : Goal(EnumSet<GoalFlag>{GoalFlag::MOVE})
    , m_creature(creature)
    , m_speed(speed)
    , m_homeRadius(homeRadius)
{
}

bool ReturnToHomeGoal::shouldExecute() {
    if (!m_creature || !m_creature->isAlive()) {
        return false;
    }

    // 检查是否离家太远
    // TODO: 检查距离家的距离
    return false;
}

bool ReturnToHomeGoal::shouldContinueExecuting() {
    return m_timeout > 0;
}

void ReturnToHomeGoal::startExecuting() {
    findPathToHome();
    m_timeout = TIMEOUT_MAX;
}

void ReturnToHomeGoal::tick() {
    m_timeout--;

    // TODO: 移动回家
}

bool ReturnToHomeGoal::findPathToHome() {
    // TODO: 寻找回家的路径
    return false;
}

// ============================================================================
// TradeWithPlayerGoal
// ============================================================================

TradeWithPlayerGoal::TradeWithPlayerGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::LOOK, GoalFlag::MOVE})
    , m_creature(creature)
{
}

bool TradeWithPlayerGoal::shouldExecute() {
    // TODO: 检查是否有玩家正在与村民交易
    return false;
}

bool TradeWithPlayerGoal::shouldContinueExecuting() {
    return m_customer != nullptr && m_customer->isAlive();
}

void TradeWithPlayerGoal::startExecuting() {
    // 面向玩家
}

void TradeWithPlayerGoal::resetTask() {
    m_customer = nullptr;
}

void TradeWithPlayerGoal::tick() {
    // 保持面向玩家
    if (m_customer) {
        m_creature->lookAt(*m_customer);
    }
}

// ============================================================================
// ShowWaresGoal
// ============================================================================

ShowWaresGoal::ShowWaresGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::LOOK})
    , m_creature(creature)
{
}

bool ShowWaresGoal::shouldExecute() {
    // TODO: 检查附近是否有玩家手持绿宝石
    return false;
}

bool ShowWaresGoal::shouldContinueExecuting() {
    return m_displayTime > 0;
}

void ShowWaresGoal::startExecuting() {
    m_displayTime = DISPLAY_DURATION;
}

void ShowWaresGoal::resetTask() {
    m_displayTime = 0;
    m_targetPlayer = nullptr;
}

void ShowWaresGoal::tick() {
    m_displayTime--;

    // 面向玩家
    if (m_targetPlayer) {
        m_creature->lookAt(*m_targetPlayer);
    }
}

// ============================================================================
// HurtByTargetGoal
// ============================================================================

HurtByTargetGoal::HurtByTargetGoal(MobEntity* mob)
    : Goal(EnumSet<GoalFlag>{GoalFlag::TARGET})
    , m_mob(mob)
{
}

bool HurtByTargetGoal::shouldExecute() {
    if (!m_mob || !m_mob->isAlive()) {
        return false;
    }

    // 检查是否最近被攻击
    // TODO: 获取最后攻击者
    return false;
}

bool HurtByTargetGoal::shouldContinueExecuting() {
    return m_attacker != nullptr && m_attacker->isAlive();
}

void HurtByTargetGoal::startExecuting() {
    m_mob->setAttackTarget(m_attacker);
}

void HurtByTargetGoal::resetTask() {
    m_attacker = nullptr;
    m_mob->setAttackTarget(nullptr);
}

// ============================================================================
// NearestAttackableTargetGoal
// ============================================================================

NearestAttackableTargetGoal::NearestAttackableTargetGoal(
    MobEntity* mob, const String& targetClass, bool checkSight, bool nearbyOnly)
    : Goal(EnumSet<GoalFlag>{GoalFlag::TARGET})
    , m_mob(mob)
    , m_targetClass(targetClass)
    , m_checkSight(checkSight)
    , m_nearbyOnly(nearbyOnly)
{
}

bool NearestAttackableTargetGoal::shouldExecute() {
    if (!m_mob || !m_mob->isAlive()) {
        return false;
    }

    if (m_searchCooldown > 0) {
        m_searchCooldown--;
        return false;
    }

    m_target = findNearestTarget();
    m_searchCooldown = SEARCH_INTERVAL;

    return m_target != nullptr;
}

bool NearestAttackableTargetGoal::shouldContinueExecuting() {
    if (!m_target || !m_target->isAlive()) {
        return false;
    }

    // 检查距离
    f32 dist = m_mob->distanceSqTo(*m_target);
    if (dist > TARGET_RANGE * TARGET_RANGE) {
        return false;
    }

    // 检查视线
    if (m_checkSight && !m_mob->canSee(*m_target)) {
        return false;
    }

    return true;
}

void NearestAttackableTargetGoal::startExecuting() {
    m_mob->setAttackTarget(m_target);
}

void NearestAttackableTargetGoal::resetTask() {
    m_target = nullptr;
    m_mob->setAttackTarget(nullptr);
}

LivingEntity* NearestAttackableTargetGoal::findNearestTarget() {
    // TODO: 在范围内搜索目标
    // 需要访问世界来获取附近实体
    return nullptr;
}

} // namespace mc::entity::ai::goal
