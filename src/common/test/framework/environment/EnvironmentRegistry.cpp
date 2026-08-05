#include "common/test/framework/environment/EnvironmentRegistry.hpp"

#include "common/test/framework/environment/AllOfEnvironment.hpp"

namespace mc::test {

EnvironmentRegistry& EnvironmentRegistry::instance() noexcept
{
    static EnvironmentRegistry s_instance;
    return s_instance;
}

bool EnvironmentRegistry::registerEnvironment(
    const std::string& name, std::shared_ptr<TestEnvironmentDefinition> environment)
{
    if (!environment || m_environments.find(name) != m_environments.end()) {
        return false;
    }
    m_environments[name] = std::move(environment);
    return true;
}

std::shared_ptr<TestEnvironmentDefinition> EnvironmentRegistry::getEnvironment(const std::string& name) const
{
    const auto it = m_environments.find(name);
    return it != m_environments.end() ? it->second : nullptr;
}

bool EnvironmentRegistry::hasEnvironment(const std::string& name) const noexcept
{
    return m_environments.find(name) != m_environments.end();
}

void EnvironmentRegistry::registerBuiltinDefaults()
{
    // 对齐 Java GameTestEnvironments.DEFAULT = "default" → AllOf(List.of())
    if (!hasEnvironment("default")) {
        registerEnvironment("default", std::make_shared<AllOfEnvironment>());
    }
}

void EnvironmentRegistry::clear() noexcept
{
    m_environments.clear();
}

} // namespace mc::test
