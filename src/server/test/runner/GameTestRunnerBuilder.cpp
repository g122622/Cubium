#include "server/test/runner/GameTestRunnerBuilder.hpp"

#include "common/util/assert/AssertMacros.hpp"

namespace mc::test {

std::unique_ptr<GameTestRunner> GameTestRunnerBuilder::build()
{
    MC_ASSERT_RELEASE_MSG(m_world != nullptr, "GameTestRunnerBuilder: world is not set");
    MC_ASSERT_RELEASE_MSG(m_ticker != nullptr, "GameTestRunnerBuilder: ticker is not set");
    // GameTestRunner 构造函数私有，仅经 friend GameTestRunnerBuilder 可访问。
    // 不能用 std::make_unique（其内部模板非 friend）；在此直接构造（friend 上下文）再 move 入 unique_ptr。
    return std::unique_ptr<GameTestRunner>(
        new GameTestRunner(*m_world, *m_ticker, std::move(m_batches), m_gridStart, m_testsPerRow));
}

} // namespace mc::test
