#include "FindWaterGoal.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Material.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../core/Constants.hpp"

namespace mc::entity::ai::goal {

FindWaterGoal::FindWaterGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_creature(creature)
{
    MC_ASSERT(creature != nullptr);
}

bool FindWaterGoal::shouldExecute() {
    if (m_creature == nullptr) {
        return false;
    }

    // 只在不在水中时执行
    if (m_creature->isInWater()) {
        return false;
    }

    // 寻找水源
    return findWater();
}

bool FindWaterGoal::shouldContinueExecuting() {
    if (m_creature == nullptr) {
        return false;
    }

    // 如果已经在水中，停止
    if (m_creature->isInWater()) {
        return false;
    }

    // 如果没有找到水源，停止
    if (!m_foundWater) {
        return false;
    }

    // 检查是否仍有路径
    auto* mob = dynamic_cast<MobEntity*>(m_creature);
    if (mob == nullptr) {
        return false;
    }
    return mob->navigator() != nullptr && mob->navigator()->hasPath();
}

void FindWaterGoal::startExecuting() {
    if (m_creature == nullptr || !m_foundWater) {
        return;
    }

    // 移动到水源
    m_creature->tryMoveTo(m_targetX, m_targetY, m_targetZ, 1.0);
}

void FindWaterGoal::resetTask() {
    m_foundWater = false;
}

void FindWaterGoal::tick() {
    // 每tick检查是否已经到达水中
    if (m_creature != nullptr && m_creature->isInWater()) {
        // 已到达水中，停止导航
        auto* mob = dynamic_cast<MobEntity*>(m_creature);
        if (mob != nullptr && mob->navigator() != nullptr) {
            mob->navigator()->clearPath();
        }
        m_foundWater = false;
    }
}

bool FindWaterGoal::findWater() {
    if (m_creature == nullptr || m_creature->world() == nullptr) {
        return false;
    }

    IWorld* world = m_creature->world();
    BlockPos entityPos(static_cast<i32>(m_creature->x()),
                       static_cast<i32>(m_creature->y()),
                       static_cast<i32>(m_creature->z()));

    // MC 1.16.5: 在实体周围搜索水源
    // 搜索范围：水平方向 16 格，垂直方向 5 格
    constexpr i32 HORIZONTAL_RANGE = 16;
    constexpr i32 VERTICAL_RANGE = 5;

    f64 bestDistance = std::numeric_limits<f64>::max();
    bool found = false;

    for (i32 dx = -HORIZONTAL_RANGE; dx <= HORIZONTAL_RANGE; ++dx) {
        for (i32 dy = -VERTICAL_RANGE; dy <= VERTICAL_RANGE; ++dy) {
            for (i32 dz = -HORIZONTAL_RANGE; dz <= HORIZONTAL_RANGE; ++dz) {
                BlockPos checkPos(entityPos.x + dx, entityPos.y + dy, entityPos.z + dz);

                // 检查是否是水源方块
                const BlockState* state = world->getBlockState(checkPos);
                if (state == nullptr || !state->getMaterial().isLiquid()) {
                    continue;
                }

                // 计算距离
                f64 distance = std::sqrt(
                    static_cast<f64>(dx * dx) +
                    static_cast<f64>(dy * dy) +
                    static_cast<f64>(dz * dz)
                );

                if (distance < bestDistance) {
                    bestDistance = distance;
                    m_targetX = static_cast<f64>(checkPos.x) + 0.5;
                    m_targetY = static_cast<f64>(checkPos.y);
                    m_targetZ = static_cast<f64>(checkPos.z) + 0.5;
                    found = true;
                }
            }
        }
    }

    m_foundWater = found;
    return found;
}

} // namespace mc::entity::ai::goal
