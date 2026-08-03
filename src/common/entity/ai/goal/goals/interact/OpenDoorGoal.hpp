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

#include "DoorInteractGoal.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {

class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 开门目标
 *
 * 使实体在路径上遇到木门时自动打开门，穿过门后根据配置决定是否关门。
 * 与 BreakDoorGoal 不同，开门目标不会破坏门方块，而是通过切换门的开关状态来通过。
 *
 * 当 closeDoor 为 true 时，实体会开门后等待一段时间（20 tick）再关门；
 * 当 closeDoor 为 false 时，实体只会开门而不会主动关门（适用于袭击者等场景）。
 *
 * 典型用法：
 * - 村民等和平生物：OpenDoorGoal(mob, true) — 开门并关门
 * - 袭击者（通过 RaiderOpenDoorGoal）：OpenDoorGoal(mob, false) — 开门但不关门
 */
class OpenDoorGoal : public DoorInteractGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param closeDoor 是否在穿过门后关门。true 表示穿过门后关门，false 表示只开门不关门
     */
    OpenDoorGoal(MobEntity* mob, bool closeDoor);

    ~OpenDoorGoal() override = default;

    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "OpenDoorGoal"; }

private:
    /// 是否在穿过门后关门
    bool m_closeDoor;

    /// 关门倒计时（tick），从 20 开始递减，降到 0 时目标结束
    i32 m_forgetTime = 0;
};

} // namespace entity::ai::goal
} // namespace mc
