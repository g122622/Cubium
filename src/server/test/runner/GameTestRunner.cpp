#include "server/test/runner/GameTestRunner.hpp"

#include "common/test/framework/batch/BaseGameTestBatchRunner.hpp"
#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/instance/GameTestState.hpp"
#include "common/test/framework/listener/IGameTestListener.hpp"
#include "server/test/minecraft/batch/MinecraftGameTestBatchRunner.hpp"
#include "server/test/runner/GameTestRunnerBuilder.hpp" // builder() 按值返回，需完整类型
#include "server/test/runner/reporter/GlobalTestReporter.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc::test {

namespace {

/**
 * @brief runner 内部监听器：实例状态变化时更新 tracker + 广播到 GlobalTestReporter。
 *
 * 挂到每个 `BaseGameTestInstance`（由 `MinecraftGameTestBatchRunner._runTest` 在创建实例后挂载——
 * 当前 1C runner 未挂实例监听器，TODO 在 1D/1F 接线时由 runner 经 batch runner 注入）。
 */
class _RunnerListener final : public IGameTestListener {
public:
    explicit _RunnerListener(MultipleTestTracker& tracker)
        : m_tracker(tracker)
    {}

    void onTestStructureLoaded(BaseGameTestInstance& test) override { MC_UNUSED(test); }
    void onTestStarted(BaseGameTestInstance& test) override { MC_UNUSED(test); }

    void onTestPassed(BaseGameTestInstance& test) override
    {
        m_tracker.onPassed();
        GlobalTestReporter::instance().onTestPassed(test);
    }

    void onTestFailed(BaseGameTestInstance& test) override
    {
        m_tracker.onFailed();
        GlobalTestReporter::instance().onTestFailed(test);
    }

    void onTestRetryStarted(BaseGameTestInstance& test) override { MC_UNUSED(test); }
    void onTestRetryFinished(BaseGameTestInstance& test) override { MC_UNUSED(test); }

private:
    MultipleTestTracker& m_tracker;
};

} // namespace

GameTestRunner::GameTestRunner(mc::server::ServerWorld& world,
    GameTestTicker& ticker,
    std::vector<GameTestBatch> batches,
    BlockPos gridStart,
    std::size_t testsPerRow)
    : m_world(world)
    , m_ticker(ticker)
{
    // 统计总测试数
    std::size_t total = 0;
    for (const auto& batch : batches) {
        total += batch.testFunctions().size();
    }
    m_tracker.setTotal(total);

    // 构造运行期参数：testsPerRow 由 builder 传入（默认 8）；其余用默认值（GameTestServer 门面 1F 细化）
    TestParameters params;
    params.setTestsPerRow(static_cast<i32>(testsPerRow));
    params.setTestPos(gridStart);

    m_batchRunner = std::make_unique<MinecraftGameTestBatchRunner>(
        std::move(batches), m_ticker, std::move(params), m_world, gridStart);
    // TODO: 把 _RunnerListener 挂到每个实例——需 batch runner 在 _runTest 内暴露实例创建钩子，
    // 或经 GameTestBatchListener 在批次开始时遍历挂载。1D/1F 接线时补。
}

GameTestRunner::~GameTestRunner() = default;

GameTestRunnerBuilder GameTestRunner::builder()
{
    return GameTestRunnerBuilder{};
}

void GameTestRunner::start()
{
    MC_ASSERT_RELEASE_MSG(m_batchRunner != nullptr, "GameTestRunner: batch runner is null");
    m_batchRunner->start();
}

void GameTestRunner::tick()
{
    if (m_batchRunner == nullptr) {
        return;
    }
    m_batchRunner->tick();
    // ticker 由 GameTestServer/IntegratedServer 的 tick 末尾统一驱动（见 GameTestTicker 单例），
    // 此处不再重复调 ticker.tick()，避免双重推进。
}

bool GameTestRunner::isComplete() const noexcept
{
    return m_batchRunner != nullptr && m_batchRunner->isComplete();
}

std::size_t GameTestRunner::failedRequiredCount() const noexcept
{
    return m_batchRunner ? m_batchRunner->failedRequiredCount() : 0;
}

std::size_t GameTestRunner::totalTestCount() const noexcept
{
    return m_batchRunner ? m_batchRunner->totalTestCount() : 0;
}

std::size_t GameTestRunner::passedCount() const noexcept
{
    return m_batchRunner ? m_batchRunner->passedCount() : 0;
}

std::size_t GameTestRunner::failedCount() const noexcept
{
    return m_batchRunner ? m_batchRunner->failedCount() : 0;
}

} // namespace mc::test
