/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "BlockEntityRendererDispatcher.hpp"
#include "BlockEntityRenderer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "renderers/BannerRenderer.hpp"
#include "renderers/BeaconRenderer.hpp"
#include "renderers/ChestRenderer.hpp"
#include <memory>
#include <spdlog/spdlog.h>

// 完整的方块实体类型头文件（用于 dynamic_cast）
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"

namespace mc::client::renderer::trident::blockentity {

BlockEntityRendererDispatcher::BlockEntityRendererDispatcher() = default;
BlockEntityRendererDispatcher::~BlockEntityRendererDispatcher() = default;

BlockEntityRendererDispatcher::BlockEntityRendererDispatcher(BlockEntityRendererDispatcher&&) noexcept = default;
BlockEntityRendererDispatcher& BlockEntityRendererDispatcher::operator=(
    BlockEntityRendererDispatcher&&) noexcept = default;

void BlockEntityRendererDispatcher::registerRenderer(BlockEntityType type, RendererFactory factory)
{
    if (!factory) {
        spdlog::warn(
            "BlockEntityRendererDispatcher: Attempted to register null factory for type {}", static_cast<u16>(type));
        return;
    }

    m_renderers[type] = factory();
}

bool BlockEntityRendererDispatcher::render(BlockEntity& entity, f32 partialTick, u32 light, i64 gameTime)
{
    const BlockEntityType type = entity.getType();
    auto it = m_renderers.find(type);

    if (it == m_renderers.end() || !it->second) {
        return false;
    }

    // 调用类型擦除的渲染方法
    return it->second->render(entity, partialTick, light, gameTime);
}

void BlockEntityRendererDispatcher::renderGlobalBlockEntities(IWorld& world, f32 partialTick)
{
    // 全局方块实体（如信标光束）可以在远距离看到，需要特殊处理
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

void BlockEntityRendererDispatcher::initializeDefaults()
{
    spdlog::info("BlockEntityRendererDispatcher: Initializing default renderers");

    // 注册已实现的渲染器
    registerRenderer(BlockEntityType::Beacon,
        []() -> std::unique_ptr<BlockEntityRendererBase> { return std::make_unique<BeaconRenderer>(); });

    registerRenderer(BlockEntityType::Chest,
        []() -> std::unique_ptr<BlockEntityRendererBase> { return std::make_unique<ChestRenderer>(); });

    registerRenderer(BlockEntityType::Banner,
        []() -> std::unique_ptr<BlockEntityRendererBase> { return std::make_unique<BannerRenderer>(); });

    // 注册其他渲染器将在各自实现完成后添加
    // - SignRenderer (告示牌)
    // - BedRenderer (床)
    // - BellRenderer (钟)
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

BlockEntityRendererBase* BlockEntityRendererDispatcher::getRenderer(BlockEntityType type)
{
    auto it = m_renderers.find(type);
    return it != m_renderers.end() ? it->second.get() : nullptr;
}

bool BlockEntityRendererDispatcher::hasRenderer(BlockEntityType type) const
{
    return m_renderers.find(type) != m_renderers.end();
}

void BlockEntityRendererDispatcher::clear()
{
    m_renderers.clear();
}

} // namespace mc::client::renderer::trident::blockentity
