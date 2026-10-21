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
#include "common/entity/ai/goal/goals/special/MoveToBlockGoal.hpp"
#include "common/world/block/BlockPos.hpp"

#include <string>

namespace mc {

class SnifferEntity;

namespace entity::ai::goal {

/**
 * @brief 嗅探兽挖掘目标（对齐 vanilla SnifferAi.Digging）
 *
 * 在可挖掘方块（`#minecraft:sniffer_diggable_block`）上就位后进入 Digging 状态，
 * 由 SnifferEntity::tick 在 DATA_DROP_SEED_AT_TICK 到达时掉落嗅探兽战利品表物品。
 *
 * 与 vanilla 的差异：vanilla 用 Brain + MemoryModuleType.SNIFFER_EXPLORED_POSITIONS 记录
 * 已探索位置并交给 SnifferAi.Searching 挑点；本项目使用等效的 Goal 体系，以 Goal 内部
 * 记录的目标方块代替记忆模块。
 */
class SnifferDigGoal : public MoveToBlockGoal {
public:
    /**
     * @param sniffer 嗅探兽
     * @param speed 移动速度倍率
     */
    SnifferDigGoal(SnifferEntity* sniffer, f64 speed);

    ~SnifferDigGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "SnifferDigGoal"; }

    /// 搜索半径（对齐 vanilla SnifferAi.Digging 的 10 格搜索）
    static constexpr i32 SEARCH_LENGTH = 10;

protected:
    /// 目标方块必须是可挖掘方块（对齐 vanilla Sniffer.canDig 的方块标签判定）
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;

private:
    /// 本次挖掘达到的最短停留 tick（对齐 vanilla 挖掘动画时长）
    static constexpr i32 DIG_TICKS = 120;

    SnifferEntity* m_sniffer;
    BlockPos m_digPos;
    i32 m_digTicks = 0;
};

} // namespace entity::ai::goal
} // namespace mc