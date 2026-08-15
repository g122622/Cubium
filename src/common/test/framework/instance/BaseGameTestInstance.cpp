#include "common/test/framework/instance/BaseGameTestInstance.hpp"

#include "common/test/framework/function/IGameTestFunctionContext.hpp"
#include "common/test/framework/function/IGameTestRunResult.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "common/test/framework/listener/IGameTestListener.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::test {

BaseGameTestInstance::BaseGameTestInstance(
    const BaseGameTestFunction& function, std::unique_ptr<IGameTestHelperProvider> helperProvider)
    : m_function(function)
    , m_helperProvider(std::move(helperProvider))
{
    MC_ASSERT_RELEASE_MSG(m_helperProvider != nullptr, "helper provider must not be null");
}

BaseGameTestInstance::~BaseGameTestInstance() = default; // 在此 IGameTestHelper 已完整，可默认生成删除器

void BaseGameTestInstance::startExecution()
{
    // 对齐 Java startExecution(-(setupTicks+1))：setup 阶段用负 tickCount，tickCount>=0 才正式运行
    m_tickCount = -m_function.data().setupTicks() - 1;
    m_state = GameTestState::NotStarted;
}

void BaseGameTestInstance::tick()
{
    if (isDone(m_state)) {
        return;
    }
    // 结构未就绪则等待（子类 _isTestReady 判定）
    if (!_isTestReady()) {
        return;
    }
    ++m_tickCount;

    // setup 阶段结束（tickCount 首次 >= 0）触发测试函数
    if (m_tickCount == 0 && !m_testFunctionStarted) {
        _runTestFunction();
        m_testFunctionStarted = true;
        // 仅当测试函数未在同步路径内终结时才进入 Running。同步测试（如骨粉催熟在函数体末尾直接
        // test.succeed()）会在 _runTestFunction 内经 succeed() 把 state 置为 Succeeded 并已 _notifyPassed；
        // 此处无条件覆盖回 Running 会让该实例继续 tick 至 maxTicks 超时再发一次 FAILED，造成同一测试
        // PASSED + FAILED 双记录（total=1 但 passed+failed=2）。已终结（Succeeded/Failed）时保持终态，
        // 由 isDone(m_state) 守卫让后续 tick 直接 return。
        if (!isDone(m_state)) {
            m_state = GameTestState::Running;
        }
        _notifyStarted();
    }

    if (isDone(m_state)) {
        return;
    }

    // 执行 runAtTickTime 到期回调（tickCount 匹配的）
    for (auto& [tick, fn] : m_runAtTickTime) {
        if (tick == m_tickCount && fn) {
            const GameTestResult result = fn();
            if (result.has_value()) {
                fail(result.value());
                return;
            }
        }
    }

    // 推进所有序列
    for (auto& seq : m_sequences) {
        const GameTestResult result = seq->tick(m_tickCount);
        if (result.has_value()) {
            fail(result.value());
            return;
        }
    }

    // 检查 failIf 条件（任一命中即失败）
    for (auto& fn : m_failConditions) {
        if (fn) {
            const GameTestResult result = fn();
            if (result.has_value()) {
                fail(result.value());
                return;
            }
        }
    }

    // 检查 succeedIf/succeedWhen 条件（全部通过即成功）
    if (!m_succeedConditions.empty()) {
        bool allPass = true;
        for (auto& fn : m_succeedConditions) {
            if (fn) {
                const GameTestResult result = fn();
                if (result.has_value()) {
                    allPass = false;
                    break;
                }
            }
        }
        if (allPass) {
            succeed();
            return;
        }
    }

    // 超时检测
    if (m_tickCount > m_function.data().maxTicks()) {
        fail(GameTestError{GameTestErrorType::ExecutionTimeout,
            "Test timed out after {0} ticks (test={1})",
            {std::to_string(m_function.data().maxTicks()), m_function.testName()}});
        return;
    }

    // 轮询异步测试函数的 Promise 状态：rejected 立即 fail，fulfilled 不动作（由 succeed/超时接管）。
    // 同步测试的 runResult isComplete() 恒 true 且 getError()=pass，此处不动作。
    if (m_runResult && m_runResult->isComplete()) {
        const GameTestResult result = m_runResult->getError();
        m_runResult.reset(); // 取走结果后释放句柄（Promise 句柄在 AsyncRunResult 析构时 releaseValue）
        if (result.has_value()) {
            fail(result.value());
            return;
        }
    }

    // 全部序列完成且无 succeedIf 阻塞 → 隐式成功（对齐基岩：序列走完即通过）
    bool allSequencesComplete = true;
    for (auto& seq : m_sequences) {
        if (!seq->isComplete()) {
            allSequencesComplete = false;
            break;
        }
    }
    if (allSequencesComplete && !m_sequences.empty() && m_succeedConditions.empty()) {
        succeed();
    }
}

