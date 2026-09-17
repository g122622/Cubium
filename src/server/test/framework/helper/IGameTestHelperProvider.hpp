#pragma once

#include <memory>

namespace mc::test {

class BaseGameTestInstance;
class IGameTestHelper;

/**
 * @brief 测试助手创建者接口。
 *
 * 对齐基岩版 `IGameTestHelperProvider`：解耦 `BaseGameTestInstance` 与具体 `IGameTestHelper` 实现的创建。
 * `BaseGameTestInstance` 不直接 new `MinecraftGameTestHelper`，而是经此接口创建，便于：
 * - 原生测试用 `MinecraftGameTestHelperProvider`（创建绑 `ServerWorld` 的 `GameTestHelper`）。
 * - 单元测试用 `NullGameTestHelperProvider`（创建 `NullGameTestHelper`）。
 * - 脚本测试用 `ScriptGameTestHelperProvider`（创建包装 `ScriptGameTestHelper` 的适配器）。
 *
 * `clone()` 用于重试时为新实例创建独立 helper。
 */
class IGameTestHelperProvider {
public:
    virtual ~IGameTestHelperProvider() = default;

    /**
     * @brief 为指定测试实例创建助手。
     */
    [[nodiscard]] virtual std::unique_ptr<IGameTestHelper> createGameTestHelper(BaseGameTestInstance& instance) = 0;

    /**
     * @brief 克隆自身（用于重试时创建独立 provider）。
     */
    [[nodiscard]] virtual std::unique_ptr<IGameTestHelperProvider> clone() const = 0;
};

} // namespace mc::test
