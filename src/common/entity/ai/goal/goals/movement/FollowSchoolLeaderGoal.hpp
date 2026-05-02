#pragma once

#include "../../Goal.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

// 前向声明
class AbstractGroupFishEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随群体领导者目标
 *
 * 使群游鱼类跟随群体领导者。
 * MC 1.16.5: FollowSchoolLeaderGoal
 */
class FollowSchoolLeaderGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param fish 群游鱼类实体
     */
    explicit FollowSchoolLeaderGoal(AbstractGroupFishEntity* fish);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] String getTypeName() const override { return "FollowSchoolLeaderGoal"; }

private:
    /**
     * @brief 寻找可加入的群体
     * @return 群首实体，如果没找到则返回 nullptr
     */
    [[nodiscard]] AbstractGroupFishEntity* findGroupToJoin();

    AbstractGroupFishEntity* m_fish;
    AbstractGroupFishEntity* m_leader = nullptr;
    i32 m_navigateTimer = 0;
};

} // namespace entity::ai::goal
} // namespace mc
