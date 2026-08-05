#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include <string_view>

namespace mc::test {

/**
 * @brief 环境定义：设置天气。
 *
 * 对齐 Java `TestEnvironmentDefinition.Weather`（record，持 `Weather.Type weather` 枚举）：
 * setup 调 `weather.apply(level)`（即 `setWeatherParameters(clearTime, rainTime, raining, thundering)`），
 * teardown 调 `level.resetWeatherCycle()`。
 *
 * 三态枚举对齐 Java `Weather.Type`：Clear（clearTime=100000, raining=false, thundering=false）、
 * Rain（rainTime=100000, raining=true, thundering=false）、Thunder（rainTime=100000, raining=true,
 * thundering=true）。
 *
 * 此处 framework 层仅持意图；实际应用到 `ServerWorld` 由 1C 阶段 `MinecraftEnvironmentApplier` 完成。
 */
class WeatherEnvironment final : public TestEnvironmentDefinition {
public:
    enum class Type : u8 { Clear, Rain, Thunder };

    WeatherEnvironment() = default;
    explicit WeatherEnvironment(Type type) noexcept
        : m_type(type)
    {}

    GameTestResult setup(BaseGameTestInstance& instance) override;
    GameTestResult teardown(BaseGameTestInstance& instance) override;

    [[nodiscard]] Type type() const noexcept { return m_type; }

    /**
     * @brief 枚举→Java codec 键名（clear/rain/thunder）。
     */
    [[nodiscard]] static std::string_view typeName(Type type) noexcept;

private:
    Type m_type = Type::Clear;
};

} // namespace mc::test
