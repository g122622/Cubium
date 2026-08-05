#pragma once

#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief 把 framework 层环境定义的"意图"应用到 `ServerWorld`。
 *
 * framework 层环境（`SetGameRulesEnvironment`/`TimeOfDayEnvironment`/`WeatherEnvironment`）引擎无关，
 * 其 `setup()` 返回 `MethodNotImplemented`。本 applier 经类型分派直接取各环境的意图字段，调
 * `ServerWorld`/`TimeManager`/`WeatherManager`/`GameRules` 应用，绕过 framework 层 stub。
 *
 * 调用时机：对齐 Java `GameTestRunner`——批次开始时 setup 一次，批次结束（下一批次 setup 前）teardown 一次。
 * 由 `MinecraftGameTestBatchRunner`（或 `GameTestRunner` 1D）在批次边界调用。
 *
 * 不对外——由 runner 内部使用。
 */
class MinecraftEnvironmentApplier {
public:
    /**
     * @brief 应用环境 setup 到世界。
     *
     * @param env 环境定义（`AllOfEnvironment` 递归展开，其余按类型分派）。
     * @param world 目标世界。
     * @return nullopt=成功；非 nullopt=应用失败（携带错误）。
     */
    [[nodiscard]] static GameTestResult applySetup(
        const TestEnvironmentDefinition& env, mc::server::ServerWorld& world);

    /**
     * @brief 应用环境 teardown（还原）。默认对无 teardown 语义的环境为 no-op。
     */
    [[nodiscard]] static GameTestResult applyTeardown(
        const TestEnvironmentDefinition& env, mc::server::ServerWorld& world);
};

} // namespace mc::test
