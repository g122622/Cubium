#pragma once

#include "common/core/Types.hpp"
#include "common/test/base/data/RetryOptions.hpp"
#include "common/test/base/data/TestData.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/helper/IGameTestHelperProvider.hpp"
#include "common/test/framework/instance/GameTestState.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace mc::test {

class IGameTestListener;

/**
 * @brief 测试实例抽象状态机。
 *
 * 对齐基岩版 `BaseGameTestInstance` + Java `GameTestInfo`：持有一个测试函数（`BaseGameTestFunction`）
 * 的运行期状态——tick 计数、序列列表、runAtTickTime 回调、监听器、状态、错误、重试选项。
 *
 * 生命周期：
 * 1. 构造（持 function 引用 + helper provider）。
 * 2. `spawnStructure()`（纯虚，子类放置结构）→ `onTestStructureLoaded` 通知监听器。
 * 3. `startExecution()`：设 tickCount = -(setupTicks+1)（setup 阶段负值，对齐 Java）。
 * 4. `tick()` 每 tick 调用：tickCount++；setup 阶段结束（tickCount>=0）触发 `runTestFunction`；
 *    执行 runAtTickTime 到期回调；推进所有序列（任一失败即 fail）；检查 succeedIf/failIf；
 *    超时（tickCount>maxTicks）fail；全序列完成 + succeedIf 通过即 succeed。
 * 5. `succeed()`/`fail(error)`：设终态，通知监听器。
 *
 * 纯虚方法（由 `MinecraftGameTestInstance` 1C 阶段实现）：
 * - `hasStructureBlock`/`clearStructure`/`spawnStructure`/`getStructureBounds`：结构与世界的具体交互。
 * - `_getLevelTick`：取当前世界 tick（驱动 runAtTickTime 时序）。
 * - `_isTestReady`：结构是否已就绪可开始 tick。
 */
class BaseGameTestInstance {
public:
    BaseGameTestInstance(const BaseGameTestFunction& function, std::unique_ptr<IGameTestHelperProvider> helperProvider);
    // 析构在 .cpp 定义：成员 unique_ptr<IGameTestHelper> 的删除器需 IGameTestHelper 完整类型，
    // 此头仅前向声明 IGameTestHelper（避免与 helper/ 互引成环），故不能 = default 内联。
    virtual ~BaseGameTestInstance();

    // === 公有 API ===
    void startExecution();
    void tick();
    void succeed();
    void fail(GameTestError error);
    [[nodiscard]] GameTestSequence& createSequence();
    void addListener(std::shared_ptr<IGameTestListener> listener);
    void removeListener(const std::shared_ptr<IGameTestListener>& listener);

    /**
     * @brief 若结构尚未放置则放置（runner/command 在 tick 前调一次）。
     *
     * `spawnStructure()`/`hasStructureBlock()` 为 protected 子类钩子（供实例自身生命周期内部判定），
     * 但批次 runner 与 `/gametest` 命令需在实例加入 ticker 前**显式触发**一次结构放置（tick() 内
     * `_isTestReady()` 依赖结构已就绪）。此公有包装转发到子类实现，保持封装。
     */
    void spawnStructureIfNeeded()
    {
        if (!hasStructureBlock()) {
            spawnStructure();
        }
    }

    [[nodiscard]] const BaseGameTestFunction& function() const noexcept { return m_function; }
    [[nodiscard]] GameTestState state() const noexcept { return m_state; }
    [[nodiscard]] i32 tickCount() const noexcept { return m_tickCount; }
    [[nodiscard]] const std::optional<GameTestError>& error() const noexcept { return m_error; }
    [[nodiscard]] IGameTestHelper& helper() noexcept { return *m_helper; }
    [[nodiscard]] const RetryOptions& retryOptions() const noexcept { return m_retryOptions; }
    void setRetryOptions(RetryOptions options) noexcept { m_retryOptions = options; }

    // === 注册回调（供 IGameTestHelper 实现调用，转发了 helper 的 runAtTickTime/succeedWhen/failIf 等）===
    void registerRunAtTickTime(i32 tick, std::function<GameTestResult()> fn);
    void registerSucceedCondition(std::function<GameTestResult()> fn);
    void registerFailCondition(std::function<GameTestResult()> fn);
    void registerOnFinish(std::function<GameTestResult()> fn);

protected:
    // === 子类实现 ===
    [[nodiscard]] virtual bool hasStructureBlock() const = 0;
    virtual void clearStructure() = 0;
    virtual void spawnStructure() = 0;
    [[nodiscard]] virtual i32 _getLevelTick() const = 0;
    [[nodiscard]] virtual bool _isTestReady() = 0;

    // 通知监听器（子类也可调用）
    void notifyStructureLoaded();
    void _notifyStarted();
    void _notifyPassed();
    void _notifyFailed();

    IGameTestHelper& _helperRef() noexcept { return *m_helper; }

private:
    void _runTestFunction();

    const BaseGameTestFunction& m_function;
    std::unique_ptr<IGameTestHelperProvider> m_helperProvider;
    std::unique_ptr<IGameTestHelper> m_helper;
    std::vector<std::shared_ptr<IGameTestListener>> m_listeners;
    std::vector<std::unique_ptr<GameTestSequence>> m_sequences;
    std::vector<std::pair<i32, std::function<GameTestResult()>>> m_runAtTickTime; // tick → 回调
    std::vector<std::function<GameTestResult()>> m_succeedConditions;             // succeedWhen/succeedIf
    std::vector<std::function<GameTestResult()>> m_failConditions;                // failIf
    std::vector<std::function<GameTestResult()>> m_onFinish;                      // runOnFinish
    GameTestState m_state = GameTestState::NotStarted;
    i32 m_tickCount = 0;
    std::optional<GameTestError> m_error;
    RetryOptions m_retryOptions = RetryOptions::noRetries();
    bool m_testFunctionStarted = false;
};

} // namespace mc::test
