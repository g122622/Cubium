#include "common/test/framework/environment/TimeOfDayEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult TimeOfDayEnvironment::setup(BaseGameTestInstance& instance)
{
    // TODO: 1C 阶段 MinecraftEnvironmentApplier 接管——经 instance 取 ServerWorld，调 setDayTime(m_time)。
    MC_UNUSED(instance);
    return mc::test::fail(
        GameTestErrorType::MethodNotImplemented, "TimeOfDayEnvironment.setup requires MinecraftEnvironmentApplier");
}

} // namespace mc::test
