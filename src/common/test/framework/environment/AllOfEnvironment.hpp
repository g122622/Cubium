#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include <memory>
#include <vector>

namespace mc::test {

/**
 * @brief 复合环境：依次 setup/teardown 一组子环境。
 *
 * 对齐 Java `TestEnvironmentDefinition.AllOf`（record，持 `List<Holder<TestEnvironmentDefinition>>`）。
 * setup 顺序执行所有子环境的 setup（任一失败即返回该错误，对齐 Java 抛异常即终止）；
 * teardown 逆序执行所有子环境的 teardown（尽力还原，忽略子环境 teardown 错误以保后续还原）。
 *
 * 便捷构造对齐 Java `AllOf(TestEnvironmentDefinition...)` 可变参数工厂。
 */
class AllOfEnvironment final : public TestEnvironmentDefinition {
public:
    AllOfEnvironment() = default;
    explicit AllOfEnvironment(std::vector<std::shared_ptr<TestEnvironmentDefinition>> definitions)
        : m_definitions(std::move(definitions))
    {}

    GameTestResult setup(BaseGameTestInstance& instance) override;
    GameTestResult teardown(BaseGameTestInstance& instance) override;

    [[nodiscard]] const std::vector<std::shared_ptr<TestEnvironmentDefinition>>& definitions() const noexcept
    {
        return m_definitions;
    }

private:
    std::vector<std::shared_ptr<TestEnvironmentDefinition>> m_definitions;
};

} // namespace mc::test
