#include "BlockEntityRendererDispatcher.hpp"
#include "BlockEntityRenderer.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/IWorld.hpp"
#include "client/renderer/trident/core/TridentContext.hpp"
#include "client/resource/BlockModelCache.hpp"
#include <spdlog/spdlog.h>

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

    // 类型擦除的渲染器无法直接调用模板方法
    // 这里需要一种机制来分派到正确的类型
    // TODO: 实现类型安全的渲染分派

    return true;
}

void BlockEntityRendererDispatcher::renderGlobalBlockEntities(IWorld& world, f32 partialTick) {
    // 遍历所有全局方块实体
    // TODO: 实现全局方块实体列表和渲染

    // 全局方块实体包括：
    // - 信标光束
    // - 末地传送门
    // 等可以在远距离看到的方块实体
}

void BlockEntityRendererDispatcher::initializeDefaults() {
    spdlog::info("BlockEntityRendererDispatcher: Initializing default renderers");

    // 默认渲染器将在具体渲染器实现后注册
    // registerRenderer<PistonBlockEntity, PistonRenderer>();
    // registerRenderer<ChestEntity, ChestRenderer>();
    // registerRenderer<BeaconEntity, BeaconRenderer>();
    // 等等...
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
