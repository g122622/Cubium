#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include "common/core/Types.hpp"

namespace mc::test {

/**
 * @brief 环境定义：设置世界时间。
 *
 * 对齐 Java `TestEnvironmentDefinition.TimeOfDay`（record，持 `int time`）：setup 调
 * `level.setDayTime(time)`，无 teardown。Java 用 `NON_NEGATIVE_INT` 约束。
 *
 * 此处 framework 层仅持意图；实际应用到 `ServerWorld` 由 1C 阶段 `MinecraftEnvironmentApplier` 完成。
 */
class TimeOfDayEnvironment final : public TestEnvironmentDefinition {
public:
    TimeOfDayEnvironment() = default;
    explicit TimeOfDayEnvironment(i32 time) noexcept
        : m_time(time)
    {}

    GameTestResult setup(BaseGameTestInstance& instance) override;

    [[nodiscard]] i32 time() const noexcept { return m_time; }

private:
    i32 m_time = 0;
};

} // namespace mc::test
