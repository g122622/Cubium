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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"

namespace mc {

// Forward declarations
class AxolotlEntity;

namespace entity {
namespace ai {
namespace goal {

/**
 * @brief 美西螈装死目标
 *
 * 美西螈在水中受击时有概率装死。
 * 装死期间：
 * - 不移动、不看向目标
 * - 给予自身再生I效果（200tick）
 * - 不能被作为敌人看到
 * - 持续200tick（10秒）
 */
class AxolotlPlayDeadGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param axolotl 美西螈实体
     */
    explicit AxolotlPlayDeadGoal(AxolotlEntity* axolotl);

    ~AxolotlPlayDeadGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    [[nodiscard]] bool isPreemptible() const override { return false; }

    void startExecuting() override;
    void resetTask() override;
    void tick() override;

private:
    AxolotlEntity* m_axolotl;
};

/**
 * @brief 美西螈攻击目标选择
 *
 * 美西螈攻击的目标包括：
 * - 始终攻击：溺尸、守卫者、远古守卫者
 * - 狩猎（无冷却时）：热带鱼、河豚、鲑鱼、鳕鱼、鱿鱼
 */
class AxolotlTargetGoal : public NearestAttackableTargetGoal<LivingEntity> {
public:
    /**
     * @brief 构造函数
     * @param axolotl 美西螈实体
     */
    explicit AxolotlTargetGoal(AxolotlEntity* axolotl);

    ~AxolotlTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

private:
    AxolotlEntity* m_axolotl;
};

} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
