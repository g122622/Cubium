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
#include "../../../../../util/assert/AssertMacros.hpp"
#include "../../../../../util/math/MathConstants.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/Vector3.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../effect/EffectInstance.hpp"
#include "../../../../effect/EffectType.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../../entities/villager/VillagerEntity.hpp"
#include "../../../../../item/Items.hpp"
#include "../../../../../item/core/ItemStack.hpp"
#include <cmath>

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace wandering_trader {

// ============================================================================
// UseItemGoal - 简化实现，待后续完善药水系统
// ============================================================================

UseItemGoal::UseItemGoal(MobEntity* mob, const ItemStack& stack,
                         const ResourceLocation& soundEvent, UseCondition condition)
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
    // 简化实现：检查条件
    if (m_mob == nullptr) {
        return false;
    }

    // 冷却中不执行
    if (m_cooldown > 0) {
        return false;
    }

    // 检查使用条件
    return m_condition(m_mob);
}

void UseItemGoal::startExecuting()
{
    m_isUsing = true;
    m_useDuration = 0;
}

void UseItemGoal::resetTask()
{
    m_isUsing = false;
    m_useDuration = 0;
    m_cooldown = COOLDOWN_TICKS;
}

void UseItemGoal::tick()
{
    if (!m_isUsing) {
        return;
    }

    m_useDuration++;

    // 物品使用完成
    if (m_useDuration >= ITEM_USE_DURATION) {
        applyItemEffect();
        resetTask();
    }
}

void UseItemGoal::applyItemEffect()
{
    // MC 1.16.5: 应用物品效果
    // 根据物品类型决定效果：
    // - 牛奶桶：清除所有效果
    // - 药水：添加对应效果

    if (m_mob == nullptr) {
        return;
    }

    // 检查物品类型
    const Item* item = m_itemStack.getItem();
    if (item == nullptr) {
        return;
    }

    // 牛奶桶 - 清除所有效果
    if (item == Items::MILK_BUCKET) {
        m_mob->removeAllEffects();
        return;
    }

    // 药水 - 添加隐身效果（流浪商人夜间使用）
    // 简化实现：直接添加隐身效果
    // 完整实现应该从药水物品中读取效果
    effect::EffectInstance invisibilityEffect(
        effect::EffectType::Invisibility,
        1200,  // 持续时间：60秒 = 1200 ticks
        0      // 等级
    );
    m_mob->addEffect(invisibilityEffect);
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
        m_customer = villager->tradingPlayer();
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

    // 检查是否超时
    return m_lookTime > 0;
}

void LookAtCustomerGoal::startExecuting()
{
    // 设置随机看向时间
    math::Random rng;
    m_lookTime = rng.nextInt(LOOK_MIN_TIME, LOOK_MAX_TIME);
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

    // 简化实现：直接设置旋转角度
    // TODO: 使用 LookController 当其接口完善后
    f64 dx = m_customer->x() - m_mob->x();
    f64 dz = m_customer->z() - m_mob->z();
    f64 distance = std::sqrt(dx * dx + dz * dz);

    if (distance > 0.001) {
        // 计算yaw角度
        f64 yaw = std::atan2(-dx, dz) * 180.0 / mc::math::PI_DOUBLE;
        m_mob->setRotation(static_cast<f32>(yaw), m_mob->pitch());
    }

    m_lookTime--;
}

// ============================================================================
// TradeWithPlayerGoal
// ============================================================================

TradeWithPlayerGoal::TradeWithPlayerGoal(MobEntity* mob)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    , m_mob(mob)
{
    MC_ASSERT(m_mob != nullptr);
}

