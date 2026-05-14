#pragma once

#include "../../../../core/Types.hpp"
#include "../Goal.hpp"

namespace mc {

// 前向声明
class AnimalEntity;

namespace entity::ai::goal {

/**
 * @brief 跟随父母目标
 *
 * 幼体动物跟随成年动物。
 *
 * 参考 MC 1.16.5 FollowParentGoal
 */
class FollowParentGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param animal 幼体动物
     * @param speed 移动速度倍率
     */
    FollowParentGoal(AnimalEntity* animal, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FollowParentGoal"; }

protected:
    /**
     * @brief 寻找附近的成年动物
     * @return 成年动物，如果没有则返回 nullptr
     */
    [[nodiscard]] AnimalEntity* findParent();

    AnimalEntity* m_childAnimal;
    f64 m_speed;
    AnimalEntity* m_parentAnimal = nullptr;
    i32 m_delayCounter = 0;
};

} // namespace entity::ai::goal
} // namespace mc
