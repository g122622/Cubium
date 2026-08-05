#pragma once

#include "common/test/framework/listener/IGameTestListener.hpp"

#include <memory>

namespace mc::test {

/**
 * @brief 游戏内可视化监听器：测试通过/失败时经 spdlog 输出 +（TODO）游戏内信标光束/聊天广播。
 *
 * 对齐基岩版 `ReportGameListener` + Java `ReportGameListener` 的游戏内可视化部分：测试结果在世界上以
 * 信标光束颜色标记（绿=通过，红=失败，橙=重试，灰=进行中），并向聊天广播。
 *
 * 第一阶段（phase-1 临时）：项目方块实体可视化体系（`StructureBlockEntity` 等）未就绪，仅 spdlog 输出。
 * 信标光束/聊天广播留 TODO，待方块实体体系就绪后接入（见 `TestInstanceBlockEntity`）。
 *
 * 不对外——由 `MinecraftGameTestBatchRunner` 在 `_runTest` 时挂到实例。
 */
class WorldVisualizationListener final : public IGameTestListener {
public:
    WorldVisualizationListener() = default;

    void onTestStructureLoaded(BaseGameTestInstance& test) override;
    void onTestStarted(BaseGameTestInstance& test) override;
    void onTestPassed(BaseGameTestInstance& test) override;
    void onTestFailed(BaseGameTestInstance& test) override;
    void onTestRetryStarted(BaseGameTestInstance& test) override;
    void onTestRetryFinished(BaseGameTestInstance& test) override;
};

} // namespace mc::test
