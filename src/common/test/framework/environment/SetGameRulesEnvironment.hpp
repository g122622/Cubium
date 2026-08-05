#pragma once

#include "common/test/framework/environment/TestEnvironmentDefinition.hpp"

#include <string>
#include <unordered_map>

namespace mc::test {

/**
 * @brief 环境定义：批量设置游戏规则。
 *
 * 对齐 Java `TestEnvironmentDefinition.SetGameRules`（record，持 `GameRuleMap`）：setup 调
 * `GameRules.setAll(map, server)`，teardown 逐条重置为默认值。
 *
 * 此处 framework 层仅持意图（规则名→值映射）；实际应用到 `ServerWorld` 由 1C 阶段
 * `MinecraftEnvironmentApplier` 完成。本类 setup/teardown 返回 `MethodNotImplemented` 错误并留 TODO——
 * 经由 applier 拦截应用后此 stub 不再被调用。
 */
class SetGameRulesEnvironment final : public TestEnvironmentDefinition {
public:
    SetGameRulesEnvironment() = default;
    explicit SetGameRulesEnvironment(std::unordered_map<std::string, std::string> rules)
        : m_rules(std::move(rules))
    {}

    GameTestResult setup(BaseGameTestInstance& instance) override;
    // teardown 不覆盖：Java 逐条 resetRule 由 applier 在 teardown 阶段处理，framework 层无状态可还原。

    [[nodiscard]] const std::unordered_map<std::string, std::string>& rules() const noexcept { return m_rules; }

private:
    std::unordered_map<std::string, std::string> m_rules;
};

} // namespace mc::test