bool TradeWithPlayerGoal::shouldExecute()
{
    if (m_mob == nullptr) {
        return false;
    }

    auto* villager = dynamic_cast<AbstractVillagerEntity*>(m_mob);
    if (villager == nullptr) {
        return false;
    }

    // 检查是否有交易中的玩家
    Player* customer = villager->tradingPlayer();
    if (customer != nullptr && customer->isAlive()) {
        m_customer = customer;
        return true;
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
    // 停止移动
    // TODO: 使用 PathNavigator 当其接口完善后

    // 看向顾客
    if (m_customer != nullptr) {
        f64 dx = m_customer->x() - m_mob->x();
        f64 dz = m_customer->z() - m_mob->z();
        f64 distance = std::sqrt(dx * dx + dz * dz);

        if (distance > 0.001) {
            f64 yaw = std::atan2(-dx, dz) * 180.0 / mc::math::PI_DOUBLE;
            m_mob->setRotation(static_cast<f32>(yaw), m_mob->pitch());
        }
    }
}

void TradeWithPlayerGoal::resetTask()
{
    m_customer = nullptr;
}

void TradeWithPlayerGoal::tick()
{
    if (m_customer == nullptr) {
        return;
    }

    // 保持面向顾客
    f64 dx = m_customer->x() - m_mob->x();
    f64 dz = m_customer->z() - m_mob->z();
    f64 distance = std::sqrt(dx * dx + dz * dz);

    if (distance > 0.001) {
        f64 yaw = std::atan2(-dx, dz) * 180.0 / mc::math::PI_DOUBLE;
        m_mob->setRotation(static_cast<f32>(yaw), m_mob->pitch());
    }
}

// ============================================================================
// MoveToWanderTargetGoal
// ============================================================================

MoveToWanderTargetGoal::MoveToWanderTargetGoal(MobEntity* trader, f64 maxDistance, f64 speed)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_mob(trader)
    , m_maxDistance(maxDistance)
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

    // 检查是否超出最大距离
    return isOutsideDistance(target, m_maxDistance);
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

    // 简化实现：检查是否接近目标
    return isOutsideDistance(target, 1.0);
}

void MoveToWanderTargetGoal::startExecuting()
{
    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return;
    }

    m_wanderTarget = trader->wanderTarget();

    // 简化实现：设置移动目标
    // TODO: 使用 PathNavigator 当其接口完善后
}

void MoveToWanderTargetGoal::resetTask()
{
    // 清除游荡目标
    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader != nullptr) {
        trader->setWanderTarget(BlockPos(0, 0, 0));
    }
}

void MoveToWanderTargetGoal::tick()
{
    // 导航在 startExecuting 中已设置
    // 简化实现：检查是否接近目标
    if (m_mob == nullptr) {
        return;
    }

    auto* trader = dynamic_cast<WanderingTraderEntity*>(m_mob);
    if (trader == nullptr) {
        return;
    }

    BlockPos target = trader->wanderTarget();
    if (!isOutsideDistance(target, 1.0)) {
        // 已接近目标，完成任务
        trader->setWanderTarget(BlockPos(0, 0, 0));
    }
}

bool MoveToWanderTargetGoal::isOutsideDistance(const BlockPos& pos, f64 distance) const
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

Vector3 MoveToWanderTargetGoal::calculateMoveTarget(const BlockPos& target) const
{
    if (m_mob == nullptr) {
        return Vector3(static_cast<f32>(target.x), static_cast<f32>(target.y), static_cast<f32>(target.z));
    }

    // MC 1.16.5: 计算从当前位置到目标的方向向量，然后扩展10格
    Vector3 direction(
        static_cast<f32>(target.x - m_mob->x()),
        static_cast<f32>(target.y - m_mob->y()),
        static_cast<f32>(target.z - m_mob->z())
    );

    // 归一化
    f32 length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (length > 0.001f) {
        direction.x /= length;
        direction.y /= length;
        direction.z /= length;
    }

    // 扩展10格
    constexpr f32 EXTEND_DISTANCE = 10.0f;
    Vector3 moveTarget(
        m_mob->x() + direction.x * EXTEND_DISTANCE,
        m_mob->y() + direction.y * EXTEND_DISTANCE,
        m_mob->z() + direction.z * EXTEND_DISTANCE
    );

    return moveTarget;
}

} // namespace wandering_trader
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
