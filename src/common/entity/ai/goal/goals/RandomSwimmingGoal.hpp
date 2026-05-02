#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../util/math/Vector3.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 水下随机游泳目标
 *
 * 使水生生物在水中随机选择方向游泳。
 * 类似于 RandomWalkingGoal，但专门针对水下环境。
 *
 * 参考 MC 1.16.5 RandomSwimmingGoal
 */
class RandomSwimmingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 游泳速度倍率
     */
    RandomSwimmingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带执行概率）
     * @param creature 拥有此目标的生物
     * @param speed 游泳速度倍率
     * @param chance 执行概率倒数（1/chance 的概率执行）
     */
    RandomSwimmingGoal(CreatureEntity* creature, f64 speed, i32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 强制下次执行
     */
    void makeUpdate() { m_forceUpdate = true; }

    /**
     * @brief 设置执行概率倒数
     */
    void setExecutionChance(i32 chance) { m_executionChance = chance; }

    [[nodiscard]] String getTypeName() const override { return "RandomSwimmingGoal"; }

protected:
    /**
     * @brief 获取随机游泳目标位置
     * @param outPos 输出位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] virtual bool getRandomSwimPosition(Vector3& outPos);

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_executionChance;
    i32 m_timeoutCounter = 0;
    bool m_forceUpdate = false;
};

} // namespace entity::ai::goal
} // namespace mc
