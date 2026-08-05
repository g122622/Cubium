#include "common/test/framework/environment/WeatherEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult WeatherEnvironment::setup(BaseGameTestInstance& instance)
{
    // framework 层引擎无关，无法直接操作 ServerWorld。环境应用由 minecraft 绑定层
    // MinecraftEnvironmentApplier::applySetup 经 dynamic_cast<WeatherEnvironment> 接管，
    // 调 WeatherManager::setClear/setRain/setThunder。此 setup() 不再被 batch runner 调用
    // （MinecraftGameTestBatchRunner._applyBatchEnvironmentSetup 直转 applier），保留为接口占位。
    MC_UNUSED(instance);
    return mc::test::pass();
}

GameTestResult WeatherEnvironment::teardown(BaseGameTestInstance& instance)
{
    // 同 setup：由 MinecraftEnvironmentApplier::applyTeardown 调 WeatherManager::resetWeather 接管。
    MC_UNUSED(instance);
    return mc::test::pass();
}

std::string_view WeatherEnvironment::typeName(Type type) noexcept
{
    switch (type) {
        case Type::Clear:
            return "clear";
        case Type::Rain:
            return "rain";
        case Type::Thunder:
            return "thunder";
    }
    return "clear";
}

} // namespace mc::test
