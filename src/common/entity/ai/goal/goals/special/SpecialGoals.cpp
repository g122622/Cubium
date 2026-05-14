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
#include "../../../../../network/packet/EntityPackets.hpp"
#include "../../../../../util/math/MathUtils.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/MobEntity.hpp"
#include "../../../../entities/monster/basic/CreeperEntity.hpp"
#include "../../../../entities/passive/horse/AbstractHorseEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../EntitySenses.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../util/RandomPositionGenerator.hpp"

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

    // MC 1.16.5 CreeperSwellGoal.shouldExecute():
    // LivingEntity livingentity = this.swellingCreeper.getAttackTarget();
    // return this.swellingCreeper.getCreeperState() > 0
    //     || livingentity != null && this.swellingCreeper.getDistanceSq(livingentity) < 9.0D;

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

    // MC 1.16.5: 清除导航路径，停止移动
    m_creeper->clearNavigation();

    // 记录攻击目标
    m_attackTarget = m_creeper->attackTarget();
}

void CreeperSwellGoal::resetTask()
{
    // MC 1.16.5: 清除攻击目标引用
    m_attackTarget = nullptr;
}

void CreeperSwellGoal::tick()
{
    if (!m_creeper) return;

    // MC 1.16.5 CreeperSwellGoal.tick():
    // if (this.creeperAttackTarget == null) {
    //     this.swellingCreeper.setCreeperState(-1);
    // } else if (this.swellingCreeper.getDistanceSq(this.creeperAttackTarget) > 49.0D) {
    //     this.swellingCreeper.setCreeperState(-1);
    // } else if (!this.swellingCreeper.getEntitySenses().canSee(this.creeperAttackTarget)) {
    //     this.swellingCreeper.setCreeperState(-1);
    // } else {
    //     this.swellingCreeper.setCreeperState(1);
    // }

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
    // MC 1.16.5: 只在未被驯服且被玩家骑乘时执行
    if (!m_horse) return false;

    // 检查是否未被驯服且被骑乘
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;

    // 找到随机目标位置
    return findTarget();
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

    // MC 1.16.5: 每tick有概率增加驯服进度或甩下玩家
    // 有 1/50 的概率执行驯服检查
    math::Random rng = m_horse->getRandom();

    if (rng.nextInt(50) == 0) {
        // 获取骑乘者
        const auto& passengers = m_horse->getPassengers();
        if (passengers.empty()) return;

        IWorld* worldPtr = m_horse->world();
        if (!worldPtr) return;

        Entity* passenger = worldPtr->getEntity(passengers[0]);
        if (!passenger) return;

        // 检查是否为玩家
        if (passenger->legacyType() != LegacyEntityType::Player) return;

        ::mc::Player* player = static_cast<::mc::Player*>(passenger);

        // 驯服检查
        // MC 1.16.5: if (j > 0 && this.horseHost.getRNG().nextInt(j) < i &&
        // !net.minecraftforge.event.ForgeEventFactory.onAnimalTame(horseHost, (PlayerEntity)entity))
        i32 temper = m_horse->getTemper();
        i32 maxTemper = m_horse->getMaxTemper();

        if (maxTemper > 0 && rng.nextInt(maxTemper) < temper) {
            // 达到驯服条件
            // MC 1.16.5: this.horseHost.setTamedBy((PlayerEntity)entity);
            m_horse->setTamedBy(player);
            return;
        }

        // 增加驯服进度
        m_horse->increaseTemper(5);

        // 甩下玩家
        // MC 1.16.5: this.horseHost.removePassengers();
        //            this.horseHost.makeMad();
        //            this.horseHost.world.setEntityState(this.horseHost, (byte)6);
        m_horse->removePassengers();
        m_horse->makeMad();

        // 发送驯服失败状态包（烟雾粒子效果）
        if (worldPtr != nullptr) {
            worldPtr->broadcastEntityStatus(
                m_horse->id(), static_cast<u8>(network::EntityStatusPacket::Status::TamingFailed));
        }
    }
}

bool RunAroundLikeCrazyGoal::findTarget()
{
    if (!m_horse) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator 找随机位置
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

} // namespace mc::entity::ai::goal
