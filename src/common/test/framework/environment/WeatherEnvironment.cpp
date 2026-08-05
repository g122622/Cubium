#include "common/test/framework/environment/WeatherEnvironment.hpp"

#include "common/util/assert/AssertMacros.hpp" // MC_UNUSED

namespace mc::test {

GameTestResult WeatherEnvironment::setup(BaseGameTestInstance& instance)
{
    // TODO: 1C 阶段 MinecraftEnvironmentApplier 接管——经 instance 取 ServerWorld，按 m_type 调
    // setWeatherParameters(Clear: clearTime=100000/raining=false/thundering=false; Rain/Thunder: rainTime=100000)。
    MC_UNUSED(instance);
    return mc::test::fail(
        GameTestErrorType::MethodNotImplemented, "WeatherEnvironment.setup requires MinecraftEnvironmentApplier");
}

GameTestResult WeatherEnvironment::teardown(BaseGameTestInstance& instance)
{
    // TODO: 1C 阶段 MinecraftEnvironmentApplier 接管——调 level.resetWeatherCycle()。
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
