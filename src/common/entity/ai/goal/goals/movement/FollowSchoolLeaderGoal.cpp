#include "FollowSchoolLeaderGoal.hpp"
#include "../../../../../entity/core/EntityUtils.hpp"
#include "../../../../../util/assert/AssertAll.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include "../../../../../world/IWorld.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../entities/passive/fish/AbstractGroupFishEntity.hpp"
#include "../../../controller/LookController.hpp"
#include "../../../pathfinding/PathNavigator.hpp"

namespace mc::entity::ai::goal {

// MC 1.16.5 常量
// 搜索范围：8格半径立方体
constexpr f32 SEARCH_RANGE = 8.0f;
// 导航重算间隔：10 ticks
constexpr i32 NAVIGATE_TIMER_INTERVAL = 10;
// 最大导航时间：200 ticks（10秒）
constexpr i32 MAX_NAVIGATE_TIME = 200;

FollowSchoolLeaderGoal::FollowSchoolLeaderGoal(AbstractGroupFishEntity* fish)
    : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    , m_fish(fish)
    , m_cooldown(getNewCooldown())
{
    MC_ASSERT_RELEASE(fish != nullptr);
}

bool FollowSchoolLeaderGoal::shouldExecute()
{
    if (m_fish == nullptr) {
        return false;
    }

    // MC 1.16.5: 如果自己是首领，不需要跟随
    if (m_fish->isGroupLeader()) {
        return false;
    }

    // MC 1.16.5: 如果已经有首领，继续跟随
    if (m_fish->hasGroupLeader()) {
        return true;
    }

    // MC 1.16.5: 冷却中，不搜索
    if (m_cooldown > 0) {
        --m_cooldown;
        return false;
    }

    // 冷却结束，重置冷却时间
    m_cooldown = getNewCooldown();

    // MC 1.16.5: 搜索附近可加入的群体
    // 使用实体查询接口搜索附近的同类群游鱼
    IWorld* world = m_fish->world();
    if (world == nullptr) {
        return false;
    }

    // 搜索范围内的所有同类群游鱼
    auto nearbyFish = EntityUtils::findEntities<AbstractGroupFishEntity>(world,
        m_fish->position(),
        SEARCH_RANGE,
        m_fish, // 排除自己
        [](AbstractGroupFishEntity* fish) {
            // MC 1.16.5 谓词：可扩群的首领 或 无首领的游离鱼
            return fish->canGroupGrow() || !fish->hasGroupLeader();
        });

    if (nearbyFish.empty()) {
        return false;
    }

    // MC 1.16.5: 尝试找一个可扩群的首领
    AbstractGroupFishEntity* leader = nullptr;
    for (AbstractGroupFishEntity* fish : nearbyFish) {
        if (fish->canGroupGrow()) {
            leader = fish;
            break;
        }
    }

    // MC 1.16.5: 如果没找到可扩群的首领，自己成为首领
    if (leader == nullptr) {
        leader = m_fish;
    }

    // MC 1.16.5: 招募无首领的鱼加入群体
    std::vector<AbstractGroupFishEntity*> followers;
    for (AbstractGroupFishEntity* fish : nearbyFish) {
        if (!fish->hasGroupLeader() && fish != leader) {
            followers.push_back(fish);
        }
    }

    leader->recruitFollowers(followers);

    // MC 1.16.5: 如果找到了可扩群的首领，自己也加入
    // 注意：招募完成后检查首领是否仍然可以扩群
    if (leader != m_fish && leader->canGroupGrow()) {
        m_fish->joinGroup(*leader);
    }

    // 检查自己是否成功加入群体
    return m_fish->hasGroupLeader();
}

bool FollowSchoolLeaderGoal::shouldContinueExecuting()
{
    if (m_fish == nullptr) {
        return false;
    }

    // MC 1.16.5: 有首领 且 在跟随范围内
    return m_fish->hasGroupLeader() && m_fish->inRangeOfGroupLeader();
}

void FollowSchoolLeaderGoal::startExecuting()
{
    m_navigateTimer = 0;
}

void FollowSchoolLeaderGoal::resetTask()
{
    // MC 1.16.5: 离开群体
    m_fish->leaveGroup();
    m_leader = nullptr;
}

void FollowSchoolLeaderGoal::tick()
{
    if (m_fish == nullptr) {
        return;
    }

    AbstractGroupFishEntity* leader = m_fish->getGroupLeader();
    if (leader == nullptr) {
        return;
    }

    // MC 1.16.5: 看向首领
    m_fish->lookController()->setLookPosition(leader->x(),
        leader->y() + leader->eyeHeight() * 0.5f,
        leader->z(),
        10.0f, // 头部最大转动角度
        20.0f  // 身体最大转动角度
    );

    // MC 1.16.5: 每 10 ticks 导航一次
    if (--m_navigateTimer <= 0) {
        m_navigateTimer = NAVIGATE_TIMER_INTERVAL;
        m_fish->moveToGroupLeader();
    }
}

AbstractGroupFishEntity* FollowSchoolLeaderGoal::findGroupLeaderToJoin()
{
    // 此方法已被 shouldExecute 中的逻辑取代
    // 保留声明以符合接口，但不再使用
    return nullptr;
}

i32 FollowSchoolLeaderGoal::getNewCooldown() const
{
    // MC 1.16.5: 200 + random.nextInt(200) % 20
    // 结果范围：200~219 ticks（约10~11秒）
    math::Random& rng = m_fish->world()->getRandom();
    return 200 + rng.nextInt(200) % 20;
}

} // namespace mc::entity::ai::goal
