#include "SpecialGoals.hpp"
#include "../../../../entities/passive/horse/AbstractHorseEntity.hpp"
#include "../../../../entities/player/Player.hpp"
#include "../../../pathfinding/PathNavigator.hpp"
#include "../../../util/RandomPositionGenerator.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../util/math/MathUtils.hpp"

namespace mc::entity::ai::goal {

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

bool RunAroundLikeCrazyGoal::shouldExecute() {
    // MC 1.16.5: 只在未被驯服且被玩家骑乘时执行
    if (!m_horse) return false;

    // 检查是否未被驯服且被骑乘
    if (m_horse->isTame()) return false;
    if (!m_horse->isBeingRidden()) return false;

    // 找到随机目标位置
    return findTarget();
}

bool RunAroundLikeCrazyGoal::shouldContinueExecuting() {
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

void RunAroundLikeCrazyGoal::startExecuting() {
    if (!m_horse) return;

    // 移动到目标位置
    if (auto* nav = m_horse->navigator()) {
        nav->moveTo(m_targetX, m_targetY, m_targetZ, m_speed);
    }
}

void RunAroundLikeCrazyGoal::resetTask() {
    m_targetX = 0.0;
    m_targetY = 0.0;
    m_targetZ = 0.0;
}

void RunAroundLikeCrazyGoal::tick() {
    if (!m_horse) return;

    // MC 1.16.5: 每tick有概率增加驯服进度或甩下玩家
    // 有 1/50 的概率执行驯服检查
    math::Random rng(m_horse->ticksExisted());

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
        i32 temper = m_horse->getTemper();
        i32 maxTemper = m_horse->getMaxTemper();

        if (maxTemper > 0 && rng.nextInt(maxTemper) < temper) {
            // 达到驯服条件
            m_horse->setTame(true);
            // TODO: 触发驯服事件
            // m_horse->setTamedBy(player);
            return;
        }

        // 增加驯服进度
        m_horse->increaseTemper(5);

        // 甩下玩家
        // MC 1.16.5: removePassengers() + makeMad()
        // TODO: 实现甩下玩家逻辑
        // m_horse->removePassengers();
        // m_horse->makeMad();
    }
}

bool RunAroundLikeCrazyGoal::findTarget() {
    if (!m_horse) return false;

    // MC 1.16.5: 使用 RandomPositionGenerator 找随机位置
    Vector3 targetPos;
    if (util::RandomPositionGenerator::findRandomTarget(
            m_horse,
            5,  // 水平范围
            4,  // 垂直范围
            targetPos)) {
        m_targetX = targetPos.x;
        m_targetY = targetPos.y;
        m_targetZ = targetPos.z;
        return true;
    }

    return false;
}

} // namespace mc::entity::ai::goal
