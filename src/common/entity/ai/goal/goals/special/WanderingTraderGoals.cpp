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

#include "WanderingTraderGoals.hpp"
#include "../../../../../item/core/ItemStack.hpp"
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../ai/controller/LookController.hpp"
#include "../../../../ai/pathfinding/PathNavigator.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cmath>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace wandering_trader {

// ============================================================================
// UseItemGoal
// ============================================================================

UseItemGoal::UseItemGoal(
    MobEntity* mob, const ItemStack& stack, const ResourceLocation& soundEvent, UseCondition condition)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_mob(mob)
    , m_itemStack(stack)
    , m_soundEvent(soundEvent)
    , m_condition(condition)
{
    MC_ASSERT(m_mob != nullptr);
}

bool UseItemGoal::shouldExecute()
{
    if (m_mob == nullptr) {
        return false;
    }

    // 检查使用条件（MC原版UseItemGoal没有冷却机制，由条件函数防止重复触发）
    return m_condition(m_mob);
}

bool UseItemGoal::shouldContinueExecuting()
{
    // 当实体仍在使用物品时继续
    return m_mob != nullptr && m_mob->isUsingItem();
}

void UseItemGoal::startExecuting()
{
    // 将物品放入主手并开始使用
    m_mob->setMainHandItem(m_itemStack);
    m_mob->setActiveHand(Hand::MainHand);
}

void UseItemGoal::resetTask()
{
    // 清空主手物品
    m_mob->setMainHandItem(ItemStack{});

    // 停止使用物品
    m_mob->stopActiveHand();

    // 播放完成音效
    m_mob->playSound(m_soundEvent, 1.0f, m_mob->getRandom().nextFloat() * 0.2f + 0.9f);
}

// ============================================================================
// LookAtCustomerGoal
// ============================================================================

LookAtCustomerGoal::LookAtCustomerGoal(MobEntity* mob)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    , m_mob(mob)
{
    MC_ASSERT(m_mob != nullptr);
}

bool LookAtCustomerGoal::shouldExecute()
{
    if (m_mob == nullptr) {
        return false;
    }

    // 检查是否有交易中的玩家
    auto* villager = dynamic_cast<AbstractVillagerEntity*>(m_mob);
    if (villager != nullptr && villager->isTrading()) {
        m_customer = villager->getTradingPlayer();
        return m_customer != nullptr && m_customer->isAlive();
    }

    return false;
}

bool LookAtCustomerGoal::shouldContinueExecuting()
{
    if (m_customer == nullptr || !m_customer->isAlive()) {
        return false;
    }

    // 检查是否仍在交易
    auto* villager = dynamic_cast<AbstractVillagerEntity*>(m_mob);
    if (villager == nullptr || !villager->isTrading()) {
        return false;
    }

    // 检查距离是否在范围内
    f32 distSq = m_mob->distanceSqTo(*m_customer);
    if (distSq > LOOK_DISTANCE * LOOK_DISTANCE) {
        return false;
    }

    // 检查是否超时
    return m_lookTime > 0;
}

void LookAtCustomerGoal::startExecuting()
{
    // 使用实体的随机数生成器，确保每次调用产生不同的随机值
    m_lookTime = m_mob->getRandom().nextInt(LOOK_MIN_TIME, LOOK_MAX_TIME);
}

void LookAtCustomerGoal::resetTask()
{
    m_customer = nullptr;
    m_lookTime = 0;
}

void LookAtCustomerGoal::tick()
{
    if (m_customer == nullptr) {
        return;
    }

    // 使用 LookController 控制看向顾客
    if (m_mob->lookController() != nullptr) {
        m_mob->lookController()->setLookPositionWithEntity(
            *m_customer, m_mob->getHorizontalFaceSpeed(), m_mob->getVerticalFaceSpeed());
    }

    m_lookTime--;
}

// ============================================================================
// TradeWithPlayerGoal
// ============================================================================

TradeWithPlayerGoal::TradeWithPlayerGoal(MobEntity* mob)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Jump, GoalFlag::Move})
    , m_mob(mob)
{
    MC_ASSERT(m_mob != nullptr);
}

bool TradeWithPlayerGoal::shouldExecute()
{
    if (m_mob == nullptr || !m_mob->isAlive()) {
        return false;
    }

    // 不在水中、不在空中时才可交易
    if (m_mob->isInWater() || !m_mob->onGround()) {
        return false;
    }

    // 被击退时不可交易（对应 MC Java TradeWithPlayerGoal.canUse() 中的 hurtMarked 检查）
    // hurtMarked 在实体受伤或被施加击退时设为 true，在 EntityTracker 同步速度后重置为 false
    if (m_mob->isHurtMarked()) {
        return false;
    }

    auto* villager = dynamic_cast<AbstractVillagerEntity*>(m_mob);
    if (villager == nullptr) {
        return false;
    }

    // 检查是否有交易中的玩家
    Player* customer = villager->getTradingPlayer();
    if (customer != nullptr && customer->isAlive()) {
        // 检查距离是否在交易范围内（4格以内）
        f32 distSq = m_mob->distanceSqTo(*customer);
        if (distSq <= static_cast<f32>(TRADE_DISTANCE_SQ)) {
            m_customer = customer;
            return true;
        }
    }

    return false;
}

