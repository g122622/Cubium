#include "common/test/framework/environment/SetGameRulesEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult SetGameRulesEnvironment::setup(BaseGameTestInstance& instance)
{
    // TODO: 1C 阶段 MinecraftEnvironmentApplier 接管——经 instance 取 ServerWorld，遍历 m_rules
    // 调 GameRules::setRule(name, value)。framework 层引擎无关，无法直接操作 ServerWorld。
    MC_UNUSED(instance);
    return mc::test::fail(
        GameTestErrorType::MethodNotImplemented, "SetGameRulesEnvironment.setup requires MinecraftEnvironmentApplier");
}

} // namespace mc::test
