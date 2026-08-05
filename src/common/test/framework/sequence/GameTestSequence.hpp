#pragma once

#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/action/CallbackAction.hpp"
#include "common/test/framework/sequence/SequenceCondition.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace mc::test {

class IGameTestHelper;

/**
 * @brief 测试序列 DSL（流式链）。
 *
 * 对齐基岩版 `GameTestSequence` + JS `GameTestSequence` + Java `GameTestSequence` 三平台并集（见校正 4）。
 * 测试体经 `GameTestHelper::startSequence()` 获取引用，链式调用 `thenXxx` 追加步骤，`_tick(currentTick)`
 * 由 `BaseGameTestInstance` 每 tick 调用以推进。
 *
 * 9 个方法（覆盖三平台）：
 * - `thenExecute(fn)`：立即执行一次回调（execute 型）。
 * - `thenExecuteAfter(delay, fn)`：延迟 `delay` tick 后执行一次（execute 型）。
 * - `thenExecuteFor(tickCount, fn)`：连续 `tickCount` 个 tick 执行同一回调，任一失败则失败。
 * - `thenWait(fn)`：从当前 tick 起每 tick 轮询回调，直到返回通过才进入下一步（wait 型）。
 * - `thenWaitAfter(delay, fn)`：延迟 `delay` tick 后开始每 tick 轮询（wait 型）。
 * - `thenIdle(delayTicks)`：空闲 `delayTicks` tick（= `thenExecuteAfter(delay, noop)`）。
 * - `thenSucceed()`：标记序列成功完成（到达此步即 succeed）。
 * - `thenFail(error)`：标记序列失败（到达此步即 fail，携带 error）。
 * - `thenTrigger()`：返回 `SequenceCondition`，本步不阻塞，测试体在适当时机 `trigger` 后
 *   `assertTriggeredThisTick`（Java 独有，基岩/JS 无）。
 *
 * 序列推进语义：`_tick(currentTick)` 检查当前步的 `startTick` 是否到达；到达则按类型执行；
 * execute/executeFor/wait 完成后进入下一步；succeed/fail 终止序列。任一步返回失败（非 idle/wait 的轮询）
 * 则序列失败，`_tick` 返回该错误。全部步骤完成（无 thenSucceed/thenFail）则序列正常结束（不强制 succeed）。
 */
class GameTestSequence {
public:
    explicit GameTestSequence(IGameTestHelper& helper) noexcept
        : m_helper(helper)
    {}

    ~GameTestSequence() = default;
    GameTestSequence(const GameTestSequence&) = delete;
    GameTestSequence& operator=(const GameTestSequence&) = delete;
    GameTestSequence(GameTestSequence&&) noexcept = default;
    GameTestSequence& operator=(GameTestSequence&&) noexcept = default;

    GameTestSequence& thenExecute(std::function<GameTestResult()> fn);
    GameTestSequence& thenExecuteAfter(i32 delay, std::function<GameTestResult()> fn);
    GameTestSequence& thenExecuteFor(i32 tickCount, std::function<GameTestResult()> fn);
    GameTestSequence& thenWait(std::function<GameTestResult()> fn);
    GameTestSequence& thenWaitAfter(i32 delay, std::function<GameTestResult()> fn);
    GameTestSequence& thenIdle(i32 delayTicks);
    void thenSucceed();
    void thenFail(GameTestError error);

    /**
     * @brief 追加一个触发条件步骤，返回条件对象供测试体后续 assert。
     *
     * 返回 `shared_ptr` 因序列内部也持引用以追踪触发时机。
     */
    [[nodiscard]] std::shared_ptr<SequenceCondition> thenTrigger();

    /**
     * @brief 每 tick 推进序列。
     *
     * @param currentTick 当前测试 tick（由 `BaseGameTestInstance` 传入）。
     * @return nullopt=序列正常推进中或已完成；非 nullopt=序列失败（携带错误）。
     *         序列已完成后再调用返回 nullopt（幂等）。
     */
    [[nodiscard]] GameTestResult tick(i32 currentTick);

    /**
     * @brief 序列是否已完成（成功或失败）。
     */
    [[nodiscard]] bool isComplete() const noexcept { return m_completed; }

    /**
     * @brief 序列是否以 succeed 终止（仅 `thenSucceed` 触发）。
     */
    [[nodiscard]] bool isSucceeded() const noexcept { return m_succeeded; }

private:
    enum class StepKind {
        Execute,
        ExecuteFor,
        Wait,
        Idle,
        Succeed,
        Fail,
        Trigger,
    };

    struct Step {
        StepKind kind;
        i32 startTick = 0; // 相对序列起始的 tick
        i32 duration = 0;  // ExecuteFor 的 tickCount / Idle 的 delay
        std::function<GameTestResult()> fn;
        GameTestError failError;                      // Fail 用
        std::shared_ptr<SequenceCondition> condition; // Trigger 用
        bool started = false;                         // 是否已开始执行（executeFor/wait 的首次）
        i32 executeForRemaining = 0;                  // ExecuteFor 剩余次数
    };

    IGameTestHelper& m_helper;
    std::vector<Step> m_steps;
    i32 m_nextStartTick = 0; // 下一步的起始 tick（相对序列开始）
    std::size_t m_currentStep = 0;
    bool m_completed = false;
    bool m_succeeded = false;
};

} // namespace mc::test
