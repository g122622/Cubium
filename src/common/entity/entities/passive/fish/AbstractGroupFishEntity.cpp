#include "AbstractGroupFishEntity.hpp"
#include "../../../ai/pathfinding/PathNavigator.hpp"

namespace mc {

void AbstractGroupFishEntity::moveToGroupLeader()
{
    // MC 1.16.5: 如果已经有群首，导航到群首位置
    if (hasGroupLeader() && navigator() != nullptr) {
        navigator()->moveTo(
            m_groupLeader->x(),
            m_groupLeader->y(),
            m_groupLeader->z(),
            1.0  // 速度
        );
    }
}

} // namespace mc
