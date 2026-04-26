#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../util/math/Vector3.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 随机漫步目标
 *
 * 使生物随机选择一个方向并移动过去。
 *
 * 参考 MC 1.16.5 RandomWalkingGoal
 */
class RandomWalkingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带执行概率）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 执行概率（1/chance 的概率执行）
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance);

    /**
     * @brief 构造函数（带执行概率和空闲时间检查）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 执行概率（1/chance 的概率执行）
     * @param checkIdleTime 是否检查空闲时间（如果空闲时间>=100则不执行）
     */
    RandomWalkingGoal(CreatureEntity* creature, f64 speed, i32 chance, bool checkIdleTime);

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
     * @brief 设置执行概率
     */
    void setExecutionChance(i32 chance) { m_executionChance = chance; }

    [[nodiscard]] String getTypeName() const override { return "RandomWalkingGoal"; }

protected:
    /**
     * @brief 获取随机目标位置
     * @return 目标位置，如果没有有效位置则返回空
     */
    [[nodiscard]] virtual bool getRandomPosition(Vector3& outPos);

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_targetX = 0.0f;
    f32 m_targetY = 0.0f;
    f32 m_targetZ = 0.0f;
    i32 m_executionChance;
    i32 m_timeoutCounter = 0;
    bool m_forceUpdate = false;
    bool m_checkIdleTime = true;  // MC 1.16.5: 默认检查空闲时间
};

} // namespace entity::ai::goal
} // namespace mc
