#pragma once

#include "../../../../core/Types.hpp"
#include "../../../../util/math/Vector3.hpp"
#include "../Goal.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 寻找水源目标
 *
 * 当水生生物离开水后，尝试寻找最近的水源。
 *
 * 参考 MC 1.16.5 FindWaterGoal
 */
class FindWaterGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     */
    explicit FindWaterGoal(CreatureEntity* creature);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FindWaterGoal"; }

private:
    /**
     * @brief 寻找最近的水源位置
     * @return 是否找到水源
     */
    [[nodiscard]] bool findWater();

    CreatureEntity* m_creature;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    bool m_foundWater = false;
};

} // namespace entity::ai::goal
} // namespace mc
