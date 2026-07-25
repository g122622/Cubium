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

#include "SpecialGoals.hpp"
#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/CreatureEntity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../damage/DamageSource.hpp"
#include "../../../../effect/EffectInstance.hpp"
#include "../../../../effect/EffectType.hpp"
#include "../../../../entities/monster/basic/CreeperEntity.hpp"
#include "../../../../entities/passive/fish/PufferfishEntity.hpp"
#include "../../../../entities/passive/horse/AbstractHorseEntity.hpp"
#include "../../../../entities/passive/horse/LlamaEntity.hpp"
#include "../../../../entities/passive/horse/SkeletonHorseEntity.hpp"
#include "../../../../entities/passive/horse/TraderLlamaEntity.hpp"
#include "../../../../entities/passive/tamable/WolfEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../registry/VanillaEntityTypeKeys.hpp"
#include "../../../EntitySenses.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../util/RandomPositionGenerator.hpp"
#include "../../GoalFlag.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include <cmath>
#include <limits>

namespace mc::entity::ai::goal {

// ============================================================================
// CreeperSwellGoal
// ============================================================================

CreeperSwellGoal::CreeperSwellGoal(CreeperEntity* creeper)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creeper(creeper)
{
    MC_ASSERT(creeper != nullptr);
}

bool CreeperSwellGoal::shouldExecute()
{
    if (!m_creeper) return false;

    // 如果已经在膨胀状态，继续执行
    if (m_creeper->getCreeperState() > 0) {
        return true;
    }

    // 检查是否有攻击目标
    LivingEntity* target = m_creeper->attackTarget();
    if (!target) {
        return false;
    }

    // 检查目标是否存活
    if (!target->isAlive()) {
        return false;
    }

    // 检查距离是否在触发范围内 (3 格)
    f32 distSq = m_creeper->distanceSqTo(*target);
    return distSq < SWELL_TRIGGER_DISTANCE_SQ;
}

void CreeperSwellGoal::startExecuting()
{
    if (!m_creeper) return;

    // 清除导航路径，停止移动
    m_creeper->clearNavigation();

    // 记录攻击目标
    m_attackTarget = m_creeper->attackTarget();
}

void CreeperSwellGoal::resetTask()
{
    // 清除攻击目标引用
    m_attackTarget = nullptr;
}

void CreeperSwellGoal::tick()
{
    if (!m_creeper) return;

    // 情况1: 攻击目标为空 -> 取消膨胀
    if (!m_attackTarget) {
        m_creeper->setCreeperState(-1);
        return;
    }

    // 情况2: 目标已死亡 -> 取消膨胀
    if (!m_attackTarget->isAlive()) {
        m_creeper->setCreeperState(-1);
        m_attackTarget = nullptr;
        return;
    }

    // 情况3: 距离超过 7 格 -> 取消膨胀
    f32 distSq = m_creeper->distanceSqTo(*m_attackTarget);
    if (distSq > SWELL_CANCEL_DISTANCE_SQ) {
        m_creeper->setCreeperState(-1);
        return;
    }

    // 情况4: 无法看到目标 -> 取消膨胀
    // 使用 EntitySenses 缓存的视线检测
    if (!m_creeper->senses()->canSee(*m_attackTarget)) {
        m_creeper->setCreeperState(-1);
        return;
    }

    // 情况5: 所有条件满足 -> 开始/继续膨胀
    m_creeper->setCreeperState(1);
}

// ============================================================================
// RunAroundLikeCrazyGoal
// ============================================================================

RunAroundLikeCrazyGoal::RunAroundLikeCrazyGoal(AbstractHorseEntity* horse, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_horse(horse)
    , m_speed(speed)
{
    MC_ASSERT(horse != nullptr);
}

bool RunAroundLikeCrazyGoal::shouldExecute()
{
    // 只在未被驯服且被玩家骑乘时执行
    if (!m_horse) return false;

    // 检查是否未被驯服且被骑乘
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;

    // 找到随机目标位置
    return _findTarget();
}

bool RunAroundLikeCrazyGoal::shouldContinueExecuting()
{
    if (!m_horse) return false;

    // 检查是否还在被骑乘且未被驯服
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;

    // 检查是否还有路径
    if (auto* nav = m_horse->navigator()) {
        if (nav->noPath()) return false;
    }

    return true;
}

void RunAroundLikeCrazyGoal::startExecuting()
{
    if (!m_horse) return;

    // 移动到目标位置
    if (auto* nav = m_horse->navigator()) {
        static_cast<void>(nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed));
    }
}

void RunAroundLikeCrazyGoal::resetTask()
{
    m_targetX = 0.0;
    m_targetY = 0.0;
    m_targetZ = 0.0;
}

void RunAroundLikeCrazyGoal::tick()
{
    if (!m_horse) return;

    // 每tick有概率增加驯服进度或甩下玩家
    // 有 1/50 的概率执行驯服检查
    math::Random& rng = m_horse->getRandom();

    if (rng.nextInt(50) == 0) {
        // 获取骑乘者
        const auto& passengers = m_horse->getPassengers();
        if (passengers.empty()) return;

        IWorld* worldPtr = m_horse->world();
        if (!worldPtr) return;

        Entity* passenger = worldPtr->getEntity(passengers[0]);
        if (!passenger) return;

        // 检查是否为玩家
        if (passenger->entityType() != entity::VanillaEntityTypeKeys::PLAYER) return;

        ::mc::Player* player = static_cast<::mc::Player*>(passenger);

        // 驯服检查
        i32 temper = m_horse->getTemper();
        i32 maxTemper = m_horse->getMaxTemper();

        if (maxTemper > 0 && rng.nextInt(maxTemper) < temper) {
            // 达到驯服条件
            m_horse->setTamedBy(player);
            return;
        }

        // 增加驯服进度
        m_horse->increaseTemper(5);

        // 甩下玩家
        m_horse->removePassengers();
        m_horse->makeMad();

        // 发送驯服失败状态包（烟雾粒子效果）
        if (worldPtr != nullptr) {
            worldPtr->broadcastEntityStatus(m_horse->id(), static_cast<u8>(network::EntityStatus::TamingFailed));
        }
    }
}

bool RunAroundLikeCrazyGoal::_findTarget()
{
    if (!m_horse) return false;

    // 使用 RandomPositionGenerator 找随机位置
    Vector3 targetPos;
    if (util::RandomPositionGenerator::findRandomTarget(m_horse,
            5, // 水平范围
            4, // 垂直范围
            targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        return true;
    }

    return false;
}

// ============================================================================
// PuffGoal
// ============================================================================

PuffGoal::PuffGoal(::mc::PufferfishEntity* fish)
    : Goal() // 无互斥标志
    , m_fish(fish)
{
    MC_ASSERT(fish != nullptr);
}

bool PuffGoal::shouldExecute()
{
    if (!m_fish || !m_fish->isAlive()) return false;
    if (!m_fish->world()) return false;

    return _findNearbyEnemy();
}

bool PuffGoal::shouldContinueExecuting()
{
    // 与 shouldExecute() 相同逻辑
    return shouldExecute();
}

void PuffGoal::startExecuting()
{
    if (!m_fish) return;

    // 开始膨胀计时器
    m_fish->startPuffTimer();
}

void PuffGoal::resetTask()
{
    if (!m_fish) return;

    // 重置膨胀计时器
    m_fish->resetPuffTimer();
    m_nearbyEnemy = nullptr;
}

bool PuffGoal::_isEnemy(const LivingEntity* entity)
{
    if (!entity) return false;

    // 检查是否为玩家
    if (entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER) {
        // 需要使用 dynamic_cast 安全转换
        const Player* player = dynamic_cast<const Player*>(entity);
        if (player) {
            // 观察者模式或创造模式的玩家不是敌人
            if (player->isSpectator() || player->isCreative()) {
                return false;
            }
        }
        return true; // 非观察者/创造模式的玩家是敌人
    }

    // 检查实体类型是否为水生生物
    const entity::EntityType* type = entity->entityType();
    // 水生生物 - 不是敌人
    if (type == entity::VanillaEntityTypeKeys::COD || type == entity::VanillaEntityTypeKeys::SALMON ||
        type == entity::VanillaEntityTypeKeys::PUFFERFISH || type == entity::VanillaEntityTypeKeys::TROPICAL_FISH ||
        type == entity::VanillaEntityTypeKeys::SQUID || type == entity::VanillaEntityTypeKeys::DOLPHIN ||
        type == entity::VanillaEntityTypeKeys::TURTLE) {
        return false;
    }
    // 其他所有生物都是敌人（包括怪物、陆地动物等）
    return true;
}

bool PuffGoal::_findNearbyEnemy()
{
    if (!m_fish || !m_fish->world()) return false;

    m_nearbyEnemy = nullptr;

    // 获取扩展 2 格的碰撞箱
    AxisAlignedBB searchBox = m_fish->boundingBox().grow(DETECTION_RANGE);

    // 获取范围内的所有生物实体
    std::vector<Entity*> entities = m_fish->world()->getEntitiesInAABB(searchBox, m_fish);

    for (Entity* entity : entities) {
        if (!entity || !entity->isAlive()) continue;

        // 只检测生物实体
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (!living) continue;

        if (_isEnemy(living)) {
            m_nearbyEnemy = living;
            return true;
        }
    }

    return false;
}

// ============================================================================
// LlamaFollowCaravanGoal
// ============================================================================

LlamaFollowCaravanGoal::LlamaFollowCaravanGoal(LlamaEntity* llama, f32 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_llama(llama)
    , m_speed(speed)
    , m_speedModifier(speed)
    , m_distCheckCounter(0)
{
    MC_ASSERT(llama != nullptr);
}

bool LlamaFollowCaravanGoal::shouldExecute()
{
    // 如果自己被拴绳拴住或已在商队中，不能发起加入商队
    if (!m_llama || m_llama->isLeashed() || m_llama->isInCaravan()) {
        return false;
    }

    IWorld* world = m_llama->world();
    if (!world) {
        return false;
    }

    // 搜索附近的羊驼（9 格半径，4 格高度）
    AxisAlignedBB searchBox = m_llama->boundingBox().expand(SEARCH_RADIUS, SEARCH_HEIGHT, SEARCH_RADIUS);
    std::vector<Entity*> entities = world->getEntitiesInAABB(searchBox, m_llama);

    LlamaEntity* bestCandidate = nullptr;
    f64 bestDistanceSq = std::numeric_limits<f64>::max();

    // 第一阶段：寻找已在商队中但无尾部的羊驼（商队链末尾的羊驼）
    for (Entity* entity : entities) {
        const entity::EntityType* type = entity->entityType();
        if (type != entity::VanillaEntityTypeKeys::LLAMA && type != entity::VanillaEntityTypeKeys::TRADER_LLAMA) {
            continue;
        }

        LlamaEntity* otherLlama = static_cast<LlamaEntity*>(entity);
        if (!otherLlama->isAlive()) {
            continue;
        }

        if (otherLlama->isInCaravan() && !otherLlama->hasCaravanTail()) {
            f64 distSq = m_llama->distanceSqTo(*otherLlama);
            if (distSq < bestDistanceSq) {
                bestDistanceSq = distSq;
                bestCandidate = otherLlama;
            }
        }
    }

    // 第二阶段：如果没有找到商队尾部的羊驼，寻找被拴住且无尾部的羊驼
    if (bestCandidate == nullptr) {
        for (Entity* entity : entities) {
            const entity::EntityType* type = entity->entityType();
            if (type != entity::VanillaEntityTypeKeys::LLAMA && type != entity::VanillaEntityTypeKeys::TRADER_LLAMA) {
                continue;
            }

            LlamaEntity* otherLlama = static_cast<LlamaEntity*>(entity);
            if (!otherLlama->isAlive()) {
                continue;
            }

            if (otherLlama->isLeashed() && !otherLlama->hasCaravanTail()) {
                f64 distSq = m_llama->distanceSqTo(*otherLlama);
                if (distSq < bestDistanceSq) {
                    bestDistanceSq = distSq;
                    bestCandidate = otherLlama;
                }
            }
        }
    }

    // 最终条件判断
    if (bestCandidate == nullptr) {
        return false;
    }

    // 距离太近不加入（< 2 格）
    if (bestDistanceSq < MIN_JOIN_DISTANCE_SQ) {
        return false;
    }

    // 目标羊驼自己没被拴住，且沿商队链递归向上也找不到被拴住的羊驼，则商队无效
    if (!bestCandidate->isLeashed() && !_firstIsLeashed(bestCandidate, 1)) {
        return false;
    }

    // 加入商队
    m_llama->joinCaravan(bestCandidate);
    return true;
}

bool LlamaFollowCaravanGoal::shouldContinueExecuting()
{
    if (!m_llama || !m_llama->isInCaravan()) {
        return false;
    }

    LlamaEntity* head = m_llama->getCaravanHead();
    if (!head || !head->isAlive()) {
        return false;
    }

    // 检查商队头领是否被拴绳拴住
    if (!_firstIsLeashed(m_llama, 0)) {
        return false;
    }

    // 检查距离
    f64 distSq = m_llama->distanceSqTo(*head);

    // 如果距离超过 26 格，尝试加速
    if (distSq > MAX_FOLLOW_DISTANCE_SQ) {
        if (m_speedModifier <= 3.0) {
            m_speedModifier *= 1.2;
            m_distCheckCounter = 40;
            return true;
        }

        if (m_distCheckCounter == 0) {
            // 速度已达上限且计数器归零，放弃跟随
            return false;
        }
    }

    if (m_distCheckCounter > 0) {
        --m_distCheckCounter;
    }

    return true;
}

void LlamaFollowCaravanGoal::startExecuting()
{
    m_speedModifier = static_cast<f64>(m_speed);
    m_distCheckCounter = 0;
}

void LlamaFollowCaravanGoal::resetTask()
{
    m_llama->leaveCaravan();
    m_speedModifier = static_cast<f64>(m_speed);
    m_distCheckCounter = 0;
}

void LlamaFollowCaravanGoal::tick()
{
    if (!m_llama || !m_llama->isInCaravan()) {
        return;
    }

    // 如果自己被拴在栅栏柱上，不移动跟随商队
    // C++ 中使用 leashFencePos() 判断：被拴在栅栏时 leashFencePos() 有值
    if (m_llama->isLeashed() && m_llama->leashFencePos().has_value()) {
        return;
    }

    LlamaEntity* head = m_llama->getCaravanHead();
    if (!head) {
        return;
    }

    // 计算到头领的距离
    f64 dist = m_llama->distanceTo(*head);

    // 计算移动向量，保持 2 格间距
    f64 dx = head->x() - m_llama->x();
    f64 dy = head->y() - m_llama->y();
    f64 dz = head->z() - m_llama->z();

    // 归一化
    f64 length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length > 0.001) {
        dx /= length;
        dy /= length;
        dz /= length;
    }

    // 缩放为 (距离 - 2)，保持 2 格间距
    f64 scale = std::max(dist - CARAVAN_FOLLOW_DISTANCE, 0.0);
    dx *= scale;
    dy *= scale;
    dz *= scale;

    // 计算目标位置
    f64 targetX = m_llama->x() + dx;
    f64 targetY = m_llama->y() + dy;
    f64 targetZ = m_llama->z() + dz;

    // 移动到目标位置
    if (auto* nav = m_llama->navigator()) {
        static_cast<void>(nav->moveTo(targetX, targetY, targetZ, m_speedModifier));
    }
}

bool LlamaFollowCaravanGoal::_firstIsLeashed(const LlamaEntity* llama, i32 depth) const
{
    // 递归检查商队链头是否被拴绳拴住
    // 沿 caravanHead 链向上追溯，直到找到一个被拴住的羊驼（返回 true）
    // 或到达链的末端/深度超限（返回 false）
    if (depth > MAX_CARAVAN_LENGTH) {
        return false;
    }

    if (llama->isInCaravan()) {
        LlamaEntity* head = llama->getCaravanHead();
        if (head == nullptr) {
            return false;
        }

        // 如果头领被拴住则返回 true，否则继续递归检查头领的上级
        return head->isLeashed() || _firstIsLeashed(head, depth + 1);
    }

    // 不在商队中（到达链的末端），且自身没被拴住，返回 false
    return false;
}

// ============================================================================
// LlamaDefendTargetGoal
// ============================================================================

LlamaDefendTargetGoal::LlamaDefendTargetGoal(LlamaEntity* llama)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    , m_llama(llama)
    , m_target(nullptr)
{
    MC_ASSERT(llama != nullptr);
}

bool LlamaDefendTargetGoal::shouldExecute()
{
    if (!m_llama || !m_llama->world()) {
        return false;
    }

    // 获取检测范围（基础范围 * 0.25）
    f64 followRange = m_llama->getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE, 40.0);
    f64 targetRange = followRange * TARGET_RANGE_MODIFIER;

    // 搜索附近的狼
    AxisAlignedBB searchBox = m_llama->boundingBox().expand(targetRange, targetRange, targetRange);
    std::vector<Entity*> entities = m_llama->world()->getEntitiesInAABB(searchBox, m_llama);

    m_target = nullptr;
    f64 minDistSq = std::numeric_limits<f64>::max();

    for (Entity* entity : entities) {
        // 只检查狼
        if (entity->entityType() != entity::VanillaEntityTypeKeys::WOLF) {
            continue;
        }

        // 只攻击未驯服的狼
        WolfEntity* wolf = dynamic_cast<WolfEntity*>(entity);
        if (!wolf || !wolf->isAlive()) {
            continue;
        }

        // 检查是否已驯服
        if (wolf->isTamed()) {
            continue;
        }

        f64 distSq = m_llama->distanceSqTo(*wolf);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            m_target = wolf;
        }
    }

