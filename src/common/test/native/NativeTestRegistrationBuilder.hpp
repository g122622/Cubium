#pragma once

#include "common/test/base/data/TestData.hpp"
#include "common/test/native/NativeGameTestFunction.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mc::test {

/**
 * @brief 原生测试注册 builder（11 链式方法，镜像 `TestData` 字段）。
 *
 * 对齐基岩 `RegistrationBuilder`/JS `RegistrationBuilder`：由 `GameTestRegistrar::register()`（facade 1F）
 * 创建，作者链式设置 `structureName/maxTicks/padding/required/...` 后 `register()` 提交到 `GameTestRegistry`。
 *
 * 与 `ScriptRegistrationBuilder`（JS 注册，`script/`）/`GameTestRunnerBuilder`（运行，`runner/`）前缀区分。
 *
 * 11 链式方法（对齐 JS `RegistrationBuilder`）：`batch`/`maxAttempts`/`maxTicks`/`padding`/`required`/
 * `requiredSuccessfulAttempts`/`rotate`/`setupTicks`/`structureName`/`structureLocation`/`tag`。
 */
class NativeTestRegistrationBuilder {
public:
    NativeTestRegistrationBuilder(std::string className, std::string testName, NativeGameTestFunction::TestBody body)
        : m_className(std::move(className))
        , m_testName(std::move(testName))
        , m_body(std::move(body))
    {}

    NativeTestRegistrationBuilder& batch(std::string name)
    {
        m_data.setBatchName(std::move(name));
        return *this;
    }
    NativeTestRegistrationBuilder& maxAttempts(i32 n) noexcept
    {
        m_data.setMaxAttempts(n);
        return *this;
    }
    NativeTestRegistrationBuilder& maxTicks(i32 n) noexcept
    {
        m_data.setMaxTicks(n);
        return *this;
    }
    NativeTestRegistrationBuilder& padding(i32 n) noexcept
    {
        m_data.setPadding(n);
        return *this;
    }
    NativeTestRegistrationBuilder& required(bool r) noexcept
    {
        m_data.setRequired(r);
        return *this;
    }
    NativeTestRegistrationBuilder& requiredSuccessfulAttempts(i32 n) noexcept
    {
        m_data.setRequiredSuccesses(n);
        return *this;
    }
    NativeTestRegistrationBuilder& rotate(bool r) noexcept
    {
        // JS `rotateTest(bool)`：true=允许 4 旋转压测；此处暂存为标签，--verify 由 GameTestServer 展开
        // TODO: 旋转压测标记待 GameTestServer --verify 接线时消费
        m_rotate = r;
        return *this;
    }
    NativeTestRegistrationBuilder& setupTicks(i32 n) noexcept
    {
        m_data.setSetupTicks(n);
        return *this;
    }
    NativeTestRegistrationBuilder& structureName(std::string name)
    {
        m_data.setStructure(std::move(name));
        return *this;
    }
    NativeTestRegistrationBuilder& structureLocation(std::string name)
    {
        // JS `structureLocation` 等价 `structureName`（结构资源位置）
        m_data.setStructure(std::move(name));
        return *this;
    }
    NativeTestRegistrationBuilder& tag(std::string t)
    {
        m_tags.push_back(std::move(t));
        return *this;
    }

    /**
     * @brief 提交注册到 `GameTestRegistry`。返回 true=注册成功。
     *
     * 由 `GameTestRegistrar::register()`（facade）调用；作者经 `MC_REGISTER_GAME_TEST` 宏间接调用。
     */
    bool registerTest();

private:
    std::string m_className;
    std::string m_testName;
    NativeGameTestFunction::TestBody m_body;
    TestData m_data;
    std::vector<std::string> m_tags;
    bool m_rotate = false;
};

} // namespace mc::test
