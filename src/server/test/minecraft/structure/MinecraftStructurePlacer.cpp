#include "server/test/minecraft/structure/MinecraftStructurePlacer.hpp"

#include "common/core/Types.hpp" // i32
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"  // MC_ASSERT_RELEASE
#include "common/world/IWorld.hpp"              // IWorld（setBlockState/getRandom）
#include "common/world/block/BlockRegistry.hpp" // mc::BlockRegistry::airState
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/world/gen/feature/template/Template.hpp"        // Template + PlacementSettings
#include "common/world/gen/feature/template/TemplateManager.hpp" // TemplateManager
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"           // JigsawAssembler::getTemplateManager
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/ServerChunkManager.hpp" // ServerChunkManager（chunkManager()->ticketManager()）
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

namespace mc::test {

namespace {

// 全限定命名空间路径，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using Template = mc::world::gen::feature::template_::Template;
using TemplateManager = mc::world::gen::feature::template_::TemplateManager;
using PlacementSettings = mc::world::gen::feature::template_::PlacementSettings;
using StructureBoundingBox = mc::world::gen::structure::StructureBoundingBox;
using JigsawAssembler = mc::world::gen::jigsaw::JigsawAssembler;

/**
 * @brief 取结构模板（经全局 TemplateManager 单例）。
 *
 * 项目无 server 级 TemplateManager 访问器；结构模板管理器是 `JigsawAssembler` 持有的全局单例，
 * 由 `RegistryBootstrap` 在服务端启动期注入资源包/数据包。GameTestServer/IntegratedServer 启动后可用。
 */
const Template* _getTemplate(const std::string& structureName)
{
    auto& tm = JigsawAssembler::getTemplateManager();
    const mc::resource::ResourceLocation loc(structureName);
    return tm.getTemplate(loc);
}

/**
 * @brief 用 air 清空指定 StructureBoundingBox 范围（含方块更新）。
 */
void _clearBox(mc::server::ServerWorld& world, const StructureBoundingBox& box)
{
    const mc::BlockState* air = mc::BlockRegistry::instance().airState();
    for (i32 x = box.minX(); x <= box.maxX(); ++x) {
        for (i32 y = box.minY(); y <= box.maxY(); ++y) {
            for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
                world.setBlockState(x, y, z, air);
            }
        }
    }
}

} // namespace

