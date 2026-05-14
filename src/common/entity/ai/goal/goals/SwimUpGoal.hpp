#pragma once

#include "../../../../core/Types.hpp"
#include "../Goal.hpp"

namespace mc {

// 前向声明
class CreatureEntity;

namespace entity::ai::goal {

/**
 * @brief 向上游泳目标
 *
 * 使水生生物向水面游泳，用于补充空气或其他行为。
 *
 * 参考 MC 1.16.5 SwimUpGoal
 */
class SwimUpGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 游泳速度倍率
     * @param targetY 目标Y高度（可选，默认自动检测水面）
     */
    SwimUpGoal(CreatureEntity* creature, f64 speed, i32 targetY = -1);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SwimUpGoal"; }

private:
    /**
     * @brief 检查是否到达目标高度
     * @return 是否到达
     */
    [[nodiscard]] bool hasReachedTarget() const;

    CreatureEntity* m_creature;
    f64 m_speed;
    i32 m_targetY;
    i32 m_originalTargetY; // 保存构造时传入的目标Y
    i32 m_timeoutCounter = 0;
    bool m_active = false;
};

} // namespace entity::ai::goal
} // namespace mc
