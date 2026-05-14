#include "FollowParentGoal.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/AgeableEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/passive/basic/AnimalEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

using namespace constants;

FollowParentGoal::FollowParentGoal(AnimalEntity* animal, f64 speed)
    : m_childAnimal(animal)
    , m_speed(speed)
{
    // MC 1.16.5: FollowParentGoal 不设置任何 mutex flags
    // 原版: this.setMutexFlags(EnumSet.noneOf(Goal.Flag.class));
    // 这意味着它可以与其他 Move goals 同时运行
}

bool FollowParentGoal::shouldExecute()
{
    if (!m_childAnimal) return false;

    // MC 1.16.5: 检查是否是幼体 (getGrowingAge() < 0)
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false; // 已成年，不需要跟随父母
    }

    // MC 1.16.5: 在 8x4x8 范围内寻找成年动物
    // 使用 getEntitiesWithinAABB(class, boundingBox.grow(8.0D, 4.0D, 8.0D))
    m_parentAnimal = findParent();
    if (!m_parentAnimal) {
        return false;
    }

    // MC 1.16.5: 只有距离 >= 9.0D 时才开始跟随
    f64 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    return distSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ;
}

bool FollowParentGoal::shouldContinueExecuting()
{
    if (!m_childAnimal || !m_parentAnimal) return false;

    // MC 1.16.5: 检查是否成年
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false;
    }

    // MC 1.16.5: 检查父/母是否存活
    if (!m_parentAnimal->isAlive()) {
        return false;
    }

    // MC 1.16.5: 检查距离 - 只有在 [9.0D, 256.0D] 范围内才继续
    f64 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    return distSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ && distSq <= FOLLOW_PARENT_MAX_DISTANCE_SQ;
}

void FollowParentGoal::startExecuting()
{
    m_delayCounter = 0;
}

void FollowParentGoal::resetTask()
{
    m_parentAnimal = nullptr;
    if (m_childAnimal) {
        m_childAnimal->clearNavigation();
    }
}

void FollowParentGoal::tick()
{
    if (!m_childAnimal || !m_parentAnimal) return;

    // MC 1.16.5: tick 方法只有路径更新逻辑，没有 lookAt
    // 等待延迟计数器
    if (--m_delayCounter <= 0) {
        m_delayCounter = FOLLOW_DELAY_INTERVAL; // 10 tick

        // MC 1.16.5: 使用 navigator.tryMoveToEntityLiving(parentAnimal, speed)
        if (auto* nav = m_childAnimal->navigator()) {
            static_cast<void>(nav->moveTo(*m_parentAnimal, m_speed));
        }
    }
}

AnimalEntity* FollowParentGoal::findParent()
{
    if (!m_childAnimal || !m_childAnimal->world()) return nullptr;

    // MC 1.16.5: 在 8x4x8 范围内搜索
    // 找最近的成年同类
    return EntityUtils::findClosestEntity<AnimalEntity>(m_childAnimal->world(),
        m_childAnimal->position(),
        FOLLOW_PARENT_SEARCH_RANGE, // 8.0f
        m_childAnimal,
        [](AnimalEntity* animal) {
            // MC 1.16.5: 必须是成年动物 (getGrowingAge() >= 0)
            return animal->getGrowingAge() >= 0;
        });
}

} // namespace mc::entity::ai::goal
