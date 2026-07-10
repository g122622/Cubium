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

#include "client/application/features/ClientApplicationHelpers.hpp"

#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/firstperson/FirstPersonRenderer.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiSpriteMappings.hpp"
#include "client/renderer/trident/gui/GuiSpriteRegistry.hpp"
#include "client/renderer/trident/gui/GuiTextureLoader.hpp"
#include "client/renderer/trident/weather/WeatherRenderer.hpp"
#include "client/renderer/util/GpuInfo.hpp"
#include "client/skin/ClientSkinManager.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/CrosshairWidget.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/widgets/TitleWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/world/block/dispense/DispenseItemBehaviorRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <memory>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

using namespace mc::trace;

namespace mc::client {

void ClientApplication::initializeCoreRegistries()
{
    // 初始化方块注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeVanillaBlocks");
        VanillaBlocks::initialize();
        spdlog::info("Vanilla blocks initialized");
    }

    // 初始化物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeVanillaItems");
        Items::initialize();
        spdlog::info("Vanilla items initialized");
    }

    // 初始化唱片机歌曲注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeJukeboxSongs");
        JukeboxSongs::initialize();
        spdlog::info("Jukebox songs initialized");
    }

    // 注册实体类型
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "RegisterVanillaEntities");
        entity::VanillaEntities::registerAll();
        spdlog::info("Entity types registered");
    }

    // 初始化实体类型标签（必须在所有实体类型注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeEntityTypeTags");
        EntityTypeTags::initialize();
        spdlog::info("Entity type tags initialized");
    }

    // 初始化伤害类型标签（用于狼铠吸收判定、伤害分类等，客户端预测也需要）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeDamageTypeTags");
        DamageTypeTags::initialize();
        spdlog::info("Damage type tags initialized");
    }

    // 初始化方块物品注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeVanillaBlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        spdlog::info("Block items initialized");
    }

    // 初始化物品标签（必须在所有物品注册后）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeItemTags");
        item::tag::ItemTags::initialize();
        spdlog::info("Item tags initialized");
    }

    // 初始化发射器行为注册表
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeDispenseBehaviors");
        blocks::DispenseItemBehaviorRegistry::instance().initDefaultBehaviors();
        spdlog::info("Dispense item behaviors initialized");
    }
}

Result<void> ClientApplication::initializeWindowAndInput()
{
    // 初始化按键绑定
    m_settings.initializeKeyBindings();
    spdlog::info("Key bindings initialized");

    // 创建窗口
    WindowConfig windowConfig;

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "CreateWindow");

        windowConfig.width = 1280;
        windowConfig.height = 720;
        windowConfig.title = "Cubium";
        windowConfig.fullscreen = m_settings.fullscreen.get();
        windowConfig.vsync = m_settings.vsync.get();

        auto windowResult = m_window.create(windowConfig);
        if (windowResult.failed()) {
            spdlog::error("Failed to create window: {}", windowResult.error().toString());
            return windowResult.error();
        }
    }

    // 初始化输入管理器
    m_input.initialize(m_window.handle());

    // 设置按键绑定
    setupInputBindings();

    // 设置设置变更回调
    setupSettingCallbacks();

    // 设置窗口大小变化回调
    m_window.setResizeCallback(
        [](i32 width, i32 height, void* userData) {
            auto* app = static_cast<ClientApplication*>(userData);
            spdlog::info("Window resized: {}x{}", width, height);
            if (app && app->m_renderer) {
                auto result = app->m_renderer->onResize(static_cast<u32>(width), static_cast<u32>(height));
                if (result.failed()) {
                    spdlog::error("Failed to handle resize: {}", result.error().toString());
                }
                app->m_camera.setAspectRatio(static_cast<f32>(width) / static_cast<f32>(height));
                app->applyGuiScale();
            }
        },
        this);

    return Result<void>::ok();
}

