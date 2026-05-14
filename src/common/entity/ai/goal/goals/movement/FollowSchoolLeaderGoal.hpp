#pragma once

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"

namespace mc {

// 前向声明
class AbstractGroupFishEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随群体领导者目标
 *
 * 使群游鱼类跟随群体领导者。
 * MC 1.16.5: FollowSchoolLeaderGoal
 *
 * 群游行为：
 * 1. 如果自己是首领 → 不执行
 * 2. 如果已有首领 → 继续跟随
 * 3. 冷却结束 → 搜索附近鱼群，找可扩群的首领或自己成为首领
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

    [[nodiscard]] std::string getTypeName() const override { return "FollowSchoolLeaderGoal"; }

private:
    /**
     * @brief 寻找可加入的群体
     * @return 可扩群的首领实体，如果没找到则返回 nullptr
     */
    [[nodiscard]] AbstractGroupFishEntity* findGroupLeaderToJoin();

    /**
     * @brief 获取新的冷却时间
     *
     * MC 1.16.5: 200 + random.nextInt(200) % 20
     * 结果范围：200~219 ticks（约10~11秒）
     */
    [[nodiscard]] i32 getNewCooldown() const;

    AbstractGroupFishEntity* m_fish;
    AbstractGroupFishEntity* m_leader = nullptr;
    i32 m_navigateTimer = 0;
    i32 m_cooldown = 0; // 搜索冷却
};

} // namespace entity::ai::goal
} // namespace mc
