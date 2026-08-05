#include "common/test/framework/batch/BaseGameTestBatchRunner.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/instance/GameTestState.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "common/util/Direction.hpp" // Rotation / Rotations::add

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

void BaseGameTestBatchRunner::_trackInstance(std::unique_ptr<BaseGameTestInstance> instance)
{
    m_ticker.add(*instance);
    m_currentBatchInstances.push_back(std::move(instance));
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
