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

#include "OpenDoorGoal.hpp"
#include <string>

namespace mc {

class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 袭击者开门目标
 *
 * 袭击期间灾厄村民（如卫道士）打开木门的 AI 目标。
 * 继承自 OpenDoorGoal，额外添加了袭击活跃状态检查：
 * 只有当实体正在参与活跃的袭击时，才会尝试开门。
 *
 * 袭击者开门后不会关门（closeDoor = false），这模拟了袭击者冲入建筑的场景——
 * 他们不需要礼貌地关门。
 *
 * 与 BreakDoorGoal 的区别：
 * - BreakDoorGoal：破坏门方块，需要 mobGriefing 游戏规则和难度检查
 * - RaiderOpenDoorGoal：仅切换门的开关状态，无需游戏规则或难度检查
 *
 * 在卫道士的 AI 中，两个目标可以共存：
 * - BreakDoorGoal 优先级 1（破门，仅 Normal/Hard 难度）
 * - RaiderOpenDoorGoal 优先级 2（开门，仅袭击期间）
 * 当门已打开或难度不允许破门时，RaiderOpenDoorGoal 作为后备方案让袭击者通过门。
 */
class RaiderOpenDoorGoal : public OpenDoorGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的袭击者实体（必须是 AbstractRaiderEntity 或其子类）
     */
    explicit RaiderOpenDoorGoal(MobEntity* mob);

    ~RaiderOpenDoorGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "RaiderOpenDoorGoal"; }
};

} // namespace entity::ai::goal
} // namespace mc
