#pragma once

#include "common/test/framework/helper/IGameTestHelperProvider.hpp"

#include <memory>

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief `IGameTestHelperProvider` 的 `ServerWorld` 具体实现。
 *
 * 创建绑 `ServerWorld` 的 `GameTestHelper`（facade 层具体类，实现 `IGameTestHelper`）。
 * 持 `ServerWorld&` 引用，`createGameTestHelper` 内向下转型 `MinecraftGameTestInstance&`
 * 取 origin/bounds 构造 helper。`clone()` 复制自身供重试用。
 *
 * 不对外——由 `MinecraftGameTestBatchRunner` 持有。
 */
class MinecraftGameTestHelperProvider final : public IGameTestHelperProvider {
public:
    explicit MinecraftGameTestHelperProvider(mc::server::ServerWorld& world);

    [[nodiscard]] std::unique_ptr<IGameTestHelper> createGameTestHelper(BaseGameTestInstance& instance) override;
    [[nodiscard]] std::unique_ptr<IGameTestHelperProvider> clone() const override;

private:
    mc::server::ServerWorld& m_world;
};

} // namespace mc::test
