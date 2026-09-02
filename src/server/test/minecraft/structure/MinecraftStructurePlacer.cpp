#include "server/test/minecraft/structure/MinecraftStructurePlacer.hpp"

#include "common/core/Types.hpp" // i32
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"  // MC_ASSERT_RELEASE
#include "common/world/IWorld.hpp"              // IWorld（setBlockState/getRandom）
#include "common/world/WorldConstants.hpp"      // MAX_BUILD_HEIGHT
#include "common/world/block/BlockRegistry.hpp" // mc::BlockRegistry::airState
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "common/world/gen/feature/template/Template.hpp"        // Template + PlacementSettings
#include "common/world/gen/feature/template/TemplateManager.hpp" // TemplateManager
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"           // JigsawAssembler::getTemplateManager
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/ServerChunkManager.hpp" // ServerChunkManager（chunkManager()->ticketManager()）
#include "server/world/ServerWorld.hpp"

#include <algorithm> // std::max
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
 *
 * @param flags setBlockState flags（对齐 vanilla Block.setFlags）：默认 3（UPDATE_NEIGHBORS|NOTIFY）
 *              触发 6 向邻居更新，适合小范围 padding 清理；大范围清理（如 skyAccess 清空高空 worldgen）
 *              传 18（UPDATE_CLIENTS|NOTIFY，无 bit0）避免每方块 6 向邻居更新的指数级开销。
 */
void _clearBox(mc::server::ServerWorld& world, const StructureBoundingBox& box, i32 flags = 3)
{
    const mc::BlockState* air = mc::BlockRegistry::instance().airState();
    for (i32 x = box.minX(); x <= box.maxX(); ++x) {
        for (i32 y = box.minY(); y <= box.maxY(); ++y) {
            for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
                world.setBlockState(x, y, z, air, flags);
            }
        }
    }
}

/// loadSpawnChunks 强制加载的区块半径（结构 footprint 中心周围）。
///
/// 不用 NaturalSpawner::SPAWN_DISTANCE_CHUNK=8（满载 289 区块）——289 区块同步 worldgen +
/// post-process 积压致主 tick 严重滞后（backlog>256），测试超时无法完成。改用半径 3（7×7=49 区块）：
///   - Monster cap = 70 * 49 / 289 = 11 > 0（怪物可生成）
///   - Creature cap = 10 * 49 / 289 = 1 > 0（动物可生成，每 400 tick 节流 1 个名额）
/// 49 区块 worldgen post-process backlog（~49）低于阈值 256，主 tick 不卡。
/// 副作用：结构外 49 区块 worldgen 出真实地形，NaturalSpawner 在结构外列（heightmap 落 worldgen
/// 地表 y≈62）也会选位——但结构 footprint 仅 3 区块，命中结构内 air 腔概率 3/49≈6%，需宽 maxTick
/// 轮询。结构外生成位多在露天白天地表（brightness=15 怪物拒）或夜晚地表（怪物生成残留），
/// 故 NaturalSpawner 测试须独立 batch（避免污染）+ 区域限定计数（只数结构内）。
constexpr i32 LOAD_SPAWN_CHUNK_RADIUS = 3;

/**
 * @brief 强制加载以 (centerCx, centerCz) 为中心、半径 LOAD_SPAWN_CHUNK_RADIUS 的所有区块。
 *
 * GameTestServer 的 `SimulatedPlayer` 缺真实玩家区块加载链路（`_loadPlayerChunks` 对 PlayerId=0
 * 是 no-op），NaturalSpawner._collectSpawnableChunks 仅数到结构 footprint 区块（3 个），
 * cap=maxInstances*3/289=0 致 activeCategories.empty() 早退。本函数 force 结构中心周围区块
 * 使 spawnableChunkCount 达 49，cap>0。
 * 顺序同结构 footprint force：先 forceChunk ticket + processUpdates，再 requestFullChunkSync
 * 同步 worldgen；已生成区块命中 `tryToGetChunkInMem` 短路，不重复 worldgen。
 */