Result<void> ClientApplication::initializeRenderer()
{
    // 初始化Trident渲染引擎
    spdlog::info("Initializing Trident renderer...");
    m_renderer = std::make_unique<renderer::trident::TridentEngine>();

    renderer::api::RenderEngineConfig rendererConfig;
    rendererConfig.appName = "Cubium";
    rendererConfig.enableValidation = true; // Debug模式启用验证层
    rendererConfig.enableVSync = m_settings.vsync.get();
    rendererConfig.enableAntiAliasing = m_settings.antiAliasing.get();
    rendererConfig.msaaSamples = static_cast<u32>(m_settings.antiAliasing.get() ? 4 : 1);
    rendererConfig.initialWindowWidth = static_cast<u32>(m_window.width());
    rendererConfig.initialWindowHeight = static_cast<u32>(m_window.height());

    auto rendererResult = m_renderer->initialize(m_window.handle(), rendererConfig);
    if (rendererResult.failed()) {
        spdlog::error("Failed to initialize renderer: {}", rendererResult.error().toString());
        m_window.destroy();
        return rendererResult.error();
    }

    // 从 settings 同步运行时渲染参数
    m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());
    m_renderer->setLandFogDensity(m_settings.fogDensity.get());
    m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(m_settings.clouds.get()));

    // 设置相机
    setupCamera();

    // 将相机设置给渲染器
    m_renderer->setCamera(&m_camera);

    // 更新渲染器纹理图集（使用 ResourceManager 构建的纹理）
    if (m_resourceManager) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "UpdateRendererTextureAtlas");

        spdlog::info("ResourceManager exists, atlas built: {}", m_resourceManager->isAtlasBuilt());
        if (m_resourceManager->isAtlasBuilt()) {
            const auto& atlasResult = m_resourceManager->atlasResult();
            spdlog::info("Atlas pixels size: {}, width: {}, height: {}",
                atlasResult.pixels.size(),
                atlasResult.width,
                atlasResult.height);
            if (!atlasResult.pixels.empty()) {
                spdlog::info("Updating renderer texture atlas...");
                auto atlasUpdateResult = m_renderer->updateTextureAtlas(atlasResult);
                if (atlasUpdateResult.failed()) {
                    spdlog::error("Failed to update texture atlas: {}", atlasUpdateResult.error().toString());
                } else {
                    spdlog::info("Renderer texture atlas updated from resource pack");
                }
            } else {
                spdlog::warn("Atlas pixels empty, skipping renderer update");
            }
        } else {
            spdlog::warn("Atlas not built, skipping renderer update");
        }
    } else {
        spdlog::warn("ResourceManager is null, skipping texture atlas update");
    }

    // 初始化子渲染器
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers");

        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::SkyRenderer");
            auto skyInitResult = m_renderer->initializeSkyRenderer();
            if (skyInitResult.failed()) {
                spdlog::warn("Failed to initialize sky renderer: {}", skyInitResult.error().toString());
            }
        }

        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::GuiRenderer");
            auto guiInitResult = m_renderer->initializeGuiRenderer();
            if (guiInitResult.failed()) {
                spdlog::warn("Failed to initialize GUI renderer: {}", guiInitResult.error().toString());
            }
        }

        // 初始化 GUI 纹理管理器（用于背包屏幕等容器GUI）
        if (m_renderer->isGuiRendererInitialized()) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::GuiTextureManager");

            spdlog::info("Initializing GUI texture manager...");
            m_guiTextureManager = std::make_unique<renderer::trident::gui::GuiTextureManager>();
            auto textureMgrInit = m_guiTextureManager->initialize(m_renderer->device(),
                m_renderer->physicalDevice(),
                m_renderer->commandPool(),
                m_renderer->graphicsQueue(),
                m_resourceManager.get());

            if (textureMgrInit.success()) {
                // 加载背包纹理
                auto loadResult = m_guiTextureManager->loadInventoryTexture();
                if (loadResult.failed()) {
                    spdlog::warn("Failed to load inventory texture: {}", loadResult.error().toString());
                }

                // 加载工作台纹理
                auto craftingLoadResult = m_guiTextureManager->loadCraftingTableTexture();
                if (craftingLoadResult.failed()) {
                    spdlog::warn("Failed to load crafting table texture: {}", craftingLoadResult.error().toString());
                }

                // 加载熔炉纹理
                auto furnaceLoadResult = m_guiTextureManager->loadFurnaceTexture();
                if (furnaceLoadResult.failed()) {
                    spdlog::warn("Failed to load furnace texture: {}", furnaceLoadResult.error().toString());
                }

                // 注册到 GuiRenderer
                auto registerResult = m_guiTextureManager->registerToRenderer(m_renderer->guiRenderer());
                if (registerResult.success()) {
                    spdlog::info("GUI texture manager initialized with {} atlas slots", registerResult.value());
                } else {
                    spdlog::warn("Failed to register GUI texture manager: {}", registerResult.error().toString());
                }
            } else {
                spdlog::warn("Failed to initialize GUI texture manager: {}", textureMgrInit.error().toString());
                m_guiTextureManager.reset();
            }
        }

        // 实体渲染器必须先初始化（创建 EntityPipeline）
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::EntityRenderer");
            auto entityInitResult = m_renderer->initializeEntityRenderer();
            if (entityInitResult.failed()) {
                spdlog::warn("Failed to initialize entity renderer: {}", entityInitResult.error().toString());
            }
        }

        // 实体纹理图集在 EntityPipeline 创建后初始化
        if (m_resourceManager) {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::EntityTextureAtlas");
            spdlog::info("Initializing entity texture atlas...");
            auto entityAtlasResult = m_renderer->initializeEntityTextureAtlas(m_resourceManager.get());
            if (entityAtlasResult.failed()) {
                spdlog::warn("Failed to initialize entity texture atlas: {}", entityAtlasResult.error().toString());
            }
        }

        if (m_resourceManager) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::ItemRenderer");
            auto itemInitResult = m_renderer->initializeItemRenderer(m_resourceManager.get());
            if (itemInitResult.failed()) {
                spdlog::warn("Failed to initialize item renderer: {}", itemInitResult.error().toString());
            }
        }

        // 初始化雾效果管理器
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::FogManager");
            auto fogInitResult = m_renderer->initializeFogManager();
            if (fogInitResult.failed()) {
                spdlog::warn("Failed to initialize fog manager: {}", fogInitResult.error().toString());
            }
        }

        // 初始化云渲染器
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::CloudRenderer");
            auto cloudInitResult = m_renderer->initializeCloudRenderer(m_resourceManager.get());
            if (cloudInitResult.failed()) {
                spdlog::warn("Failed to initialize cloud renderer: {}", cloudInitResult.error().toString());
            }
        }

        // 初始化粒子管理器
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::ParticleManager");
            auto particleInitResult = m_renderer->initializeParticleManager();
            if (particleInitResult.failed()) {
                spdlog::warn("Failed to initialize particle manager: {}", particleInitResult.error().toString());
            }
        }

        // 初始化天气渲染器
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::WeatherRenderer");
            auto weatherInitResult = m_renderer->initializeWeatherRenderer();
            if (weatherInitResult.failed()) {
                spdlog::warn("Failed to initialize weather renderer: {}", weatherInitResult.error().toString());
            } else {
                // 设置图形模式（Fancy/Fast）
                const bool isFancy = m_settings.graphics.get() == static_cast<u8>(GraphicsMode::Fancy);
                m_renderer->weatherRenderer().setFancyGraphics(isFancy);
            }
        }

        // 初始化破坏进度渲染器
        {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::BreakProgressRenderer");
            auto breakProgressInitResult = m_renderer->initializeBreakProgressRenderer(m_resourceManager.get());
            if (breakProgressInitResult.failed()) {
                spdlog::warn(
                    "Failed to initialize break progress renderer: {}", breakProgressInitResult.error().toString());
            }
        }

        // 初始化第一人称手部渲染器
        {
            MC_TRACE_SCOPED_EVENT(
                TraceEvents.Client.Initialization, "InitializeTridentSubRenderers::FirstPersonRenderer");
            auto firstPersonInitResult = m_renderer->initializeFirstPersonRenderer();
            if (firstPersonInitResult.failed()) {
                spdlog::warn(
                    "Failed to initialize first person renderer: {}", firstPersonInitResult.error().toString());
            }
        }
    }

    return Result<void>::ok();
}

