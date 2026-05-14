#pragma once

#include "../../../../util/math/Vector3.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../core/CreatureEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../Goal.hpp"
#include <cmath>
#include <functional>

namespace mc {

namespace entity::ai::goal {

/**
 * @brief 避开实体目标
 *
 * 使生物避开特定类型的实体。
 *
 * 参考 MC 1.16.5 AvoidEntityGoal
 */
class AvoidEntityGoal : public Goal {
public:
    /**
     * @brief 实体过滤函数类型
     */
    using EntityPredicate = std::function<bool(const LivingEntity*)>;

    /**
     * @brief 构造函数
     * @param creature 生物实体
     * @param avoidDistance 避开距离
     * @param farSpeed 远距离速度
     * @param nearSpeed 近距离速度
     */
    AvoidEntityGoal(CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed);

    /**
     * @brief 构造函数（带过滤条件）
     * @param creature 生物实体
     * @param avoidDistance 避开距离
     * @param farSpeed 远距离速度
     * @param nearSpeed 近距离速度
     * @param predicate 实体过滤条件
     */
    AvoidEntityGoal(
        CreatureEntity* creature, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, EntityPredicate predicate);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "AvoidEntityGoal"; }

protected:
    /**
     * @brief 寻找要避开的实体
     * @return 要避开的实体，如果没有则返回 nullptr
     */
    [[nodiscard]] LivingEntity* findEntityToAvoid();

    /**
     * @brief 寻找远离实体的位置
     * MC 1.16.5: 使用 RandomPositionGenerator.findRandomTargetBlockAwayFrom
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool findEscapePosition();

    /**
     * @brief 验证逃跑位置是否有效
     * MC 1.16.5: 检查逃跑位置比当前位置更远离目标
     * @param escapePos 逃跑位置
     * @return 如果逃跑位置有效返回 true
     */
    [[nodiscard]] bool isEscapePositionValid(const Vector3& escapePos) const;

    CreatureEntity* m_creature;
    f32 m_avoidDistance;
    f64 m_farSpeed;
    f64 m_nearSpeed;
    EntityPredicate m_predicate;
    LivingEntity* m_avoidTarget = nullptr;
    f64 m_escapeX = 0.0;
    f64 m_escapeY = 0.0;
    f64 m_escapeZ = 0.0;

    // MC 1.16.5: RandomPositionGenerator.findRandomTargetBlockAwayFrom 的参数
    static constexpr i32 ESCAPE_HORIZONTAL_RANGE = 16; // 水平搜索范围
    static constexpr i32 ESCAPE_VERTICAL_RANGE = 7;    // 垂直搜索范围
};

} // namespace entity::ai::goal
} // namespace mc
