#pragma once

#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "server/test/minecraft/structure/MinecraftStructurePlacer.hpp"
#include "server/test/minecraft/structure/StructureBounds.hpp"
#include "common/world/block/BlockPos.hpp"

#include <memory>

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief `BaseGameTestInstance` 的 `ServerWorld` 具体实现。
 *
 * 持 `ServerWorld&` 引用 + `MinecraftStructurePlacer`，实现框架层的 5 个纯虚：
 * - `spawnStructure()`：经 placer 放置 `TestData.structure()` 到 `m_origin`，存 `StructureBounds`，
 *   调 `notifyStructureLoaded()` 通知监听器。
 * - `clearStructure()`：经 placer 清理结构范围 + padding。
 * - `hasStructureBlock()`：结构是否已放置（`m_bounds != nullptr`）。
 * - `_getLevelTick()`：取 `ServerWorld` 当前 tick（驱动 runAtTickTime 时序）。
 * - `_isTestReady()`：结构已放置即就绪。
 *
 * 不对外——由 `MinecraftGameTestBatchRunner`（`minecraft/batch/`）创建，外部经门面 `GameTestServer`/
 * `GameTestCommand` 间接接触。
 */
class MinecraftGameTestInstance final : public BaseGameTestInstance {
public:
    MinecraftGameTestInstance(const BaseGameTestFunction& function,
        std::unique_ptr<IGameTestHelperProvider> helperProvider,
        mc::server::ServerWorld& world,
        BlockPos origin);

    [[nodiscard]] mc::server::ServerWorld& world() const noexcept { return m_world; }
    [[nodiscard]] const BlockPos& origin() const noexcept { return m_origin; }
    [[nodiscard]] const StructureBounds* bounds() const noexcept { return m_bounds.get(); }

protected:
    [[nodiscard]] bool hasStructureBlock() const noexcept override;
    void clearStructure() override;
    void spawnStructure() override;
    [[nodiscard]] i32 _getLevelTick() const override;
    [[nodiscard]] bool _isTestReady() override;

private:
    mc::server::ServerWorld& m_world;
    BlockPos m_origin;
    MinecraftStructurePlacer m_placer;
    std::unique_ptr<StructureBounds> m_bounds;
    bool m_structurePlaced = false;
};

} // namespace mc::test
