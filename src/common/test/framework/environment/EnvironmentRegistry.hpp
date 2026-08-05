#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include <string>
#include <unordered_map>

namespace mc::test {

/**
 * @brief 环境注册表：按名（如 `"default"`）解析为 `TestEnvironmentDefinition` 实例。
 *
 * 对齐 Java `GameTestEnvironments`（`DEFAULT = "default"` → `AllOf(List.of())`）+ 注册表
 * `Registries.TEST_ENVIRONMENT`。Java 侧环境是数据驱动（`RegistryFileCodec`），按资源 key 解析；
 * 此处简化为运行期 `unordered_map<string, shared_ptr<TestEnvironmentDefinition>>`，由 `GameTestServer`/
 * `GameTestCommand` 在启动期注册内置环境（`"default"` 默认空 `AllOfEnvironment`）。
 *
 * `TestData.environment()` 持字符串键，runner 经此注册表解析为实例后交给 `MinecraftEnvironmentApplier` 应用。
 */
class EnvironmentRegistry {
public:
    [[nodiscard]] static EnvironmentRegistry& instance() noexcept;

    /**
     * @brief 注册环境实例到键名。同名不覆盖（返回 false）。
     */
    bool registerEnvironment(const std::string& name, std::shared_ptr<TestEnvironmentDefinition> environment);

    /**
     * @brief 按名查询环境实例。
     */
    [[nodiscard]] std::shared_ptr<TestEnvironmentDefinition> getEnvironment(const std::string& name) const;

    /**
     * @brief 是否存在该键名。
     */
    [[nodiscard]] bool hasEnvironment(const std::string& name) const noexcept;

    /**
     * @brief 注册内置默认环境（`"default"` → 空 `AllOfEnvironment`，对齐 Java `GameTestEnvironments.DEFAULT`）。
     *
     * 由 `GameTestServer`/`IntegratedServer` 启动期调用。幂等。
     */
    void registerBuiltinDefaults();

    void clear() noexcept;

private:
    EnvironmentRegistry() = default;

    std::unordered_map<std::string, std::shared_ptr<TestEnvironmentDefinition>> m_environments;
};

} // namespace mc::test
