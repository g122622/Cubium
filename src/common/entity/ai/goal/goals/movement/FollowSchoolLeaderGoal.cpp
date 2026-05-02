#include "FollowSchoolLeaderGoal.hpp"
#include "../../../../entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/assert/AssertAll.hpp"

namespace mc::entity::ai::goal {

FollowSchoolLeaderGoal::FollowSchoolLeaderGoal(AbstractGroupFishEntity* fish)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_fish(fish)
{
    MC_ASSERT_RELEASE(fish != nullptr);
}

bool FollowSchoolLeaderGoal::shouldExecute() {
    if (m_fish == nullptr) {
        return false;
    }

    // 如果已经有群首，继续跟随
    if (m_fish->hasGroupLeader()) {
        return true;
    }

    // 如果自己已经是群首，不需要跟随
    if (m_fish->isGroupLeader()) {
        return false;
    }

    // 尝试找到可加入的群体
    m_leader = findGroupToJoin();
    return m_leader != nullptr;
}

bool FollowSchoolLeaderGoal::shouldContinueExecuting() {
    if (m_fish == nullptr) {
        return false;
    }

    // 如果没有群首，停止
    if (!m_fish->hasGroupLeader() && m_leader == nullptr) {
        return false;
    }

    // 如果群首已死亡，停止
    AbstractGroupFishEntity* leader = m_fish->hasGroupLeader() ? m_fish->getGroupLeader() : m_leader;
    if (leader == nullptr || !leader->isAlive()) {
        return false;
    }

    // 如果超出跟随范围太久，停止
    if (!m_fish->inRangeOfGroupLeader() && m_navigateTimer <= 0) {
        return false;
    }

    return true;
}

void FollowSchoolLeaderGoal::startExecuting() {
    if (m_fish == nullptr) {
        return;
    }

    m_navigateTimer = 200; // 最大导航时间（10秒）

    // 如果找到新的群首，加入群体
    if (m_leader != nullptr && !m_fish->hasGroupLeader()) {
        m_fish->joinGroup(*m_leader);
    }
}

void FollowSchoolLeaderGoal::resetTask() {
    m_leader = nullptr;
    m_navigateTimer = 0;
}

void FollowSchoolLeaderGoal::tick() {
    if (m_fish == nullptr) {
        return;
    }

    m_navigateTimer--;

    AbstractGroupFishEntity* leader = m_fish->getGroupLeader();
    if (leader == nullptr) {
        return;
    }

    // 看向群首
    m_fish->lookController()->setLookPosition(
        leader->x(),
        leader->y() + leader->eyeHeight() * 0.5f,
        leader->z(),
        10.0f,  // 头部最大转动角度
        20.0f   // 身体最大转动角度
    );

    // 如果超出范围，移动到群首附近
    if (!m_fish->inRangeOfGroupLeader()) {
        // 使用导航器移动到群首位置
        if (m_fish->navigator() != nullptr) {
            m_fish->tryMoveTo(leader->x(), leader->y(), leader->z(), 1.0);
        }
    } else {
        // 在范围内，清空路径
        if (m_fish->navigator() != nullptr && m_fish->navigator()->hasPath()) {
            // 只有当距离很近时才清空路径
            f32 distSq = m_fish->distanceSqTo(*leader);
            if (distSq < 4.0f) { // 2格内
                m_fish->navigator()->clearPath();
            }
        }
    }
}

AbstractGroupFishEntity* FollowSchoolLeaderGoal::findGroupToJoin() {
    if (m_fish == nullptr || m_fish->world() == nullptr) {
        return nullptr;
    }

    // MC 1.16.5: 搜索范围内的同类群游鱼
    IWorld* world = m_fish->world();
    BlockPos pos(static_cast<i32>(m_fish->x()), static_cast<i32>(m_fish->y()), static_cast<i32>(m_fish->z()));
    f32 range = m_fish->getSchoolingRange();

    // 简化实现：搜索附近实体
    // TODO: 使用世界实体查询接口
    // 当前返回 nullptr，等待实体查询系统完善
    (void)world;
    (void)pos;
    (void)range;

    return nullptr;
}

} // namespace mc::entity::ai::goal
