#include "common/test/framework/ticker/GameTestTicker.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/instance/GameTestState.hpp"

#include <algorithm>

namespace mc::test {

GameTestTicker& GameTestTicker::instance() noexcept
{
    // 局部静态单例（对齐 Java SINGLETON）
    static GameTestTicker s_instance;
    return s_instance;
}

void GameTestTicker::add(BaseGameTestInstance& instance)
{
    m_instances.push_back(&instance);
}

void GameTestTicker::addClearTask(std::unique_ptr<GameTestClearTask> task)
{
    m_clearTasks.push_back(std::move(task));
}

void GameTestTicker::clear()
{
    // 对齐 Java：IDLE 时立即清空；RUNNING 时设 HALTING 待下一轮
    if (m_state == State::Idle) {
        m_instances.clear();
        m_clearTasks.clear();
    } else {
        m_state = State::Halting;
    }
}

void GameTestTicker::forceStop()
{
    m_instances.clear();
    m_clearTasks.clear();
    m_state = State::Idle;
}

void GameTestTicker::tick()
{
    if (m_state == State::Halting) {
        // 上一轮收到 clear，现在清空
        m_instances.clear();
        m_clearTasks.clear();
        m_state = State::Idle;
        return;
    }
    if (m_instances.empty() && m_clearTasks.empty()) {
        return;
    }

    m_state = State::Running;

    // 推进所有测试实例；m_currentTicking 记录正在 tick 的实例（崩溃诊断用）
    for (auto* instance : m_instances) {
        m_currentTicking = instance;
        instance->tick();
    }
    m_currentTicking = nullptr;
    // 移除已完成的实例（对齐 Java removeIf(isDone)）
    m_instances.erase(
        std::remove_if(
            m_instances.begin(), m_instances.end(), [](BaseGameTestInstance* inst) { return isDone(inst->state()); }),
        m_instances.end());

    // 推进清理任务，移除已完成的（tick 返回 true=完成）
    m_clearTasks.erase(std::remove_if(m_clearTasks.begin(),
                           m_clearTasks.end(),
                           [](const std::unique_ptr<GameTestClearTask>& task) {
                               return task && task->tick(); // tick 推进并判定是否完成（完成则移除）
                           }),
        m_clearTasks.end());
    m_state = State::Idle;
}

} // namespace mc::test
