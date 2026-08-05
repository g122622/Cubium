#include "server/test/minecraft/listener/WorldVisualizationListener.hpp"

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/instance/GameTestState.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

const char* _testLabel(const BaseGameTestInstance& test)
{
    return test.function().testName().c_str();
}

} // namespace

void WorldVisualizationListener::onTestStructureLoaded(BaseGameTestInstance& test)
{
    // TODO: 游戏内信标光束标记"进行中"（灰色）+ 聊天广播结构已加载
    spdlog::info("[GameTest] structure loaded: {}", _testLabel(test));
}

void WorldVisualizationListener::onTestStarted(BaseGameTestInstance& test)
{
    spdlog::info("[GameTest] started: {}", _testLabel(test));
}

void WorldVisualizationListener::onTestPassed(BaseGameTestInstance& test)
{
    // TODO: 游戏内信标光束标记"通过"（绿色）+ 聊天广播
    spdlog::info("[GameTest] PASSED: {}", _testLabel(test));
}

void WorldVisualizationListener::onTestFailed(BaseGameTestInstance& test)
{
    // TODO: 游戏内信标光束标记"失败"（红色）+ 聊天广播错误位置
    const auto& err = test.error();
    if (err.has_value()) {
        spdlog::error("[GameTest] FAILED: {} - {}", _testLabel(test), err->formattedMessage());
    } else {
        spdlog::error("[GameTest] FAILED: {}", _testLabel(test));
    }
}

void WorldVisualizationListener::onTestRetryStarted(BaseGameTestInstance& test)
{
    // TODO: 游戏内信标光束标记"重试中"（橙色）
    spdlog::warn("[GameTest] retry started: {}", _testLabel(test));
}

void WorldVisualizationListener::onTestRetryFinished(BaseGameTestInstance& test)
{
    spdlog::info(
        "[GameTest] retry finished: {} ({})", _testLabel(test), hasSucceeded(test.state()) ? "passed" : "failed");
}

} // namespace mc::test
