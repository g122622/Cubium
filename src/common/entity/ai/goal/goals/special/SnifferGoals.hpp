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

#include <optional>
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

    [[nodiscard]] std::string getTypeName() const override
    {
        return "SnifferDigGoal";
    }

    /// 搜索半径（对齐 vanilla SnifferAi.Digging 的 10 格搜索）
    static constexpr i32 SEARCH_LENGTH = 10;

protected:
    /// 目标方块必须是可挖掘方块（对齐 vanilla Sniffer.canDig 的方块标签判定）
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;

private:
    /// 本次挖掘达到的最短停留 tick（对齐 vanilla 挖掘动画时长）
    static constexpr i32 DIG_TICKS = 120;
    /// 挖掘完成后嗅探冷却时长（对齐 vanilla SnifferAi.SNIFFING_COOLDOWN_TICKS）
    static constexpr u64 SNIFF_COOLDOWN_TICKS = 9600;

    SnifferEntity* m_sniffer;
    BlockPos m_digPos;
    i32 m_digTicks = 0;
};

namespace sniffer {

/// 闻气味目标是否处于执行中（对齐 vanilla Sniffer.isScenting：getBrain().isActive(Activity.SCENTING)）
[[nodiscard]] bool isScenting(SnifferEntity* sniffer);
/// 嗅探目标是否处于执行中（对齐 vanilla Sniffer.isSniffing：getBrain().isActive(Activity.SNIFFING)）
[[nodiscard]] bool isSniffing(SnifferEntity* sniffer);
/// 是否正被玩家用食物引诱（对齐 vanilla Sniffer.isTempted，Brain 记忆 IS_TEMPTED）
[[nodiscard]] bool isTempted(SnifferEntity* sniffer);
/// 是否处于恐慌逃跑中（对齐 vanilla Sniffer.isPanicking，Brain 记忆 IS_PANICKING）
[[nodiscard]] bool isPanicking(SnifferEntity* sniffer);

} // namespace sniffer

/**
 * @brief 嗅探兽搜索挖掘点目标（对齐 vanilla SnifferAi.Searching）
 *
 * 仅在“嗅探目标已确定”时启动：进入 Searching 状态并在本目标内搜索可挖掘方块，找到后转入
 * SnifferDigGoal（对应 vanilla 设置 SNIFFER_DIGGING 记忆）。持续 600 tick 仍未找到或中途
 * 变得不可挖掘（canDig 为假）时结束，并清除嗅探目标（对齐 vanilla Searching.stop 擦除记忆）。
 *
 * 与 vanilla 的差异：vanilla 由 Brain 的 SNIFF 活动在 SNIFFER_SNIFFING_TARGET + WALK_TARGET
 * 同时存在且未恐慌时激活 Searching；本项目无 Brain，改以实体上的嗅探目标标记等效表达。
 */
class SnifferSearchGoal : public Goal {
public:
    explicit SnifferSearchGoal(SnifferEntity* sniffer);

    ~SnifferSearchGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "SnifferSearchGoal";
    }

    /// 搜索最长持续时间（对齐 vanilla Searching 的 600 tick 超时）
    static constexpr i32 MAX_SEARCH_TICKS = 600;
    /// 单次搜索的方块半径（对齐 vanilla Searching 经 LandRandomPos 的 10 格搜索长度）
    static constexpr i32 SEARCH_LENGTH = 10;

private:
    /// 在嗅探兽周围寻找一个可挖掘且未探索过的方块（对齐 vanilla Sniffer.calculateDigPosition）
    [[nodiscard]] std::optional<BlockPos> findDigPosition() const;

    SnifferEntity* m_sniffer;
    i32 m_searchTicks = 0;
};

/**
 * @brief 嗅探兽闻气味目标（对齐 vanilla SnifferAi.Scenting，持续 40~80 tick）
 *
 * 空闲时进入 Scenting 状态（播放闻气味音效），结束后回到 Idling。
 */
class SnifferScentGoal : public Goal {
public:
    explicit SnifferScentGoal(SnifferEntity* sniffer);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "SnifferScentGoal";
    }

    static constexpr i32 MIN_TICKS = 40;
    static constexpr i32 MAX_TICKS = 80;

private:
    SnifferEntity* m_sniffer;
    i32 m_ticks = 0;
    i32 m_goalTicks = 0;
};

/**
 * @brief 嗅探兽嗅探目标（对齐 vanilla SnifferAi.Sniffing，持续 40~80 tick）
 *
 * 空闲时进入 Sniffing 状态（播放嗅探音效），结束后回到 Idling。
 */
class SnifferSniffGoal : public Goal {
public:
    explicit SnifferSniffGoal(SnifferEntity* sniffer);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "SnifferSniffGoal";
    }

    static constexpr i32 MIN_TICKS = 40;
    static constexpr i32 MAX_TICKS = 80;

private:
    SnifferEntity* m_sniffer;
    i32 m_ticks = 0;
    i32 m_goalTicks = 0;
};

/**
 * @brief 嗅探兽开心目标（对齐 vanilla SnifferAi.FeelingHappy，持续 40~100 tick）
 *
 * 仅在繁殖后（开心标记置位）执行一次：进入 FeelingHappy 状态，结束后清除标记。
 */
class SnifferHappyGoal : public Goal {
public:
    explicit SnifferHappyGoal(SnifferEntity* sniffer);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "SnifferHappyGoal";
    }

    static constexpr i32 MIN_TICKS = 40;
    static constexpr i32 MAX_TICKS = 100;

private:
    SnifferEntity* m_sniffer;
    i32 m_ticks = 0;
    i32 m_goalTicks = 0;
};

} // namespace entity::ai::goal
} // namespace mc