    return m_target != nullptr;
}

void LlamaDefendTargetGoal::startExecuting()
{
    // 设置攻击目标
    if (m_llama && m_target) {
        m_llama->setAttackTarget(m_target);
    }
}

void LlamaDefendTargetGoal::resetTask()
{
    m_target = nullptr;
}

// ============================================================================
// TriggerSkeletonTrapGoal
// ============================================================================

TriggerSkeletonTrapGoal::TriggerSkeletonTrapGoal(SkeletonHorseEntity* horse)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_horse(horse)
{
    MC_ASSERT(horse != nullptr);
}

bool TriggerSkeletonTrapGoal::shouldExecute()
{
    // 执行条件: 陷阱马且玩家在 10 格范围内
    if (!m_horse || !m_horse->isAlive()) {
        return false;
    }

    // 必须是陷阱马
    if (!m_horse->isTrap()) {
        return false;
    }

    IWorld* world = m_horse->world();
    if (!world) {
        return false;
    }

    // 检测附近是否有玩家
    Vector3 pos = m_horse->position();
    AxisAlignedBB searchBox =
        m_horse->boundingBox().expand(PLAYER_DETECTION_RANGE, PLAYER_DETECTION_RANGE, PLAYER_DETECTION_RANGE);

    std::vector<Entity*> entities = world->getEntitiesInAABB(searchBox, m_horse);

    for (Entity* entity : entities) {
        // 只检查玩家
        if (entity->entityType() != entity::VanillaEntityTypeKeys::PLAYER) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(entity);
        if (!player || !player->isAlive()) {
            continue;
        }

        // 跳过旁观者和创造模式玩家
        if (player->isSpectator() || player->isCreative()) {
            continue;
        }

        // 检查距离
        f64 distSq = m_horse->distanceSqTo(*player);
        if (distSq <= PLAYER_DETECTION_RANGE_SQ) {
            return true;
        }
    }

    return false;
}

