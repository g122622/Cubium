#include "common/test/framework/environment/FunctionsEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult FunctionsEnvironment::setup(BaseGameTestInstance& instance)
{
    // TODO: 项目命令函数系统（.mcfunction / ServerFunctionManager 等价物）尚未就绪，本环境暂不可用。
    // 函数系统就绪后由 MinecraftEnvironmentApplier 接管——按 m_setupFunction 取函数并以 GAMEMASTER 权限执行。
    MC_UNUSED(instance);
    return mc::test::fail(
        GameTestErrorType::MethodNotImplemented, "FunctionsEnvironment.setup requires .mcfunction system (not ready)");
}

GameTestResult FunctionsEnvironment::teardown(BaseGameTestInstance& instance)
{
    // TODO: 同 setup，待 .mcfunction 系统就绪。
    MC_UNUSED(instance);
    return mc::test::fail(GameTestErrorType::MethodNotImplemented,
        "FunctionsEnvironment.teardown requires .mcfunction system (not ready)");
}

} // namespace mc::test
