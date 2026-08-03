/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <cmath>
#include <functional>
#include <string>

namespace mc {

namespace entity::ai::goal {

/**
 * @brief 避开实体目标
 *
 * 使生物避开特定类型的实体。
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

private:
    /**
     * @brief 寻找要避开的实体
     * @return 要避开的实体，如果没有则返回 nullptr
     */
    [[nodiscard]] LivingEntity* _findEntityToAvoid();

    /**
     * @brief 寻找远离实体的位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool _findEscapePosition();

    /**
     * @brief 验证逃跑位置是否有效
     *
     * 检查逃跑位置比当前位置更远离目标。
     *
     * @param escapePos 逃跑位置
     * @return 如果逃跑位置有效返回 true
     */
    [[nodiscard]] bool _isEscapePositionValid(const Vector3& escapePos) const;

    CreatureEntity* m_creature;
    f32 m_avoidDistance;
    f64 m_farSpeed;
    f64 m_nearSpeed;
    EntityPredicate m_predicate;
    LivingEntity* m_avoidTarget = nullptr;
    f64 m_escapeX = 0.0;
    f64 m_escapeY = 0.0;
    f64 m_escapeZ = 0.0;

    static constexpr i32 ESCAPE_HORIZONTAL_RANGE = 16;
    static constexpr i32 ESCAPE_VERTICAL_RANGE = 7;
};

} // namespace entity::ai::goal
} // namespace mc
