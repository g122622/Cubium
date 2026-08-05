#pragma once

#include "common/test/base/data/TestData.hpp"
#include "server/test/minecraft/structure/StructureBounds.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"

#include <memory>

namespace mc::server {
class ServerWorld;
}

namespace mc::test {

/**
 * @brief 结构放置器：在 `ServerWorld` 中放置测试结构 + 屏障包裹（尊重 skyAccess）+ 周边清理。
 *
 * 对齐基岩版 `MinecraftGameTestHelper::spawnStructure`/Java `StructureSpawner` 语义：
 * 1. 经 `JigsawAssembler::getTemplateManager()` 取 `Template`（项目无 server 级 TemplateManager 访问器，
 *    结构模板管理器是全局单例，由 `RegistryBootstrap` 在启动期注入资源包/数据包）。
 * 2. 按 `TestData.rotation()` 构造 `PlacementSettings`（setRotation + setIgnoreEntities=false）。
 * 3. 放置前按 `TestData.padding()` 清理周边（写 air），避免残留方块干扰测试。
 * 4. `Template::placeInWorld` 放置结构（含方块实体/实体）。
 * 5. 屏障包裹：周边写 air 清理 +（当 `!TestData.skyAccess()`）顶部封顶，隔离光照与外部实体。
 *
 * `place()` 返回放置后的 `StructureBounds`（旋转后包围盒），供 instance 计算坐标变换与清理范围。
 */
class MinecraftStructurePlacer {
public:
    /**
     * @brief 放置结构到 `origin`。
     *
     * @param world 目标世界（须已初始化，否则 setBlockState 触发光照断言）。
     * @param origin 结构原点（旋转后包围盒的 min 角）。
     * @param data 测试元数据（取 structure/rotation/padding/skyAccess）。
     * @return 放置后的 `StructureBounds`；结构加载失败返回 nullptr。
     */
    [[nodiscard]] std::unique_ptr<StructureBounds> place(
        server::ServerWorld& world, const BlockPos& origin, const TestData& data);

    /**
     * @brief 清理结构范围及周边 padding 格（写 air），供测试结束清理复用。
     */
    void clearArea(server::ServerWorld& world, const StructureBounds& bounds, i32 padding);
};

} // namespace mc::test
