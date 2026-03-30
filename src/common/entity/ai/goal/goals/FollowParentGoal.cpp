#include "FollowParentGoal.hpp"
#include "../../../entities/passive/basic/AnimalEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/AgeableEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../GoalConstants.hpp"
#include "../../../../world/IWorld.hpp"

namespace mc::entity::ai::goal {

using namespace constants;

FollowParentGoal::FollowParentGoal(AnimalEntity* animal, f64 speed)
    : m_childAnimal(animal)
    , m_speed(speed)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool FollowParentGoal::shouldExecute() {
    if (!m_childAnimal) return false;

    // 检查是否是幼体
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false; // 已成年，不需要跟随父母
    }

    // 寻找成年动物
    m_parentAnimal = findParent();
    return m_parentAnimal != nullptr;
}

bool FollowParentGoal::shouldContinueExecuting() {
    if (!m_childAnimal || !m_parentAnimal) return false;

    // 检查是否成年
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false;
    }

    // 检查父/母是否存活
    if (!m_parentAnimal->isAlive()) {
        return false;
    }

    // 检查距离 - 只有超出最大距离才停止
    f32 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    if (distSq > FOLLOW_PARENT_MAX_DISTANCE_SQ) {
        return false;
    }

    return true;
}

void FollowParentGoal::startExecuting() {
    m_delayCounter = 0;
}

void FollowParentGoal::resetTask() {
    m_parentAnimal = nullptr;
    if (m_childAnimal) {
        m_childAnimal->clearNavigation();
    }
}

void FollowParentGoal::tick() {
    if (!m_childAnimal || !m_parentAnimal) return;

    // 看向父/母
    m_childAnimal->lookAt(*m_parentAnimal);

    // 检查距离 - 太近时只暂停移动，不停止目标
    f32 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    if (distSq < FOLLOW_PARENT_MIN_DISTANCE_SQ) {
        // 距离足够近，暂停移动
        m_childAnimal->clearNavigation();
        return;
    }

    // 定期更新路径
    if (--m_delayCounter <= 0) {
        m_delayCounter = FOLLOW_DELAY_INTERVAL;

        // 移动到父/母
        m_childAnimal->tryMoveTo(
            m_parentAnimal->x(),
            m_parentAnimal->y(),
            m_parentAnimal->z(),
            m_speed
        );
    }
}

AnimalEntity* FollowParentGoal::findParent() {
    if (!m_childAnimal || !m_childAnimal->world()) return nullptr;

    return EntityUtils::findClosestEntity<AnimalEntity>(
        m_childAnimal->world(),
        m_childAnimal->position(),
        FOLLOW_PARENT_SEARCH_RANGE,
        m_childAnimal,
        [](AnimalEntity* animal) {
            // 必须是成年动物
            return animal->getGrowingAge() >= 0;
        }
    );
}

} // namespace mc::entity::ai::goal