void BaseGameTestInstance::succeed()
{
    if (isDone(m_state)) {
        return;
    }
    m_state = GameTestState::Succeeded;
    m_runResult.reset(); // 测试结束：释放异步 runResult 持有的 Promise 句柄
    // 执行 runOnFinish 回调
    for (auto& fn : m_onFinish) {
        if (fn) {
            fn();
        }
    }
    _notifyPassed();
}

void BaseGameTestInstance::fail(GameTestError error)
{
    if (isDone(m_state)) {
        return;
    }
    m_error = std::move(error);
    m_state = GameTestState::Failed;
    m_runResult.reset(); // 测试结束：释放异步 runResult 持有的 Promise 句柄
    for (auto& fn : m_onFinish) {
        if (fn) {
            fn();
        }
    }
    _notifyFailed();
}

GameTestSequence& BaseGameTestInstance::createSequence()
{
    auto seq = std::make_unique<GameTestSequence>(*m_helper);
    m_sequences.push_back(std::move(seq));
    return *m_sequences.back();
}

void BaseGameTestInstance::addListener(std::shared_ptr<IGameTestListener> listener)
{
    m_listeners.push_back(std::move(listener));
}

void BaseGameTestInstance::removeListener(const std::shared_ptr<IGameTestListener>& listener)
{
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), listener), m_listeners.end());
}

void BaseGameTestInstance::registerRunAtTickTime(i32 tick, std::function<GameTestResult()> fn)
{
    m_runAtTickTime.emplace_back(tick, std::move(fn));
}

void BaseGameTestInstance::registerSucceedCondition(std::function<GameTestResult()> fn)
{
    m_succeedConditions.push_back(std::move(fn));
}

void BaseGameTestInstance::registerFailCondition(std::function<GameTestResult()> fn)
{
    m_failConditions.push_back(std::move(fn));
}

void BaseGameTestInstance::registerOnFinish(std::function<GameTestResult()> fn)
{
    m_onFinish.push_back(std::move(fn));
}

void BaseGameTestInstance::_runTestFunction()
{
    // 创建 helper（若未创建）+ 上下文 + 执行 run
    if (!m_helper) {
        m_helper = m_helperProvider->createGameTestHelper(*this);
        MC_ASSERT_RELEASE_MSG(m_helper != nullptr, "helper creation failed");
    }
    auto context = m_function.createContext(*m_helper);
    // 保存 runResult 供 tick 轮询：同步测试 isComplete() 恒 true（立即处理）；
    // 异步测试（ScriptAsyncGameTestRunResult）isComplete() 待 Promise settle 后 true。
    m_runResult = m_function.run(*m_helper, *context);
    if (m_runResult && m_runResult->isComplete()) {
        const GameTestResult result = m_runResult->getError();
        if (result.has_value()) {
            // 测试函数立即返回失败（同步抛错 / Promise 同步 rejected）
            fail(result.value());
            return;
        }
        // 同步立即通过：runResult 留存至 succeed/fail 时 reset（fulfilled 的异步测试同此处理）。
    }
}

void BaseGameTestInstance::notifyStructureLoaded()
{
    for (auto& l : m_listeners) {
        l->onTestStructureLoaded(*this);
    }
}

void BaseGameTestInstance::_notifyStarted()
{
    for (auto& l : m_listeners) {
        l->onTestStarted(*this);
    }
}

void BaseGameTestInstance::_notifyPassed()
{
    for (auto& l : m_listeners) {
        l->onTestPassed(*this);
    }
}

void BaseGameTestInstance::_notifyFailed()
{
    for (auto& l : m_listeners) {
        l->onTestFailed(*this);
    }
}

} // namespace mc::test
