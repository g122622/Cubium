#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include <optional>
#include <string>

namespace mc::test {

/**
 * @brief 环境定义：setup/teardown 时执行 .mcfunction 命令函数。
 *
 * 对齐 Java `TestEnvironmentDefinition.Functions`（record，持 `Optional<Identifier> setupFunction`/
 * `teardownFunction`）：setup/teardown 经 `ServerFunctionManager` 取函数并以 GAMEMASTER 权限执行。
 *
 * TODO: 项目命令函数系统（.mcfunction 加载与 `ServerFunctionManager` 等价物）尚未就绪，本类 setup/teardown
 * 返回 `MethodNotImplemented` 错误。函数系统就绪后由 `MinecraftEnvironmentApplier` 接管执行。
 */
class FunctionsEnvironment final : public TestEnvironmentDefinition {
public:
    FunctionsEnvironment() = default;
    FunctionsEnvironment(std::optional<std::string> setupFunction, std::optional<std::string> teardownFunction)
        : m_setupFunction(std::move(setupFunction))
        , m_teardownFunction(std::move(teardownFunction))
    {}

    GameTestResult setup(BaseGameTestInstance& instance) override;
    GameTestResult teardown(BaseGameTestInstance& instance) override;

    [[nodiscard]] const std::optional<std::string>& setupFunction() const noexcept { return m_setupFunction; }
    [[nodiscard]] const std::optional<std::string>& teardownFunction() const noexcept { return m_teardownFunction; }

private:
    std::optional<std::string> m_setupFunction;
    std::optional<std::string> m_teardownFunction;
};

} // namespace mc::test
