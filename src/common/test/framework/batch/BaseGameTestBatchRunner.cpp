#include "common/test/framework/batch/BaseGameTestBatchRunner.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/instance/GameTestState.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/util/Direction.hpp" // Rotation / Rotations::add

#include <spdlog/spdlog.h>

namespace mc::test {

BaseGameTestBatchRunner::BaseGameTestBatchRunner(
    std::vector<GameTestBatch> batches, GameTestTicker& ticker, TestParameters params)
    : m_batches(std::move(batches))
    , m_ticker(ticker)
    , m_params(std::move(params))
{
    for (const auto& batch : m_batches) {
        m_totalTestCount += batch.testFunctions().size();
    }
}

void BaseGameTestBatchRunner::addBatchListener(std::shared_ptr<GameTestBatchListener> listener)
{
    m_batchListeners.push_back(std::move(listener));
}

void BaseGameTestBatchRunner::setInstanceListener(std::shared_ptr<IGameTestListener> listener)
{
    m_instanceListener = std::move(listener);
}

void BaseGameTestBatchRunner::_trackInstance(std::unique_ptr<BaseGameTestInstance> instance)
{
    // 挂载实例级监听器（_RunnerListener）——实例 succeed/fail 时更新 tracker + 广播 GlobalTestReporter。
    if (m_instanceListener) {
        instance->addListener(m_instanceListener);
    }
    m_ticker.add(*instance);
    m_currentBatchInstances.push_back(std::move(instance));
}

void BaseGameTestBatchRunner::_failBatchEnvironment(GameTestBatch& batch, const GameTestError& error)
{
    MC_UNUSED(error);
    // 环境 setup 失败：本批所有测试函数计为 failed（required 计入 failedRequiredCount），
    // 不创建任何实例。直接推进下一批（m_currentBatchInstances 为空，tick() 会跳过本批推进，
    // 故此处显式链式调用 _runBatch 下一批，保证 isComplete() 最终收敛）。
    m_failedCount += batch.testFunctions().size();
    for (const auto& fn : batch.testFunctions()) {
        if (fn && fn->data().required()) {
            ++m_failedRequiredCount;
        }
    }
    if (batch.afterBatch()) {
        batch.afterBatch()();
    }
    for (auto& l : m_batchListeners) {
        l->onBatchFinished(batch);
    }
    // 推进下一批
    if (m_currentBatch + 1 < m_batches.size()) {
        _runBatch(m_currentBatch + 1);
    }
}

void BaseGameTestBatchRunner::start()
{
    if (m_started || m_batches.empty()) {
        return;
    }
    m_started = true;
    _runBatch(0);
}

void BaseGameTestBatchRunner::_runBatch(std::size_t batchIndex)
{
    if (batchIndex >= m_batches.size()) {
        return; // 全部批次完成
    }
    m_currentBatch = batchIndex;
    auto& batch = m_batches[batchIndex];

    // beforeBatch 回调
    if (batch.beforeBatch()) {
        batch.beforeBatch()();
    }
    // 批次开始通知
    for (auto& l : m_batchListeners) {
        l->onBatchStarting(batch);
    }

    // 环境 setup（minecraft 绑定层经 MinecraftEnvironmentApplier 应用到 ServerWorld）。
    // setup 失败则整批测试计为失败，跳过实例创建（对齐 Java：环境 setup 抛异常即批次失败）。
    if (batch.environment()) {
        if (GameTestResult setupResult = _applyBatchEnvironmentSetup(batch); !isPass(setupResult)) {
            const GameTestError err = setupResult.has_value()
                ? *setupResult
                : GameTestError(GameTestErrorType::LevelStateModificationFailed, "batch environment setup failed");
            spdlog::error("[GameTest] batch '{}' environment setup failed: {}", batch.name(), err.formattedMessage());
            _failBatchEnvironment(batch, err);
            return;
        }
    }

    // 为批次内每个测试函数创建实例（应用 TestParameters.rotation 叠加）
    for (auto& function : batch.testFunctions()) {
        const Rotation combined = Rotations::add(function->data().rotation(), m_params.rotation());
        auto instance = _createGameTestInstance(*function, combined);
        _runTest(std::move(instance));
    }
}

void BaseGameTestBatchRunner::tick()
{
    if (!m_started || m_currentBatchInstances.empty()) {
        return;
    }
    // 检查当前批次所有实例是否完成
    const bool allDone = std::all_of(m_currentBatchInstances.begin(),
        m_currentBatchInstances.end(),
        [](const std::unique_ptr<BaseGameTestInstance>& inst) { return isDone(inst->state()); });
    if (!allDone) {
        return;
    }

    // 计数 + afterBatch 回调
    auto& batch = m_batches[m_currentBatch];
    for (auto& inst : m_currentBatchInstances) {
        if (hasSucceeded(inst->state())) {
            ++m_passedCount;
        } else {
            ++m_failedCount;
            if (inst->function().data().required()) {
                ++m_failedRequiredCount;
            }
        }
    }
    // 环境 teardown（在 afterBatch 前还原世界状态）。teardown 失败仅记日志，不影响计数。
    if (batch.environment()) {
        if (GameTestResult tdResult = _applyBatchEnvironmentTeardown(batch); !isPass(tdResult)) {
            const std::string msg =
                tdResult.has_value() ? tdResult->formattedMessage() : "batch environment teardown failed";
            spdlog::warn("[GameTest] batch '{}' environment teardown failed: {}", batch.name(), msg);
        }
    }
    if (batch.afterBatch()) {
        batch.afterBatch()();
    }
    for (auto& l : m_batchListeners) {
        l->onBatchFinished(batch);
    }

    m_currentBatchInstances.clear();

    // 推进下一批
    if (m_currentBatch + 1 < m_batches.size()) {
        _runBatch(m_currentBatch + 1);
    }
}

bool BaseGameTestBatchRunner::isComplete() const noexcept
{
    return m_started && m_currentBatchInstances.empty() && (m_currentBatch + 1 >= m_batches.size());
}

} // namespace mc::test
