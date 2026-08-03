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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>
#include <string>

namespace mc {
class Block;
namespace entity {
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民夜间睡眠目标
 *
 * 村民在夜间寻找床位并睡眠。
 * 需要先通过POI系统绑定床位。
 */
class SleepAtNightGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit SleepAtNightGoal(VillagerEntity* villager);

    /**
     * @brief 检查是否应该开始执行
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每tick更新
     */
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SleepAtNightGoal"; }

private:
    /**
     * @brief 寻找最近的床位
     * @return 床位位置（如果找到）
     */
    [[nodiscard]] std::optional<BlockPos> _findNearestBed() const;

    /**
     * @brief 移动到床位
     */
    void _moveToBed();

    /**
     * @brief 尝试睡眠
     */
    void _trySleep();

    /**
     * @brief 检查床位是否仍然有效
     * @return 床位是否有效
     */
    [[nodiscard]] bool _isBedStillValid() const;

private:
    VillagerEntity* m_villager;
    BlockPos m_bedPos;
    bool m_sleeping = false;
    i32 m_trySleepTicks = 0;                        // 尝试睡眠的tick计数
    static constexpr i32 MAX_TRY_SLEEP_TICKS = 100; // 最大尝试时间
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
