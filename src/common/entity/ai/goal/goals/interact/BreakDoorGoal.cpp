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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BreakDoorGoal.hpp"

#include "common/entity/ai/goal/goals/interact/DoorInteractGoal.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include <algorithm>
#include <utility>

namespace mc::entity::ai::goal {

BreakDoorGoal::BreakDoorGoal(MobEntity* mob, DifficultyPredicate validDifficulties)
    : DoorInteractGoal(mob)
    , m_validDifficulties(std::move(validDifficulties))
{}

BreakDoorGoal::BreakDoorGoal(MobEntity* mob, i32 doorBreakTime, DifficultyPredicate validDifficulties)
    : DoorInteractGoal(mob)
    , m_validDifficulties(std::move(validDifficulties))
    , m_customDoorBreakTime(doorBreakTime)
{}

i32 BreakDoorGoal::getDoorBreakTime() const
{
    return std::max(DEFAULT_DOOR_BREAK_TIME, m_customDoorBreakTime);
}

bool BreakDoorGoal::_isValidDifficulty() const
{
    if (!m_mob || !m_mob->world()) {
        return false;
    }
    return m_validDifficulties(m_mob->world()->difficulty());
}

bool BreakDoorGoal::shouldExecute()
{
    // 先检查父类条件（水平碰撞 + 找到木门）
    if (!DoorInteractGoal::shouldExecute()) {
        return false;
    }

    if (!m_mob || !m_mob->world()) {
        return false;
    }

    // 检查 mobGriefing 游戏规则
    if (!m_mob->world()->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        return false;
    }

    // 检查难度是否允许破门
    if (!_isValidDifficulty()) {
        return false;
    }

    // 已经打开的门不需要破坏
    if (_isDoorOpen()) {
        return false;
    }

    return true;
}

void BreakDoorGoal::startExecuting()
{
    DoorInteractGoal::startExecuting();
    m_breakTime = 0;
    m_lastBreakProgress = -1;
}

bool BreakDoorGoal::shouldContinueExecuting()
{
    const i32 breakTime = getDoorBreakTime();

    // 破门未完成
    if (m_breakTime > breakTime) {
        return false;
    }

    // 门已被打开（被其他实体打开），停止破门
    if (_isDoorOpen()) {
        return false;
    }

    // 生物距离门太远（超过2方块中心距离）
    if (m_mob && m_hasDoor) {
        f32 distSq = math::distanceHorizontalSq(m_mob->position().x,
            m_mob->position().z,
            static_cast<f32>(m_doorPos.x) + 0.5f,
            static_cast<f32>(m_doorPos.z) + 0.5f);
        if (distSq > 4.0f) { // 2.0^2 = 4.0
            return false;
        }
    }

    // 难度仍然有效
    if (!_isValidDifficulty()) {
        return false;
    }

    return true;
}

void BreakDoorGoal::resetTask()
{
    DoorInteractGoal::resetTask();

    // 移除客户端的方块破坏动画
    // 对应 MC Java: this.mob.level().destroyBlockProgress(this.mob.getId(), this.doorPos, -1)
    if (m_mob && m_mob->world() && m_hasDoor) {
        m_mob->world()->destroyBlockProgress(m_mob->id(), m_doorPos, -1);
    }

    m_breakTime = 0;
    m_lastBreakProgress = -1;
}

void BreakDoorGoal::tick()
{
    DoorInteractGoal::tick();

    if (!m_mob || !m_mob->world() || !m_hasDoor) {
        return;
    }

    const i32 breakTime = getDoorBreakTime();

    // 随机播放攻击音效和挥臂动画（平均每20 tick一次）
    if (m_breakTime % 20 == 0 && m_breakTime > 0) {
        // 播放僵尸攻击木门音效
        m_mob->world()->playEvent(world::WorldEvents::ZOMBIE_ATTACK_DOOR_WOOD_SOUND, m_doorPos, 0);

        // 挥臂动画
        m_mob->swingArm();
    }

    m_breakTime++;

    // 计算破坏阶段（0-9，对应客户端的裂纹纹理）
    i32 progress = static_cast<i32>(static_cast<f32>(m_breakTime) / static_cast<f32>(breakTime) * 10.0f);
    progress = std::min(progress, 9);

    // 阶段变化时同步到客户端
    if (progress != m_lastBreakProgress) {
        m_lastBreakProgress = progress;
        // 发送方块破坏进度动画
        // 对应 MC Java: this.mob.level().destroyBlockProgress(this.mob.getId(), this.doorPos, i)
        m_mob->world()->destroyBlockProgress(m_mob->id(), m_doorPos, progress);
    }

    // 破门完成
    if (m_breakTime >= breakTime && _isValidDifficulty()) {
        // 移除门方块（仅移除下半部分，上半部分由 DoorBlock::updatePostPlacement 自动处理）
        const BlockState* doorState = m_mob->world()->getBlockState(m_doorPos);
        if (doorState) {
            // 播放门被破坏的音效
            m_mob->world()->playEvent(world::WorldEvents::ZOMBIE_BREAK_DOOR_WOOD_SOUND, m_doorPos, 0);

            // 播放方块破坏粒子效果
            m_mob->world()->playEvent(
                world::WorldEvents::BREAK_BLOCK_EFFECTS, m_doorPos, static_cast<i32>(doorState->stateId()));

            // 将门方块设为空气
            const BlockState* airState = BlockRegistry::instance().airState();
            m_mob->world()->setBlockState(m_doorPos, airState, 3);
        }
    }
}

} // namespace mc::entity::ai::goal
