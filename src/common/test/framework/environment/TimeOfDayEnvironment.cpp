#include "common/test/framework/environment/TimeOfDayEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult TimeOfDayEnvironment::setup(BaseGameTestInstance& instance)
{
    // framework 层引擎无关，无法操作 ServerWorld。由 minecraft 绑定层
    // MinecraftEnvironmentApplier::applySetup 经 dynamic_cast<TimeOfDayEnvironment> 接管，
    // 调 TimeManager::setDayTime(m_time)。此 setup() 不再被 batch runner 调用，保留为接口占位。
    MC_UNUSED(instance);
    return mc::test::pass();
}

} // namespace mc::test