void _forceSpawnChunks(mc::server::ServerChunkManager& chunkManager, mc::ChunkCoord centerCx, mc::ChunkCoord centerCz)
{
    auto& ticketManager = chunkManager.ticketManager();
    for (i32 dx = -LOAD_SPAWN_CHUNK_RADIUS; dx <= LOAD_SPAWN_CHUNK_RADIUS; ++dx) {
        for (i32 dz = -LOAD_SPAWN_CHUNK_RADIUS; dz <= LOAD_SPAWN_CHUNK_RADIUS; ++dz) {
            ticketManager.forceChunk(centerCx + dx, centerCz + dz, true);
        }
    }
    ticketManager.processUpdates();
    for (i32 dx = -LOAD_SPAWN_CHUNK_RADIUS; dx <= LOAD_SPAWN_CHUNK_RADIUS; ++dx) {
        for (i32 dz = -LOAD_SPAWN_CHUNK_RADIUS; dz <= LOAD_SPAWN_CHUNK_RADIUS; ++dz) {
            // requestFullChunkSync 阻塞等待生成完成；返回值仅用于触发加载，不持有。
            (void)chunkManager.requestFullChunkSync(centerCx + dx, centerCz + dz);
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

        // 强制加载结构 footprint 外扩 1 区块，并使用 BlockTicking(32) 级别而非 Full(33)。
        // 背景：振动系统的 _areAdjacentChunksTicking 要求监听器周围 3x3 区块全部 <= BlockTicking(32)。
        // 但 forceChunk 用 Full(33) 票据——33 > 32 不满足 BlockTicking 门控，且仅覆盖结构 footprint。
        // 解法：外扩 1 区块覆盖监听器周围 3x3，并改用 registerTicket 显式指定 BlockTicking(32) 级别。
        // 注意：原 forceChunk 使用 Full(33)，但 Full(33) > BlockTicking(32)，故 vibration gate 失败。
        constexpr i32 CHUNK_RADIUS = 1;
        const ChunkCoord forcedMinCx = minCx - CHUNK_RADIUS;
        const ChunkCoord forcedMaxCx = maxCx + CHUNK_RADIUS;
        const ChunkCoord forcedMinCz = minCz - CHUNK_RADIUS;
        const ChunkCoord forcedMaxCz = maxCz + CHUNK_RADIUS;
        auto& ticketManager = chunkManager->ticketManager();
        for (ChunkCoord cx = forcedMinCx; cx <= forcedMaxCx; ++cx) {
            for (ChunkCoord cz = forcedMinCz; cz <= forcedMaxCz; ++cz) {
                // 使用 BlockTicking(32) 级别，确保振动门控通过且方块可 tick。
                ticketManager.registerTicket(
                    mc::world::chunk::TicketTypes::FORCED, cx, cz, static_cast<i32>(mc::world::chunk::ChunkLoadLevel::BlockTicking), mc::ChunkPos(cx, cz));
            }
        }
        ticketManager.processUpdates();
        for (ChunkCoord cx = forcedMinCx; cx <= forcedMaxCx; ++cx) {
            for (ChunkCoord cz = forcedMinCz; cz <= forcedMaxCz; ++cz) {
                // requestFullChunkSync 阻塞等待生成完成；返回值仅用于触发加载，不持有。
                (void)chunkManager->requestFullChunkSync(cx, cz);
            }
        }

        // loadSpawnChunks：额外 force 结构 footprint 中心周围 SPAWN_DISTANCE_CHUNK(8) 区块。
        // GameTestServer 无头门面无玩家区块加载链路，NaturalSpawner._collectSpawnableChunks
        // 仅数到结构 footprint 区块（3 个），cap=maxInstances*3/289=0 致 activeCategories.empty()
        // 早退。开启此标志以结构中心区块为圆心 force 8 区块半径（满载 289），对齐原版
        // DistanceManager.getNaturalSpawnChunkCount。玩家站结构内，结构中心区块≈玩家所在区块，
        // 覆盖玩家周围 8 区块。副作用见 _forceSpawnChunks 注释（结构外 worldgen + 自然生成污染）。
        if (data.loadSpawnChunks()) {
            const ChunkCoord centerCx = (minCx + maxCx) / 2;
            const ChunkCoord centerCz = (minCz + maxCz) / 2;
            _forceSpawnChunks(*chunkManager, centerCx, centerCz);
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

    // skyAccess=true 时清空结构 footprint（含 padding 外围）正上方至世界顶部的所有方块，
    // 确保该列无 worldgen 方块遮挡，canSeeSky=true（skyLight 重算后达 15）。
    // 背景：GameTestServer gridStartY=-59 把结构埋在地下 worldgen 石头中，结构上方（y>placedBox.maxY）
    // 全是 worldgen 方块，canSeeSky 恒 false——这会使依赖阳光的测试（如骷髅阳光下燃烧）稳定失败。
    // 对齐基岩 GameTest skyAccess 语义："结构上方露天，允许天空光照进入"——基岩结构本就放在地表露天，
    // 而项目结构埋于地下，故需主动清空上方 worldgen 制造露天列。对齐 vanilla heightmap：整列无遮挡方
    // 块时 skyLight=15，canSeeSky=true。
    // 清理范围 X/Z 用 padding 扩展（与封顶逻辑对称），Y 从 placedBox.maxY+1 到 MAX_BUILD_HEIGHT-1。
    // setBlockState(air) 入队光照变更（m_lightQueue），后续世界 tick 的 drainAndProcess 批量重算
    // skyLight；测试设 setupTicks 给光照重算留时间（见各 skyAccess 测试）。
    if (data.skyAccess()) {
        const i32 padXZ = std::max(0, data.padding());
        const StructureBoundingBox skyBox(placedBox.minX() - padXZ,
            placedBox.maxY() + 1,
            placedBox.minZ() - padXZ,
            placedBox.maxX() + padXZ,
            mc::world::MAX_BUILD_HEIGHT - 1,
            placedBox.maxZ() + padXZ);
        // flags=18（UPDATE_CLIENTS|NOTIFY，无 UPDATE_NEIGHBORS bit0）：清空高空 worldgen 不触发
        // 6 向邻居更新（30300 方块 ×6 邻居的指数级开销），对齐 placeInWorld 的 flags=18 语义。
        // 光照变更仍入队 m_lightQueue（flags 不影响光照入队），后续 tick 重算 skyLight。
        _clearBox(world, skyBox, 18);
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
