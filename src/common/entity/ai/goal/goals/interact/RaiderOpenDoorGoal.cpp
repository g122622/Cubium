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

#include "RaiderOpenDoorGoal.hpp"

#include "common/entity/ai/goal/goals/interact/OpenDoorGoal.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/world/village/raid/Raid.hpp"
#include "common/world/village/raid/RaiderType.hpp"

namespace mc::entity::ai::goal {

RaiderOpenDoorGoal::RaiderOpenDoorGoal(MobEntity* mob)
    : OpenDoorGoal(mob, false) // 袭击者不关门
{}

bool RaiderOpenDoorGoal::shouldExecute()
{
    // 先检查父类条件（水平碰撞 + 路径上有木门）
    if (!OpenDoorGoal::shouldExecute()) {
        return false;
    }

    // 仅在袭击活跃时开门
    auto* raider = dynamic_cast<AbstractRaiderEntity*>(m_mob);
    if (raider == nullptr) {
        return false;
    }

    auto* raid = raider->getCurrentRaid();
    return raid != nullptr && raid->status() == world::village::raid::RaidStatus::Ongoing;
}

} // namespace mc::entity::ai::goal
