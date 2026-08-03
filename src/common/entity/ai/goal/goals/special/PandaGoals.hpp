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

#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

// Forward declarations
class PandaEntity;

namespace entity::ai::goal {

/**
 * @brief 熊猫打滚目标
 *
 * 顽皮性格的熊猫或幼年熊猫会随机打滚。
 *
 * 行为流程：
 * 1. 检查是否可以执行打滚（幼年或顽皮、在地面、无其他状态）
 * 2. 概率触发或前方是悬崖时触发
 * 3. 设置打滚状态，由 PandaEntity::updateRoll() 处理物理
 *
 * 优先级: 12
 * 互斥标志: MOVE, LOOK, JUMP
 * 不可被抢占
 * 触发概率：
 *   - 前方是悬崖：100%
 *   - 顽皮性格：1/60
 *   - 幼年熊猫：1/500
 */
class PandaRollGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param panda 熊猫实体
     */
    explicit PandaRollGoal(PandaEntity* panda);

    ~PandaRollGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    [[nodiscard]] bool isPreemptible() const override { return false; }
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "PandaRollGoal"; }

private:
    /**
     * @brief 检查前方是否有悬崖
     * @return 如果前方一格下方是空气返回true
     */
    [[nodiscard]] bool _isCliffInFront() const;

    PandaEntity* m_panda;

    // 常量
    static constexpr i32 PLAYFUL_ROLL_CHANCE = 60; // 顽皮熊猫触发概率 1/60
    static constexpr i32 NORMAL_ROLL_CHANCE = 500; // 普通触发概率 1/500
};

} // namespace entity::ai::goal
} // namespace mc
