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

#include "client/application/ClientApplication.hpp"

#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/sound/AudioService.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/VanillaResources.hpp"
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>

using namespace mc::trace;

namespace mc::client {

namespace {

void addBuiltinFolderPackIfPresent(ResourceManager& resourceManager, const std::filesystem::path& builtinPackDir)
{
    if (!std::filesystem::exists(builtinPackDir)) {
        spdlog::info("Built-in resource pack directory not found, skipping: {}", builtinPackDir.string());
        return;
    }

    auto builtinResourcesPack = std::make_shared<FolderResourcePack>(builtinPackDir.string());
    auto builtinResult = builtinResourcesPack->initialize();
    if (builtinResult.success()) {
        (void)resourceManager.addResourcePack(std::move(builtinResourcesPack));
        spdlog::info("Added built-in resource pack from: {}", builtinPackDir.string());
    } else {
        spdlog::warn("Failed to initialize built-in resource pack: {}", builtinResult.error().toString());
    }
}

} // namespace

Result<void> ClientApplication::initializeResources()
{
    // 1. 创建 ResourceManager 并首先添加内置资源包（最低优先级）
    m_resourceManager = std::make_unique<ResourceManager>();

    // 添加原版内置资源包（提供基础模型如 cube_all, cube_column 等）
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
        spdlog::info("Added vanilla built-in resource pack");
    } else {
        spdlog::warn("Failed to initialize vanilla resource pack: {}", vanillaResult.error().toString());
    }

    // 添加项目内置资源包（提供基础纹理、模型等）
    // 内置包位于可执行文件旁的 resources/data/minecraft/
    addBuiltinFolderPackIfPresent(*m_resourceManager, m_gameDirectory.builtinPackDir());

    // 2. 扫描资源包目录（从游戏目录获取路径）
    auto resourcePackDir = m_gameDirectory.resourcePacksDir();
    spdlog::info("Scanning resource pack directory: {}", resourcePackDir.string());
    auto scanResult = m_resourcePackList.scanDirectory(resourcePackDir);
    if (scanResult.success()) {
        spdlog::info("Found {} resource packs", scanResult.value());
    } else {
        spdlog::warn("Failed to scan resource pack directory: {}", scanResult.error().toString());
    }

    // 3. 从设置加载资源包配置
    m_resourcePackList.loadFromSettings(m_settings.resourcePacks);

    // 4. 添加启用的资源包（外部资源包优先级高于内置）
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        } else {
            spdlog::info("Added resource pack: {}", pack->name());
        }
    }

    // 5. 加载所有资源（如果有资源包）
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->loadAllResources();
        if (loadResult.failed()) {
            spdlog::warn("Failed to load resources: {}", loadResult.error().toString());
        } else {
            spdlog::info("Loaded {} resource packs", m_resourceManager->resourcePackCount());
        }

        // 6. 构建纹理图集
        auto atlasResult = m_resourceManager->buildTextureAtlas();
        if (atlasResult.failed()) {
            spdlog::warn("Failed to build texture atlas: {}", atlasResult.error().toString());
        } else {
            spdlog::info("Built texture atlas: {}x{}, {} textures",
                atlasResult.value().width,
                atlasResult.value().height,
                atlasResult.value().regions.size());
        }
    } else {
        spdlog::info("No resource packs found, using default resources (missing model)");
    }

    // 7. 初始化 BlockModelCache（即使没有资源包也要初始化，使用缺失模型）
    if (m_modelCache.initialize(*m_resourceManager)) {
        spdlog::info("Block model cache initialized with {} appearances", m_modelCache.cachedAppearanceCount());
        // 设置 ChunkMesher 使用 BlockModelCache
        ChunkMesher::setModelCache(&m_modelCache);
    } else {
        spdlog::warn("Failed to initialize block model cache");
    }

    // 8. 设置资源包变更回调
    m_resourcePackList.onChange([this]() {
        spdlog::info("Resource packs changed, reloading...");
        reloadResources();
        if (m_audioService) {
            m_audioService->reloadSoundDefinitions();
        }
    });

    return Result<void>::ok();
}

void ClientApplication::reloadResources()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "ReloadResources");

    if (!m_resourceManager) {
        return;
    }

    // 清除资源管理器
    m_resourceManager->clearResourcePacks();

    // 首先添加内置资源包
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
    }

    // 添加项目内置资源包（提供基础纹理、模型等）
    // 内置包位于可执行文件旁的 resources/data/minecraft/
    addBuiltinFolderPackIfPresent(*m_resourceManager, m_gameDirectory.builtinPackDir());

    // 重新添加启用的资源包
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        }
    }

    // 重新加载资源
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->reload();
        if (loadResult.failed()) {
            spdlog::error("Failed to reload resources: {}", loadResult.error().toString());
            return;
        }

        // 重新构建纹理图集
        auto atlasResult = m_resourceManager->buildTextureAtlas();
        if (atlasResult.failed()) {
            spdlog::error("Failed to rebuild texture atlas: {}", atlasResult.error().toString());
            return;
        }

        // 重建模型缓存
        if (m_modelCache.rebuild(*m_resourceManager)) {
            spdlog::info("Reloaded resources: {} appearances cached", m_modelCache.cachedAppearanceCount());
        }

        if (m_renderer) {
            auto atlasUpdateResult = m_renderer->updateTextureAtlas(atlasResult.value());
            if (atlasUpdateResult.failed()) {
                spdlog::error(
                    "Failed to update renderer texture atlas after reload: {}", atlasUpdateResult.error().toString());
            }

            auto reloadCloudResult = m_renderer->reloadCloudTexture(m_resourceManager.get());
            if (reloadCloudResult.failed()) {
                spdlog::warn(
                    "Failed to reload cloud texture after resource reload: {}", reloadCloudResult.error().toString());
            }

            auto reloadFireResult = m_renderer->reloadFireTexture(m_resourceManager.get());
            if (reloadFireResult.failed()) {
                spdlog::warn(
                    "Failed to reload fire texture after resource reload: {}", reloadFireResult.error().toString());
            }
        }

        m_world.forEachChunk([](const ChunkId&, ClientChunk& chunk) { chunk.needsMeshUpdate = true; });
        spdlog::info("Marked loaded chunks dirty after resource reload");
    }
}

} // namespace mc::client