std::unique_ptr<StructureBounds> MinecraftStructurePlacer::place(
    mc::server::ServerWorld& world, const BlockPos& origin, const TestData& data)
{
    const auto* tpl = _getTemplate(data.structure());
    if (tpl == nullptr) {
        spdlog::warn("GameTest: structure '{}' not found in TemplateManager", data.structure());
        return nullptr;
    }

    // 基岩 GameTest 结构放置约定（对齐原版 StructureBlockActor / SpawnStructure）：
    // 结构方块位于 origin（northWestCorner），结构内容从结构方块"上方一格"开始放置——
    // 即结构模板内坐标 (0,0,0) 放在 origin + (0,1,0)。GameTestHelper 的相对坐标原点 = origin
    // （结构方块本身），故相对 y=N 对应结构内 y=N-1。若不抬高一格，pressButton/spawn 等按相对
    // 坐标定位的方块/实体将整体下移一格（button 落在地板而非贴墙位置），与原版行为包错配。
    // 例如 clone_command 的 acacia_button 在结构内 (1,1,0)，JS pressButton({x:1,y:2,z:0})
    // 经 helper → origin+(1,2,0)，抬高一格后结构 (1,1,0) 恰好落在 origin+(1,2,0)。
    const BlockPos placeOrigin{origin.x, origin.y + 1, origin.z};

    // 计算旋转后包围盒（用 Template::getBoundingBox 取权威包围盒，含旋转）
    PlacementSettings settings;
    settings.setRotation(data.rotation());
    settings.setIgnoreEntities(false);
    const StructureBoundingBox placedBox = tpl->getBoundingBox(settings, placeOrigin);

    // 强制加载结构覆盖的所有 chunk（FORCED ticket + 同步生成），须在 placeInWorld 之前。
    // GameTestServer 是无头门面，无玩家无 spawn chunk 加载范围；initializeWorldSpawn 仅加载单一
    // 出生区块。测试结构沿 gridStart 在 X 方向线性铺开（跨多个 chunk），远离出生区块的结构所在
    // chunk 不会被加载，setBlockState/getBlockState 因 getChunkSync 返回 nullptr 而失败——
    // 表现为"结构放置静默失败"（placeInWorld 不检查 placed）+ 测试体 setBlockType/spawn 报错
    // （如 zombie_villager_chase 的 brick_block 在 chunk(6,0) 失败）。
    // 对齐原版 GameTestRunner：为每个测试结构区域加 forced chunk ticket，确保测试期间 chunk 常驻。
    // 顺序关键：必须先 force + 同步生成 chunk（worldgen 填地形），再 placeInWorld 放结构覆盖地形；
    // 若反过来，worldgen 会用 terrain 方块覆盖已放置的结构方块。
    // requestFullChunkSync 内部 tryToGetChunkInMem 命中则直接返回，已生成 chunk 不会重新 worldgen。
    // 测试结束世界销毁，无需显式 unforce。
    auto* chunkManager = world.chunkManager();
    if (chunkManager != nullptr) {
        const BlockPos minCorner(placedBox.minX(), placedBox.minY(), placedBox.minZ());
        const BlockPos maxCorner(placedBox.maxX(), placedBox.maxY(), placedBox.maxZ());
        const ChunkCoord minCx = minCorner.chunkX();
        const ChunkCoord maxCx = maxCorner.chunkX();
        const ChunkCoord minCz = minCorner.chunkZ();
        const ChunkCoord maxCz = maxCorner.chunkZ();
        auto& ticketManager = chunkManager->ticketManager();
        for (ChunkCoord cx = minCx; cx <= maxCx; ++cx) {
            for (ChunkCoord cz = minCz; cz <= maxCz; ++cz) {
                ticketManager.forceChunk(cx, cz, true);
            }
        }
        ticketManager.processUpdates();
        for (ChunkCoord cx = minCx; cx <= maxCx; ++cx) {
            for (ChunkCoord cz = minCz; cz <= maxCz; ++cz) {
                // requestFullChunkSync 阻塞等待生成完成；返回值仅用于触发加载，不持有。
                (void)chunkManager->requestFullChunkSync(cx, cz);
            }
        }
    }

    // 放置前清理周边（padding 格），避免残留方块/实体干扰测试
    if (data.padding() > 0) {
        const StructureBoundingBox padBox(placedBox.minX() - data.padding(),
            placedBox.minY() - data.padding(),
            placedBox.minZ() - data.padding(),
            placedBox.maxX() + data.padding(),
            placedBox.maxY() + data.padding(),
            placedBox.maxZ() + data.padding());
        _clearBox(world, padBox);
    }

    // 放置结构（含方块/方块实体/实体），flags=18 对齐 vanilla 默认（UPDATE_CLIENTS|NOTIFY）
    mc::math::Random& rng = world.getRandom();
    const bool ok = tpl->placeInWorld(world, placeOrigin, settings, rng, 18);
    if (!ok) {
        spdlog::warn("GameTest: failed to place structure '{}'", data.structure());
        return nullptr;
    }

    // 屏障包裹：当 !skyAccess 时顶部封顶（隔离光照）；周边已在 padding 阶段清理
    // TODO: 完整屏障包裹（玻璃/屏障方块边界）待 1C 后续完善；当前仅清理周边 air 隔离
    if (!data.skyAccess() && data.padding() > 0) {
        const StructureBoundingBox topBox(placedBox.minX() - data.padding(),
            placedBox.maxY() + 1,
            placedBox.minZ() - data.padding(),
            placedBox.maxX() + data.padding(),
            placedBox.maxY() + 1,
            placedBox.maxZ() + data.padding());
        _clearBox(world, topBox);
    }

    // 结构尺寸由 placedBox 跨度推导（旋转后权威值）
    const BlockPos size{
        placedBox.maxX() - placedBox.minX() + 1,
        placedBox.maxY() - placedBox.minY() + 1,
        placedBox.maxZ() - placedBox.minZ() + 1,
    };
    // StructureBounds 的 origin 用传入的 origin（结构方块位置，= helper 相对坐标原点），
    // 非 placeOrigin（结构内容起始）。helper 经 StructureBounds 取 origin 算 relativeToWorld。
    // （forceChunk 在 placeInWorld 之前已用 placedBox 的 X/Z 范围完成，见上方。）
    (void)origin;

    return std::make_unique<StructureBounds>(origin, size, data.rotation());
}

void MinecraftStructurePlacer::clearArea(mc::server::ServerWorld& world, const StructureBounds& bounds, i32 padding)
{
    _clearBox(world, bounds.paddingBounds(padding));
}

} // namespace mc::test
