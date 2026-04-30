#include "BlockEntityRendererDispatcher.hpp"
#include "BlockEntityRenderer.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/IWorld.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "renderers/BeaconRenderer.hpp"
#include "renderers/ChestRenderer.hpp"
#include <spdlog/spdlog.h>

// 完整的方块实体类型头文件（用于 dynamic_cast）
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"

namespace mc::client::renderer::trident::blockentity {

BlockEntityRendererDispatcher::BlockEntityRendererDispatcher() = default;
BlockEntityRendererDispatcher::~BlockEntityRendererDispatcher() = default;

BlockEntityRendererDispatcher::BlockEntityRendererDispatcher(BlockEntityRendererDispatcher&&) noexcept = default;
BlockEntityRendererDispatcher& BlockEntityRendererDispatcher::operator=(BlockEntityRendererDispatcher&&) noexcept = default;

void BlockEntityRendererDispatcher::registerRenderer(BlockEntityType type, RendererFactory factory) {
    if (!factory) {
        spdlog::warn("BlockEntityRendererDispatcher: Attempted to register null factory for type {}",
                     static_cast<u16>(type));
        return;
    }

    m_renderers[type] = factory();
    spdlog::debug("BlockEntityRendererDispatcher: Registered renderer for type {}",
                  static_cast<u16>(type));
}

bool BlockEntityRendererDispatcher::render(BlockEntity& entity, f32 partialTick, u32 light) {
    const BlockEntityType type = entity.getType();
    auto it = m_renderers.find(type);

    if (it == m_renderers.end() || !it->second) {
        return false;
    }

    // 调用类型擦除的渲染方法
    return it->second->render(entity, partialTick, light);
}

void BlockEntityRendererDispatcher::renderGlobalBlockEntities(IWorld& world, f32 partialTick) {
    // MC 1.16.5: 遍历所有全局方块实体并渲染
    // 全局方块实体包括：
    // - 信标光束（isGlobalRenderer = true，渲染距离 256 格）
    // - 末地传送门（渲染距离 256 格）
    //
    // 这些方块实体可以在远距离看到，需要特殊处理
    //
    // 完整实现需要：
    // 1. 在世界加载时收集所有全局方块实体
    // 2. 按类型分派到对应渲染器
    // 3. 跳过距离检查（或在渲染器中处理）

    for (auto& [type, renderer] : m_renderers) {
        if (renderer && renderer->isGlobalRenderer()) {
            // 渲染全局方块实体
            // 需要遍历世界中该类型的所有方块实体
            (void)world;
            (void)partialTick;
        }
    }
}

void BlockEntityRendererDispatcher::initializeDefaults() {
    spdlog::info("BlockEntityRendererDispatcher: Initializing default renderers");

    // MC 1.16.5 TileEntityRendererDispatcher 构造函数中注册的渲染器：
    // - SIGN -> SignTileEntityRenderer
    // - MOB_SPAWNER -> MobSpawnerTileEntityRenderer
    // - PISTON -> PistonTileEntityRenderer
    // - CHEST, ENDER_CHEST, TRAPPED_CHEST -> ChestTileEntityRenderer
    // - ENCHANTING_TABLE -> EnchantmentTableTileEntityRenderer
    // - LECTERN -> LecternTileEntityRenderer
    // - END_PORTAL -> EndPortalTileEntityRenderer
    // - END_GATEWAY -> EndGatewayTileEntityRenderer
    // - BEACON -> BeaconTileEntityRenderer
    // - SKULL -> SkullTileEntityRenderer
    // - BANNER -> BannerTileEntityRenderer
    // - STRUCTURE_BLOCK -> StructureTileEntityRenderer
    // - SHULKER_BOX -> ShulkerBoxTileEntityRenderer
    // - BED -> BedTileEntityRenderer
    // - CONDUIT -> ConduitTileEntityRenderer
    // - BELL -> BellTileEntityRenderer
    // - CAMPFIRE -> CampfireTileEntityRenderer

    // 注册已实现的渲染器
    registerRenderer(BlockEntityType::Beacon, []() -> std::unique_ptr<BlockEntityRendererBase> {
        return std::make_unique<BeaconRenderer>();
    });

    registerRenderer(BlockEntityType::Chest, []() -> std::unique_ptr<BlockEntityRendererBase> {
        return std::make_unique<ChestRenderer>();
    });

    // 注册其他渲染器将在各自实现完成后添加
    // - SignRenderer (告示牌)
    // - BedRenderer (床)
    // - BellRenderer (钟)
    // - BannerRenderer (旗帜)
    // - ShulkerBoxRenderer (潜影盒)
    // - ConduitRenderer (潮涌核心)
    // - LecternRenderer (讲台)
    // - CampfireRenderer (营火)
    // - EnchantmentTableRenderer (附魔台)
    // - EndPortalRenderer (末地传送门)
    // - EndGatewayRenderer (末地折跃门)
    // - SkullRenderer (头颅)
    // - StructureBlockRenderer (结构方块)
    // - MobSpawnerRenderer (刷怪笼)
}

BlockEntityRendererBase* BlockEntityRendererDispatcher::getRenderer(BlockEntityType type) {
    auto it = m_renderers.find(type);
    return it != m_renderers.end() ? it->second.get() : nullptr;
}

bool BlockEntityRendererDispatcher::hasRenderer(BlockEntityType type) const {
    return m_renderers.find(type) != m_renderers.end();
}

void BlockEntityRendererDispatcher::clear() {
    m_renderers.clear();
}

} // namespace mc::client::renderer::trident::blockentity