void ClientApplication::initializeUi()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "initializeUi");

    if (m_renderer->isGuiRendererInitialized()) {
        auto* guiFont = m_renderer->guiRenderer().font();
        if (guiFont == nullptr) {
            spdlog::error("Failed to get GUI font for KageroEngine");
        } else {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "CreateKageroEngine");

            // icons.png: 心形、饥饿、盔甲、经验条等
            m_iconsAtlas = std::make_unique<renderer::trident::gui::GuiSpriteAtlas>();
            auto iconsAtlasResult = m_iconsAtlas->initialize(m_renderer->context()->device(),
                m_renderer->context()->physicalDevice(),
                m_renderer->commandPool(),
                m_renderer->graphicsQueue());
            if (iconsAtlasResult.failed()) {
                spdlog::warn("Failed to initialize icons atlas: {}. Using fallback colors.",
                    iconsAtlasResult.error().toString());
                m_iconsAtlas.reset();
            }

            // widgets.png: 快捷栏、按钮等
            m_widgetsAtlas = std::make_unique<renderer::trident::gui::GuiSpriteAtlas>();
            auto widgetsAtlasResult = m_widgetsAtlas->initialize(m_renderer->context()->device(),
                m_renderer->context()->physicalDevice(),
                m_renderer->commandPool(),
                m_renderer->graphicsQueue());
            if (widgetsAtlasResult.failed()) {
                spdlog::warn("Failed to initialize widgets atlas: {}. Using fallback colors.",
                    widgetsAtlasResult.error().toString());
                m_widgetsAtlas.reset();
            }

            // 准备纹理加载器
            renderer::trident::gui::GuiTextureLoader textureLoader;
            bool hasTextureLoader = false;

            // 从资源包加载纹理
            if (m_resourceManager && m_resourceManager->resourcePackCount() > 0) {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadGuiTexturesFromResourcePacks");
                spdlog::info("[GUI] ResourceManager has {} resource packs", m_resourceManager->resourcePackCount());

                // 添加启用的资源包到加载器
                auto enabledPacks = m_resourcePackList.getEnabledPacks();
                spdlog::info("[GUI] PackRepository has {} enabled packs", enabledPacks.size());
                for (const auto& pack : enabledPacks) {
                    if (pack) {
                        spdlog::info("[GUI] Adding enabled resource pack: {}", pack->name());
                        textureLoader.addResourcePack(pack);
                        hasTextureLoader = true;
                    }
                }

                // 如果没有从设置启用的资源包，使用资源管理器中的资源包
                if (!hasTextureLoader) {
                    spdlog::info("[GUI] No enabled packs from settings, using ResourceManager packs");
                    // 获取资源管理器中所有资源包
                    for (size_t i = 0; i < m_resourceManager->resourcePackCount(); ++i) {
                        auto* pack = m_resourceManager->getResourcePack(i);
                        if (pack) {
                            spdlog::info("[GUI] Adding ResourceManager pack [{}]: {}", i, pack->name());
                            // 注意：这里使用空删除器，因为资源包生命周期由ResourceManager管理
                            textureLoader.addResourcePack(
                                std::shared_ptr<mc::IResourcePack>(pack, [](mc::IResourcePack*) {}));
                            hasTextureLoader = true;
                        }
                    }
                }
            } else {
                spdlog::info("[GUI] ResourceManager is null or has no resource packs");
            }

            // 加载纹理并注册精灵
            // 关键顺序：先加载纹理（设置正确的图集尺寸），再注册精灵（计算正确的UV）
            if (hasTextureLoader) {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadGuiTextures");
                spdlog::info("[GUI] TextureLoader has {} resource packs", textureLoader.resourcePackCount());

                // 加载 HUD/Icons 图集
                // 优先使用 MC 1.21+ 独立精灵格式，回退到旧版 icons.png 单体图集
                if (m_iconsAtlas) {
                    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadIconsAtlas");
                    spdlog::info("[GUI] Loading HUD/Icons atlas...");

                    bool iconsLoaded = false;

                    // 尝试 MC 1.21+ 独立精灵格式（从 textures/gui/sprites/hud/ 拼合图集）
                    auto spriteResult =
                        textureLoader.buildSpriteAtlas(*m_iconsAtlas, renderer::trident::gui::HUD_SPRITE_MAPPINGS);
                    if (spriteResult.success() && spriteResult.value() > 0) {
                        iconsLoaded = true;
                        spdlog::info("[GUI] Loaded HUD atlas from sprite files ({} sprites, {}x{})",
                            spriteResult.value(),
                            m_iconsAtlas->atlasWidth(),
                            m_iconsAtlas->atlasHeight());
                    } else {
                        spdlog::info("[GUI] Sprite atlas build failed or empty, trying legacy icons.png...");
                    }

                    // 回退到旧版 icons.png 单体图集
                    if (!iconsLoaded) {
                        auto loadResult =
                            textureLoader.loadGuiTexture(*m_iconsAtlas, "minecraft:textures/gui/icons.png");
                        if (loadResult.failed()) {
                            spdlog::warn("[GUI] Failed to load icons.png: {}. Using default textures.",
                                loadResult.error().toString());
                            (void)m_iconsAtlas->loadDefaultTextures();
                        } else {
                            spdlog::info("[GUI] Loaded icons.png from resource pack ({}x{})",
                                m_iconsAtlas->atlasWidth(),
                                m_iconsAtlas->atlasHeight());
                        }
                        // 旧版单体图集使用硬编码UV注册精灵
                        renderer::trident::gui::GuiSpriteRegistry::registerIconsSprites(*m_iconsAtlas);
                    }

                    spdlog::info("[GUI] Icons atlas: {} sprites registered, texture={}",
                        m_iconsAtlas->spriteCount(),
                        m_iconsAtlas->hasTexture() ? "yes" : "no");

                    // 注册图集到 GuiRenderer 并设置槽位
                    auto iconsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "icons", m_iconsAtlas->imageView(), m_iconsAtlas->sampler());
                    if (iconsSlotResult.success()) {
                        m_iconsAtlas->setAtlasSlot(static_cast<u8>(iconsSlotResult.value()));
                        spdlog::info("[GUI] Icons atlas registered at slot {}", iconsSlotResult.value());
                    } else {
                        spdlog::warn("[GUI] Failed to register icons atlas: {}", iconsSlotResult.error().toString());
                    }
                }

                // 加载 Widget 图集
                // 优先使用 MC 1.21+ 独立精灵格式，回退到旧版 widgets.png 单体图集
                if (m_widgetsAtlas) {
                    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadWidgetsAtlas");
                    spdlog::info("[GUI] Loading Widget atlas...");

                    bool widgetsLoaded = false;

                    // 尝试 MC 1.21+ 独立精灵格式（从 textures/gui/sprites/widget/ 拼合图集）
                    auto spriteResult =
                        textureLoader.buildSpriteAtlas(*m_widgetsAtlas, renderer::trident::gui::WIDGET_SPRITE_MAPPINGS);
                    if (spriteResult.success() && spriteResult.value() > 0) {
                        widgetsLoaded = true;
                        spdlog::info("[GUI] Loaded Widget atlas from sprite files ({} sprites, {}x{})",
                            spriteResult.value(),
                            m_widgetsAtlas->atlasWidth(),
                            m_widgetsAtlas->atlasHeight());
                    } else {
                        spdlog::info("[GUI] Sprite atlas build failed or empty, trying legacy widgets.png...");
                    }

                    // 回退到旧版 widgets.png 单体图集
                    if (!widgetsLoaded) {
                        auto loadResult =
                            textureLoader.loadGuiTexture(*m_widgetsAtlas, "minecraft:textures/gui/widgets.png");
                        if (loadResult.failed()) {
                            spdlog::warn("[GUI] Failed to load widgets.png: {}. Using default textures.",
                                loadResult.error().toString());
                            (void)m_widgetsAtlas->loadDefaultTextures();
                        } else {
                            spdlog::info("[GUI] Loaded widgets.png from resource pack ({}x{})",
                                m_widgetsAtlas->atlasWidth(),
                                m_widgetsAtlas->atlasHeight());
                        }
                        // 旧版单体图集使用硬编码UV注册精灵
                        renderer::trident::gui::GuiSpriteRegistry::registerWidgetsSprites(*m_widgetsAtlas);
                    }

                    spdlog::info("[GUI] Widgets atlas: {} sprites registered, texture={}",
                        m_widgetsAtlas->spriteCount(),
                        m_widgetsAtlas->hasTexture() ? "yes" : "no");

                    // 注册图集到 GuiRenderer 并设置槽位
                    auto widgetsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "widgets", m_widgetsAtlas->imageView(), m_widgetsAtlas->sampler());
                    if (widgetsSlotResult.success()) {
                        m_widgetsAtlas->setAtlasSlot(static_cast<u8>(widgetsSlotResult.value()));
                        spdlog::info("[GUI] Widgets atlas registered at slot {}", widgetsSlotResult.value());
                    } else {
                        spdlog::warn(
                            "[GUI] Failed to register widgets atlas: {}", widgetsSlotResult.error().toString());
                    }
                }
            } else {
                // 无资源包，使用默认纹理
                if (m_iconsAtlas) {
                    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadDefaultIconsTexture");

                    (void)m_iconsAtlas->loadDefaultTextures();
                    // 使用默认256x256尺寸注册精灵
                    renderer::trident::gui::GuiSpriteRegistry::registerIconsSprites(*m_iconsAtlas);
                    // 注册图集到 GuiRenderer
                    auto iconsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "icons", m_iconsAtlas->imageView(), m_iconsAtlas->sampler());
                    if (iconsSlotResult.success()) {
                        m_iconsAtlas->setAtlasSlot(static_cast<u8>(iconsSlotResult.value()));
                    }
                }
                if (m_widgetsAtlas) {
                    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "LoadDefaultWidgetsTexture");

                    (void)m_widgetsAtlas->loadDefaultTextures();
                    // 使用默认256x256尺寸注册精灵
                    renderer::trident::gui::GuiSpriteRegistry::registerWidgetsSprites(*m_widgetsAtlas);
                    // 注册图集到 GuiRenderer
                    auto widgetsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "widgets", m_widgetsAtlas->imageView(), m_widgetsAtlas->sampler());
                    if (widgetsSlotResult.success()) {
                        m_widgetsAtlas->setAtlasSlot(static_cast<u8>(widgetsSlotResult.value()));
                    }
                }
            }

            // 创建 TridentCanvas
            m_canvas = std::make_unique<ui::TridentCanvas>(m_renderer->guiRenderer(), *guiFont);
            m_canvas->resize(m_window.width(), m_window.height());

            // 创建 KageroEngine
            m_kageroEngine = std::make_unique<ui::kagero::KageroEngine>();
            ui::kagero::KageroConfig kageroConfig;
            kageroConfig.screenWidth = m_window.width();
            kageroConfig.screenHeight = m_window.height();
            auto kageroInitResult = m_kageroEngine->initialize(*m_canvas, kageroConfig);
            if (kageroInitResult.failed()) {
                spdlog::error("Failed to initialize KageroEngine: {}", kageroInitResult.error().toString());
            } else {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ConfigureKageroLayers");

                spdlog::info("KageroEngine initialized");

                // 层 Z=0: 准星
                auto crosshairWidget = std::make_unique<ui::minecraft::widgets::CrosshairWidget>();
                m_crosshairLayerId = m_kageroEngine->addLayer(std::move(crosshairWidget), 0);

                // 层 Z=10: HUD
                auto hudWidget = std::make_unique<ui::minecraft::widgets::HudWidget>();
                hudWidget->setGuiRenderer(&m_renderer->guiRenderer());
                hudWidget->setItemRenderer(&m_renderer->itemRenderer());
                if (m_iconsAtlas) {
                    hudWidget->setIconsAtlas(m_iconsAtlas.get());
                }
                if (m_widgetsAtlas) {
                    hudWidget->setWidgetsAtlas(m_widgetsAtlas.get());
                }
                if (m_player) {
                    hudWidget->setPlayer(m_player.get());
                }
                m_hudLayerId = m_kageroEngine->addLayer(std::move(hudWidget), 10);

                auto targetInfoWidget = std::make_unique<ui::minecraft::targetinfo::TargetInfoWidget>();
                m_targetInfoLayerId = m_kageroEngine->addLayer(std::move(targetInfoWidget), 15);

                // 层 Z=18: 标题显示（title 命令）
                auto titleWidget = std::make_unique<ui::minecraft::widgets::TitleWidget>();
                titleWidget->setFont(guiFont);
                m_titleLayerId = m_kageroEngine->addLayer(std::move(titleWidget), 18);

                // 层 Z=20: 聊天框
                auto chatWidget = std::make_unique<ui::minecraft::widgets::ChatWidget>();
                chatWidget->setFont(guiFont);
                chatWidget->setGuiRenderer(&m_renderer->guiRenderer());
                chatWidget->setCommandManager(m_commandManager.get());
                chatWidget->setCommandCallback([this](const std::string& input) { handleChatCommand(input); });
                // Actionbar 回调需要在 chatWidget 和 titleWidget 都加入引擎后设置，
                // 因为回调需要通过 layer ID 访问 titleWidget（见下方 setActionbarCallback）
                m_chatLayerId = m_kageroEngine->addLayer(std::move(chatWidget), 20);

                // 设置动作栏回调：当 ChatWidget 收到 Actionbar/GameInfo 消息时，
                // 路由到 TitleWidget 在动作栏区域显示（与 MC Java 一致）
                auto* chatWidgetPtr =
                    static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId));
                if (chatWidgetPtr != nullptr) {
                    chatWidgetPtr->setActionbarCallback([this](const std::string& text) {
                        if (m_kageroEngine) {
                            auto* titleWidget = static_cast<ui::minecraft::widgets::TitleWidget*>(
                                m_kageroEngine->getLayer(m_titleLayerId));
                            if (titleWidget != nullptr) {
                                titleWidget->setActionbar(text);
                            }
                        }
                    });
                }

                // 层 Z=30: Screen 栈
                auto screenStackWidget = std::make_unique<ui::minecraft::widgets::ScreenStackWidget>();
                screenStackWidget->setGuiRenderer(&m_renderer->guiRenderer());

                // 设置 ScreenManager 后端
                ScreenManager::instance().setScreenStackWidget(screenStackWidget.get());

                m_screenStackLayerId = m_kageroEngine->addLayer(std::move(screenStackWidget), 30);

                // 层 Z=100: 调试屏幕
                auto debugWidget = std::make_unique<ui::minecraft::DebugScreenWidget>();
                debugWidget->setTextWidthCallback([this](const std::string& text) -> f32 {
                    const f64 guiScale = static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1));
                    return static_cast<f32>(m_renderer->guiRenderer().getTextWidth(text) / guiScale);
                });
                debugWidget->setLineHeight(static_cast<i32>(m_renderer->guiRenderer().getFontHeight() /
                                               static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))) +
                    2);
                debugWidget->setCamera(&m_camera);
                debugWidget->setWorld(&m_world);
                debugWidget->setEntityManager(&m_world.entityManager());
                debugWidget->setNetworkClient(m_networkClient.get());
                debugWidget->setDimensionManager(&m_dimensionManager);
                debugWidget->setRenderDistance(m_settings.renderDistance.get());
                if (m_player) {
                    debugWidget->setPlayer(m_player.get());
                }

                // 设置GPU信息
                {
                    auto* context = m_renderer->context();
                    if (context != nullptr) {
                        DebugGpuInfo gpuInfo = getGpuInfo(context->deviceProperties(), context->memoryProperties());

                        debugWidget->setGpuInfo(gpuInfo);
                        debugWidget->setVersion("Cubium 0.1.0");
                        debugWidget->setRendererInfo(gpuInfo.name);
                    }
                }
                m_debugScreenLayerId = m_kageroEngine->addLayer(std::move(debugWidget), 100);

                spdlog::info("KageroEngine layers configured: crosshair={}, hud={}, targetInfo={}, title={}, chat={}, "
                             "screenStack={}, debug={}",
                    m_crosshairLayerId,
                    m_hudLayerId,
                    m_targetInfoLayerId,
                    m_titleLayerId,
                    m_chatLayerId,
                    m_screenStackLayerId,
                    m_debugScreenLayerId);
            }
        }

        applyGuiScale();

        // 设置字符输入回调 - 通过 KageroEngine 分发
        m_input.setCharCallback([this](u32 codepoint) {
            if (m_kageroEngine && m_kageroEngine->handleChar(codepoint)) {
                return;
            }
        });

        // 设置键盘事件回调 - 通过 KageroEngine 分发
        // 返回 true 表示事件已消费，阻止后续 action 触发
        m_input.setKeyEventCallback([this](i32 key, i32 action, i32 mods) -> bool {
            // F3 切换调试屏幕
            if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
                m_debugScreenVisible = !m_debugScreenVisible;
                if (m_kageroEngine) {
                    m_kageroEngine->setLayerVisible(m_debugScreenLayerId, m_debugScreenVisible);
                }
                return true;
            }

            if (m_kageroEngine && m_kageroEngine->handleKey(key, 0, action, mods)) {
                return true;
            }

            // 游戏输入处理
            if (action == GLFW_PRESS && !ScreenManager::instance().hasScreen()) {
                mc::client::application::features::captureMouseAfterScreens(m_input, m_mouseCaptured);
            }

            return false;
        });

        // 设置GUI渲染回调 - 完全通过 KageroEngine
        m_renderer->setGuiRenderCallback([this]() {
            if (m_kageroEngine) {
                m_canvas->beginFrame();
                m_kageroEngine->render();
                m_canvas->endFrame();
            }
        });
    }
}

Result<void> ClientApplication::initializeShell(const ClientLaunchParams& params)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeShell");

    // 初始化核心注册表
    initializeCoreRegistries();

    // 初始化资源系统
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeResources");
        spdlog::info("Initializing resource system...");
        auto resourceResult = initializeResources();
        if (resourceResult.failed()) {
            spdlog::error("Failed to initialize resource system: {}. Using default rendering.",
                resourceResult.error().toString());
            // 资源初始化失败不是致命错误
        }
    }

    // 初始化音频系统
    {
        auto audioResult = initializeAudio();
        if (audioResult.failed()) {
            spdlog::error("Failed to initialize audio system: {}", audioResult.error().toString());
            // 音频初始化失败不是致命错误
        }
    }

    // 初始化窗口和输入
    auto windowResult = initializeWindowAndInput();
    if (windowResult.failed()) {
        return windowResult.error();
    }

    // 初始化渲染器
    auto rendererResult = initializeRenderer();
    if (rendererResult.failed()) {
        return rendererResult.error();
    }

    // 初始化 UI
    initializeUi();

    spdlog::info("[Shell] Shell initialization complete");
    return Result<void>::ok();
}

} // namespace mc::client
