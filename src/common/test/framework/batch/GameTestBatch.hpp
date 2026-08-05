#pragma once

#include "common/test/base/data/TestData.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc::test {

class BaseGameTestFunction;
class TestEnvironmentDefinition;

/**
 * @brief 测试批次（注册期）。
 *
 * 对齐基岩版 `GameTestBatch`：一组测试函数 + before/after 回调 + 环境定义。批次是 runner 调度的单位——
 * 同批次测试共享 before/after 与环境 setup/teardown。Java 版 `GameTestBatch` 持运行期 `GameTestInfo`，
 * 此处对齐基岩持注册期 `BaseGameTestFunction`（见校正 15 两阶段决策）。
 *
 * `GameTestBatchFactory` 按 `environment` 分组、`MAX_TESTS_PER_BATCH=50` 切片生成批次。
 */
class GameTestBatch {
public:
    GameTestBatch(std::string name,
        std::vector<std::shared_ptr<BaseGameTestFunction>> testFunctions,
        std::function<void()> beforeBatch,
        std::function<void()> afterBatch,
        std::shared_ptr<TestEnvironmentDefinition> environment);

    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] const std::vector<std::shared_ptr<BaseGameTestFunction>>& testFunctions() const noexcept
    {
        return m_testFunctions;
    }
    [[nodiscard]] const std::function<void()>& beforeBatch() const noexcept { return m_beforeBatch; }
    [[nodiscard]] const std::function<void()>& afterBatch() const noexcept { return m_afterBatch; }
    [[nodiscard]] const std::shared_ptr<TestEnvironmentDefinition>& environment() const noexcept
    {
        return m_environment;
    }

private:
    std::string m_name;
    std::vector<std::shared_ptr<BaseGameTestFunction>> m_testFunctions;
    std::function<void()> m_beforeBatch;
    std::function<void()> m_afterBatch;
    std::shared_ptr<TestEnvironmentDefinition> m_environment;
};

} // namespace mc::test
