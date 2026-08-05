#include "common/test/framework/sequence/GameTestSequence.hpp"

#include "common/test/framework/helper/IGameTestHelper.hpp"

namespace mc::test {

GameTestSequence& GameTestSequence::thenExecute(std::function<GameTestResult()> fn)
{
    Step step;
    step.kind = StepKind::Execute;
    step.startTick = m_nextStartTick;
    step.fn = std::move(fn);
    m_steps.push_back(std::move(step));
    m_nextStartTick += 1; // 下一步最早下一 tick
    return *this;
}

GameTestSequence& GameTestSequence::thenExecuteAfter(i32 delay, std::function<GameTestResult()> fn)
{
    Step step;
    step.kind = StepKind::Execute;
    step.startTick = m_nextStartTick + delay;
    step.fn = std::move(fn);
    m_steps.push_back(std::move(step));
    m_nextStartTick = step.startTick + 1;
    return *this;
}

GameTestSequence& GameTestSequence::thenExecuteFor(i32 tickCount, std::function<GameTestResult()> fn)
{
    Step step;
    step.kind = StepKind::ExecuteFor;
    step.startTick = m_nextStartTick;
    step.duration = tickCount;
    step.executeForRemaining = tickCount;
    step.fn = std::move(fn);
    m_steps.push_back(std::move(step));
    m_nextStartTick = step.startTick + tickCount;
    return *this;
}

GameTestSequence& GameTestSequence::thenWait(std::function<GameTestResult()> fn)
{
    Step step;
    step.kind = StepKind::Wait;
    step.startTick = m_nextStartTick;
    step.fn = std::move(fn);
    m_steps.push_back(std::move(step));
    // wait 完成时机不确定（取决于回调何时返回通过），完成后下一步最早下一 tick；
    // startTick 在完成时按 currentTick 推进，此处先设保守值
    m_nextStartTick += 1;
    return *this;
}

GameTestSequence& GameTestSequence::thenWaitAfter(i32 delay, std::function<GameTestResult()> fn)
{
    Step step;
    step.kind = StepKind::Wait;
    step.startTick = m_nextStartTick + delay;
    step.fn = std::move(fn);
    m_steps.push_back(std::move(step));
    m_nextStartTick = step.startTick + 1;
    return *this;
}

GameTestSequence& GameTestSequence::thenIdle(i32 delayTicks)
{
    Step step;
    step.kind = StepKind::Idle;
    step.startTick = m_nextStartTick;
    step.duration = delayTicks;
    m_steps.push_back(std::move(step));
    m_nextStartTick = step.startTick + delayTicks;
    return *this;
}

void GameTestSequence::thenSucceed()
{
    Step step;
    step.kind = StepKind::Succeed;
    step.startTick = m_nextStartTick;
    m_steps.push_back(std::move(step));
    m_nextStartTick += 1;
}

void GameTestSequence::thenFail(GameTestError error)
{
    Step step;
    step.kind = StepKind::Fail;
    step.startTick = m_nextStartTick;
    step.failError = std::move(error);
    m_steps.push_back(std::move(step));
    m_nextStartTick += 1;
}

std::shared_ptr<SequenceCondition> GameTestSequence::thenTrigger()
{
    Step step;
    step.kind = StepKind::Trigger;
    step.startTick = m_nextStartTick;
    step.condition = std::make_shared<SequenceCondition>();
    m_steps.push_back(std::move(step));
    m_nextStartTick += 1;
    return m_steps.back().condition;
}

GameTestResult GameTestSequence::tick(i32 currentTick)
{
    if (m_completed) {
        return std::nullopt; // 幂等
    }
    // 跳过尚未到达起始 tick 的当前步（等待）
    while (m_currentStep < m_steps.size()) {
        Step& step = m_steps[m_currentStep];
        if (currentTick < step.startTick) {
            return std::nullopt; // 当前步未到时，等待
        }
        const bool isWait = (step.kind == StepKind::Wait);
        const bool isIdle = (step.kind == StepKind::Idle);
        const bool isExecuteFor = (step.kind == StepKind::ExecuteFor);

        if (isIdle) {
            // Idle：等 duration tick 后通过
            if (currentTick >= step.startTick + step.duration) {
                ++m_currentStep;
                continue; // 立即检查下一步是否也到达
            }
            return std::nullopt;
        }

        if (step.kind == StepKind::Succeed) {
            m_succeeded = true;
            m_completed = true;
            return std::nullopt;
        }
        if (step.kind == StepKind::Fail) {
            m_completed = true;
            return step.failError;
        }
        if (step.kind == StepKind::Trigger) {
            // trigger 本身不阻塞，condition 由测试体单独 assert；进下一步
            ++m_currentStep;
            continue;
        }

        // Execute / ExecuteFor / Wait：调用 fn
        const GameTestResult result = step.fn ? step.fn() : std::nullopt;

        if (isWait) {
            // wait：返回通过才进下一步；返回失败则继续等（不终止序列）
            if (!result.has_value()) {
                ++m_currentStep;
                continue;
            }
            return std::nullopt; // 继续等待
        }

        if (isExecuteFor) {
            // executeFor：每次通过 remaining--，失败则终止
            if (result.has_value()) {
                m_completed = true;
                return result;
            }
            --step.executeForRemaining;
            if (step.executeForRemaining <= 0) {
                ++m_currentStep;
                continue;
            }
            return std::nullopt; // 还有剩余 tick
        }

        // Execute：通过进下一步，失败终止
        if (result.has_value()) {
            m_completed = true;
            return result;
        }
        ++m_currentStep;
        // 继续检查下一步是否也在本 tick 到达
    }
    // 全部步骤完成（未遇 thenSucceed/thenFail）——序列正常结束
    m_completed = true;
    return std::nullopt;
}

} // namespace mc::test
