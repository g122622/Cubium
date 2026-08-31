#pragma once

#include "common/test/framework/ticker/GameTestClearTask.hpp"

#include <memory>
#include <vector>

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief GameTest tick 驱动器（单例 + 3 态）。
 *
 * 对齐 Java 1.21.11 `GameTestTicker.SINGLETON`（见校正 7）：单例 + `State{IDLE, RUNNING, HALTING}` 三态 +
 * 持有当前运行的测试实例列表 + 清理任务列表。`tick()` 每 tick 推进所有实例，移除已完成者；`clear()` 进入
 * HALTING 态安全跳过迭代（避免迭代中清空 bug），下一轮 IDLE 时真正清空。
 *
 * 驱动入口：`GameTestServer::tick()` 与 `IntegratedServer::tick()` 末尾调用 `GameTestTicker::instance().tick()`。
 *
 * 与基岩版差异：基岩 ticker 非单例且只管清理任务；此版对齐 Java 单例管 tick 推进 + 清理（两职责合一）。
 */
class GameTestTicker {
public:
    enum class State {
        Idle,    // 空闲，无运行中测试
        Running, // 正在 tick 推进
        Halting, // 收到 clear 请求，等待当前 tick 结束后清空
    };

    [[nodiscard]] static GameTestTicker& instance() noexcept;

    /**
     * @brief 添加运行中的测试实例。
     */
    void add(BaseGameTestInstance& instance);

    /**
     * @brief 添加清理任务。
     */
    void addClearTask(std::unique_ptr<GameTestClearTask> task);

    /**
     * @brief 请求清空所有测试（进入 HALTING，下一轮清空）。
     *
     * 对齐 Java `clear()`：若当前 IDLE 则立即清空；若 RUNNING 则设 HALTING，待 tick 结束后清空。
     */
    void clear();

    /**
     * @brief 强制停止（立即清空，不等待）。
     */
    void forceStop();

    /**
     * @brief 推进一 tick。
     */
    void tick();

    /**
     * @brief 当前正在 tick 的实例（非 tick 期间为 nullptr）。
     *
     * 崩溃诊断用：崩溃/异常捕获时可查询正在执行的测试实例。
     */
    [[nodiscard]] const BaseGameTestInstance* currentTicking() const noexcept { return m_currentTicking; }

    [[nodiscard]] State state() const noexcept { return m_state; }
    [[nodiscard]] bool isEmpty() const noexcept { return m_instances.empty() && m_clearTasks.empty(); }
    [[nodiscard]] std::size_t instanceCount() const noexcept { return m_instances.size(); }

private:
    GameTestTicker() = default;

    State m_state = State::Idle;
    std::vector<BaseGameTestInstance*> m_instances; // 非拥有，实例由 batch runner 拥有
    std::vector<std::unique_ptr<GameTestClearTask>> m_clearTasks;
    BaseGameTestInstance* m_currentTicking = nullptr; // 崩溃诊断用：正在 tick 的实例
};

} // namespace mc::test