bool TradeWithPlayerGoal::shouldContinueExecuting()
{
    if (m_customer == nullptr || !m_customer->isAlive()) {
        return false;
    }

    auto* villager = dynamic_cast<AbstractVillagerEntity*>(m_mob);
    if (villager == nullptr) {
        return false;
    }

    // 检查玩家是否仍在交易界面
    return villager->isTrading();
}

void TradeWithPlayerGoal::startExecuting()
{
    // 停止导航，使商人在交易时原地不动
    if (m_mob->navigator() != nullptr) {
        m_mob->navigator()->stop();
    }
}

void TradeWithPlayerGoal::resetTask()
{
    m_customer = nullptr;
}

// ============================================================================
// MoveToWanderTargetGoal
// ============================================================================

MoveToWanderTargetGoal::MoveToWanderTargetGoal(MobEntity* trader, f64 stopDistance, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_mob(trader)
    , m_stopDistance(stopDistance)
    , m_speed(speed)
{
    MC_ASSERT(m_mob != nullptr);
}

bool MoveToWanderTargetGoal::shouldExecute()
{
    if (m_mob == nullptr) {
        return false;
    }

    // 获取游荡目标
    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return false;
    }

    BlockPos target = trader->wanderTarget();
    if (target.x == 0 && target.y == 0 && target.z == 0) {
        return false;
    }

    // 检查是否超出停止距离
    return _isOutsideDistance(target, m_stopDistance);
}

bool MoveToWanderTargetGoal::shouldContinueExecuting()
{
    if (m_mob == nullptr) {
        return false;
    }

    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return false;
    }

    BlockPos target = trader->wanderTarget();
    if (target.x == 0 && target.y == 0 && target.z == 0) {
        return false;
    }

    // 仍在目标距离外时继续
    return _isOutsideDistance(target, m_stopDistance);
}

void MoveToWanderTargetGoal::startExecuting()
{
    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return;
    }

    m_wanderTarget = trader->wanderTarget();
}

void MoveToWanderTargetGoal::resetTask()
{
    // 清除游荡目标并停止导航
    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader != nullptr) {
        trader->setWanderTarget(BlockPos(0, 0, 0));
    }

    if (m_mob->navigator() != nullptr) {
        m_mob->navigator()->stop();
    }
}

void MoveToWanderTargetGoal::tick()
{
    if (m_mob == nullptr) {
        return;
    }

    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return;
    }

    BlockPos target = trader->wanderTarget();
    if (target.x == 0 && target.y == 0 && target.z == 0) {
        return;
    }

    // 导航器为空时无法移动
    if (m_mob->navigator() == nullptr) {
        return;
    }

    // 只在导航完成后才发起新的导航请求
    if (!m_mob->navigator()->isDone()) {
        return;
    }

    // 远距离分段接近策略：距离超过中间航点距离时，先向目标方向移动中间航点距离
    if (_isOutsideDistance(target, INTERMEDIATE_DISTANCE)) {
        // 计算从当前位置到目标的方向向量，然后沿该方向前进中间航点距离
        Vector3 direction(static_cast<f32>(target.x - m_mob->x()),
            static_cast<f32>(target.y - m_mob->y()),
            static_cast<f32>(target.z - m_mob->z()));

        // 归一化
        f32 length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (length > 0.001f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
        }

        // 计算中间航点
        f64 waypointX = m_mob->x() + static_cast<f64>(direction.x) * INTERMEDIATE_DISTANCE;
        f64 waypointY = m_mob->y() + static_cast<f64>(direction.y) * INTERMEDIATE_DISTANCE;
        f64 waypointZ = m_mob->z() + static_cast<f64>(direction.z) * INTERMEDIATE_DISTANCE;
        (void)m_mob->navigator()->moveTo(waypointX, waypointY, waypointZ, m_speed);
    } else {
        // 近距离直接导航到最终目标
        (void)m_mob->navigator()->moveTo(
            static_cast<f64>(target.x), static_cast<f64>(target.y), static_cast<f64>(target.z), m_speed);
    }
}

bool MoveToWanderTargetGoal::_isOutsideDistance(const BlockPos& pos, f64 distance) const
{
    if (m_mob == nullptr) {
        return false;
    }

    f64 dx = static_cast<f64>(pos.x) - m_mob->x();
    f64 dy = static_cast<f64>(pos.y) - m_mob->y();
    f64 dz = static_cast<f64>(pos.z) - m_mob->z();
    f64 distSq = dx * dx + dy * dy + dz * dz;

    return distSq > distance * distance;
}

} // namespace wandering_trader
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