void TriggerSkeletonTrapGoal::tick()
{
    // 触发陷阱
    if (m_horse && m_horse->isAlive() && m_horse->isTrap()) {
        m_horse->triggerTrap();
    }
}

// ============================================================================
// TraderLlamaDefendWanderingTraderGoal
// ============================================================================

TraderLlamaDefendWanderingTraderGoal::TraderLlamaDefendWanderingTraderGoal(TraderLlamaEntity* llama)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    , m_llama(llama)
{}

bool TraderLlamaDefendWanderingTraderGoal::shouldExecute()
{
    // 1. 商队羊驼必须被拴住
    if (!m_llama->isLeashed()) {
        return false;
    }

    // 2. 拴绳持有者必须是流浪商人
    Entity* holder = m_llama->getLeashHolderEntity();
    if (holder == nullptr) {
        return false;
    }
    auto* trader = dynamic_cast<mc::entity::WanderingTraderEntity*>(holder);
    if (trader == nullptr) {
        return false;
    }

    // 3. 流浪商人必须受到过攻击，且攻击时间戳有更新
    LivingEntity* hurtBy = trader->getLastHurtBy();
    if (hurtBy == nullptr) {
        return false;
    }
    i32 traderHurtTimestamp = trader->lastHurtByTimestamp();
    if (traderHurtTimestamp == m_timestamp) {
        return false;
    }

    // 4. 攻击者必须是有效的目标
    if (!hurtBy->isAlive()) {
        return false;
    }
    if (hurtBy == m_llama) {
        return false;
    }

    // 保存攻击者引用
    m_ownerLastHurtBy = hurtBy;
    return true;
}

void TraderLlamaDefendWanderingTraderGoal::startExecuting()
{
    m_llama->setAttackTarget(m_ownerLastHurtBy);

    // 更新时间戳，防止重复触发
    Entity* holder = m_llama->getLeashHolderEntity();
    if (holder != nullptr) {
        auto* trader = dynamic_cast<mc::entity::WanderingTraderEntity*>(holder);
        if (trader != nullptr) {
            m_timestamp = trader->lastHurtByTimestamp();
        }
    }

    Goal::startExecuting();
}

} // namespace mc::entity::ai::goal
