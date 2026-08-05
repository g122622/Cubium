#pragma once

#include "common/test/base/error/GameTestResult.hpp"
#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

class BaseGameTestInstance;

/**
 * @brief 测试环境定义接口（setup/teardown）。
 *
 * 对齐 Java 1.21.11 `TestEnvironmentDefinition`（interface）：在批次开始、测试运行前调一次 `setup`，
 * 批次结束（下一批次 setup 前）调一次 `teardown`，作用域是整个批次而非单个测试（见
 * `GameTestRunner.endCurrentEnvironment`）。
 *
 * Java 侧 setup/teardown 收 `ServerLevel`；此处 framework 层引擎无关，收 `BaseGameTestInstance&`
 *（实例可经 `MinecraftEnvironmentApplier` 1C 阶段转取 `ServerWorld&`）。`GameTestResult` 返回值是
 * C++ 侧的扩展：Java setup 抛异常即批次失败，此处改为错误即值（项目"no exceptions"规范），
 * 返回非 nullopt 即批次失败并提前终止。
 *
 * 子类（对齐 Java record）：`AllOfEnvironment`/`SetGameRulesEnvironment`/`TimeOfDayEnvironment`/
 * `WeatherEnvironment`/`FunctionsEnvironment`。codec 键名（对齐 Java bootstrap）：all_of/game_rules/
 * time_of_day/weather/function。
 */
class TestEnvironmentDefinition {
public:
    virtual ~TestEnvironmentDefinition() = default;

    /**
     * @brief 批次开始时应用环境（设置游戏规则/时间/天气/函数等）。
     *
     * @param instance 所属批次任一实例（用于取世界引用；同批次所有实例共享同一环境）。
     * @return nullopt=成功；非 nullopt=批次失败（携带错误）。
     */
    virtual GameTestResult setup(BaseGameTestInstance& instance) = 0;

    /**
     * @brief 批次结束时还原环境。默认空实现（对齐 Java default teardown）。
     */
    virtual GameTestResult teardown(BaseGameTestInstance& instance)
    {
        MC_UNUSED(instance);
        return mc::test::pass();
    }
};

} // namespace mc::test
