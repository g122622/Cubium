#pragma once

#include "common/test/framework/batch/GameTestBatch.hpp"
#include "common/test/framework/function/BaseGameTestFunction.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "server/test/runner/GameTestRunner.hpp"
#include "common/world/block/BlockPos.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief `GameTestRunner` 构造器（builder 模式）。
 *
 * 对齐 Java `GameTestRunner.Builder`：链式设置 world/ticker/batches/gridStart/testsPerRow 后 `build()`。
 * 与 `NativeTestRegistrationBuilder`（注册期）/`ScriptRegistrationBuilder`（脚本注册期）前缀区分——本 builder 是运行期。
 */
class GameTestRunnerBuilder {
public:
    GameTestRunnerBuilder& world(mc::server::ServerWorld& w) noexcept
    {
        m_world = &w;
        return *this;
    }
    GameTestRunnerBuilder& ticker(GameTestTicker& t) noexcept
    {
        m_ticker = &t;
        return *this;
    }
    GameTestRunnerBuilder& batches(std::vector<GameTestBatch> b)
    {
        m_batches = std::move(b);
        return *this;
    }
    GameTestRunnerBuilder& gridStart(BlockPos p) noexcept
    {
        m_gridStart = p;
        return *this;
    }
    GameTestRunnerBuilder& testsPerRow(std::size_t n) noexcept
    {
        m_testsPerRow = n;
        return *this;
    }

    [[nodiscard]] std::unique_ptr<GameTestRunner> build();

private:
    mc::server::ServerWorld* m_world = nullptr;
    GameTestTicker* m_ticker = nullptr;
    std::vector<GameTestBatch> m_batches;
    BlockPos m_gridStart{0, 0, 0};
    std::size_t m_testsPerRow = 8;
};

} // namespace mc::test
