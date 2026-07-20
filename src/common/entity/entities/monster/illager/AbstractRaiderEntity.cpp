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

#include "AbstractRaiderEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/raid/Raid.hpp"

namespace mc {

AbstractRaiderEntity::AbstractRaiderEntity(EntityInstanceId id)
    : PatrollerEntity(id)
{}

void AbstractRaiderEntity::joinRaid(world::village::raid::Raid* raid, i32 wave)
{
    m_raid = raid;
    m_wave = wave;
    m_canJoinRaid = false;
}

void AbstractRaiderEntity::leaveRaid()
{
    m_raid = nullptr;
    m_wave = 0;
    m_canJoinRaid = true;
}

void AbstractRaiderEntity::startCelebrating()
{
    m_celebrationTime = CELEBRATION_DURATION;
    setState(RaiderState::Celebrating);
}

void AbstractRaiderEntity::tick()
{
    PatrollerEntity::tick();

    // 更新庆祝时间
    if (m_celebrationTime > 0) {
        m_celebrationTime--;
        if (m_celebrationTime <= 0) {
            setState(RaiderState::Neutral);
        }
    }

    // 检查袭击是否仍然有效
    if (m_raid != nullptr) {
        // 如果袭击已结束，离开袭击
        if (m_raid->status() != world::village::raid::RaidStatus::Ongoing) {
            leaveRaid();
        }
    }
}

void AbstractRaiderEntity::die(DamageSource& cause)
{
    // 通知袭击掠夺者死亡
    if (m_raid != nullptr && m_world != nullptr) {
        m_raid->onRaiderDeath(id(), *m_world);
        leaveRaid();
    }

    // 调用父类die方法
    PatrollerEntity::die(cause);
}

} // namespace mc
