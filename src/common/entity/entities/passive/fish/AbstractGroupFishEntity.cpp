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

#include "AbstractGroupFishEntity.hpp"

#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/movement/FollowSchoolLeaderGoal.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/entities/passive/fish/AbstractFishEntity.hpp"
#include <memory>

namespace mc {

// ============================================================================
// 继承链标识（parent = AbstractFishEntity::classInfo()）。透传层无自身同步字段，
// classInfo 仅作父链遍历节点。
// ============================================================================
const entity::EntityClassInfo& AbstractGroupFishEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AbstractGroupFishEntity", &AbstractFishEntity::classInfo()};
    return s_classInfo;
}

void AbstractGroupFishEntity::registerGoals()
{
    // 先调用父类的 registerGoals
    // 父类注册了：PanicGoal(0), AvoidEntityGoal(2), RandomSwimmingGoal(4)
    AbstractFishEntity::registerGoals();

    // 优先级 5: FollowSchoolLeaderGoal - 跟随群首
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::FollowSchoolLeaderGoal>(this));
}

void AbstractGroupFishEntity::moveToGroupLeader()
{
    // 如果已经有群首，导航到群首位置
    if (hasGroupLeader() && navigator() != nullptr) {
        static_cast<void>(navigator()->moveTo(m_groupLeader->x(),
            m_groupLeader->y(),
            m_groupLeader->z(),
            1.0 // 速度
            ));
    }
}

} // namespace mc
