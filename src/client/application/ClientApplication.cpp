#include "ClientApplication.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/resource/VanillaResources.hpp"
#include "common/entity/core/VanillaEntities.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/perfetto/PerfettoManager.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/PlatformInfo.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/EntityRendererManager.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiSpriteAtlas.hpp"
#include "client/renderer/trident/gui/GuiSpriteRegistry.hpp"
#include "client/renderer/trident/gui/GuiTextureLoader.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/renderer/trident/firstperson/FirstPersonRenderer.hpp"
#include "client/renderer/util/GpuInfo.hpp"
#include "client/resource/ResourceManager.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/minecraft/widgets/CrosshairWidget.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>

namespace mc::client {

namespace {

template <typename Menu>
void applyContainerContents(Menu* menu, const std::vector<ItemStack>& items) {
    if (menu == nullptr) {
        return;
    }

    const i32 slotCount = std::min(menu->getSlotCount(), static_cast<i32>(items.size()));
    for (i32 slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        Slot* slot = menu->getSlot(slotIndex);
        if (slot != nullptr) {
            slot->set(items[slotIndex]);
        }
    }
}

template <typename Menu>
void applyContainerSlot(Menu* menu, i32 slotIndex, const ItemStack& item) {
    if (menu == nullptr) {
        return;
    }

    Slot* slot = menu->getSlot(slotIndex);
    if (slot != nullptr) {
        slot->set(item);
    }
}

void releaseMouseForScreen(InputManager& input, bool& mouseCaptured) {
    if (mouseCaptured) {
        input.setMouseLocked(false);
        mouseCaptured = false;
    }
}

void captureMouseAfterScreens(InputManager& input, bool& mouseCaptured) {
    if (!mouseCaptured) {
        input.setMouseLocked(true);
        mouseCaptured = true;
    }
}

[[nodiscard]] f32 calculateBlockBreakingDelta(const Player& player, const BlockState& state)
{
    const f32 hardness = state.hardness();
    if (hardness < 0.0f) {
        return 0.0f;
    }

    if (hardness == 0.0f) {
        return 1.0f;
    }

    const ItemStack heldItem = player.inventory().getSelectedStack();
    const f32 destroySpeed = std::max(heldItem.getDestroySpeed(state), 1.0f);
    const bool canHarvest = heldItem.isEmpty() ? true : heldItem.canHarvestBlock(state);
    const f32 divisor = canHarvest ? 30.0f : 100.0f;
    return destroySpeed / hardness / divisor;
}

/**
 * @brief ClientWorld 到 IBlockReader 的轻量适配器
 *
 * 射线检测接口当前要求 IBlockReader，
 * 而 ClientWorld 实现的是 ICollisionWorld（方法签名兼容）。
 */
class ClientWorldBlockReader final : public mc::IBlockReader {
public:
    explicit ClientWorldBlockReader(const ClientWorld& world)
        : m_world(world)
    {
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override
    {
        return m_world.isWithinWorldBounds(x, y, z);
    }

    // IWorld 接口实现 - 委托到 ClientWorld
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override;
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override;
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB& box, const Entity* except) const override;
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB& box, const Entity* except) const override;
    [[nodiscard]] PhysicsEngine* physicsEngine() override;
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity* except) const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3& pos, f32 range, const Entity* except) const override;
    [[nodiscard]] DimensionId dimension() const override;
    [[nodiscard]] u64 seed() const override;
    [[nodiscard]] u64 currentTick() const override;
    [[nodiscard]] i64 dayTime() const override;
    [[nodiscard]] bool isHardcore() const override;
    [[nodiscard]] i32 difficulty() const override;

private:
    const ClientWorld& m_world;
};

// IWorld 接口实现
bool ClientWorldBlockReader::setBlock(i32, i32, i32, const BlockState*) { return false; }

const fluid::FluidState* ClientWorldBlockReader::getFluidState(i32, i32, i32) const {
    return fluid::Fluid::getFluidState(0);
}

const ChunkData* ClientWorldBlockReader::getChunk(ChunkCoord, ChunkCoord) const { return nullptr; }
bool ClientWorldBlockReader::hasChunk(ChunkCoord, ChunkCoord) const { return false; }
i32 ClientWorldBlockReader::getHeight(i32, i32) const { return 64; }
u8 ClientWorldBlockReader::getBlockLight(i32, i32, i32) const { return 15; }
u8 ClientWorldBlockReader::getSkyLight(i32, i32, i32) const { return 15; }
bool ClientWorldBlockReader::hasBlockCollision(const AxisAlignedBB&) const { return false; }
std::vector<AxisAlignedBB> ClientWorldBlockReader::getBlockCollisions(const AxisAlignedBB&) const { return {}; }
bool ClientWorldBlockReader::hasEntityCollision(const AxisAlignedBB&, const Entity*) const { return false; }
std::vector<AxisAlignedBB> ClientWorldBlockReader::getEntityCollisions(const AxisAlignedBB&, const Entity*) const { return {}; }
PhysicsEngine* ClientWorldBlockReader::physicsEngine() { return nullptr; }
const PhysicsEngine* ClientWorldBlockReader::physicsEngine() const { return nullptr; }
std::vector<Entity*> ClientWorldBlockReader::getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const { return {}; }
std::vector<Entity*> ClientWorldBlockReader::getEntitiesInRange(const Vector3&, f32, const Entity*) const { return {}; }
DimensionId ClientWorldBlockReader::dimension() const { return DimensionId(0); }
u64 ClientWorldBlockReader::seed() const { return 0; }
u64 ClientWorldBlockReader::currentTick() const { return 0; }
i64 ClientWorldBlockReader::dayTime() const { return 0; }
bool ClientWorldBlockReader::isHardcore() const { return false; }
i32 ClientWorldBlockReader::difficulty() const { return 0; }

} // namespace

ClientApplication::ClientApplication() = default;

ClientApplication::~ClientApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ClientApplication::initialize(const ClientLaunchParams& params)
{
    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Client already initialized");
    }

    // 加载设置
    String settingsPath = params.settingsPath.value_or(
        ClientSettings::getSettingsPath("minecraft-reborn").string());
    auto settingsResult = loadSettings(settingsPath);
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
                     settingsPath, settingsResult.error().toString());
    }

    // 应用命令行覆盖
    if (params.fullscreen.has_value()) {
        m_settings.fullscreen.set(*params.fullscreen);
    }
    if (params.serverAddress.has_value()) {
        m_settings.serverAddress.set(*params.serverAddress);
    }
    if (params.serverPort.has_value()) {
        m_settings.serverPort.set(*params.serverPort);
    }
    if (params.username.has_value()) {
        m_settings.username.set(*params.username);
    }

    // 应用设置到系统
    applySettings();

    // 设置日志级别
    const auto& logLevel = m_settings.logLevel.get();
    if (logLevel == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (logLevel == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (logLevel == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (logLevel == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (logLevel == "error") {
        spdlog::set_level(spdlog::level::err);
    } else {
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Minecraft Reborn Client ===");
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing client...");

    // 初始化性能追踪
    mc::perfetto::TraceConfig traceConfig;
    traceConfig.outputPath = "client_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().startTracing();

    // 设置进程和主线程名称
    mc::perfetto::PerfettoManager::instance().setProcessName("MinecraftClient");
    mc::perfetto::PerfettoManager::instance().setThreadName("ClientMainThread");
    spdlog::info("Perfetto tracing initialized");

    // 初始化方块注册表
    VanillaBlocks::initialize();
    spdlog::info("Vanilla blocks initialized");

    Items::initialize();
    spdlog::info("Vanilla items initialized");

    // 注册实体类型
    entity::VanillaEntities::registerAll();
    spdlog::info("Entity types registered");

    // 初始化方块物品注册表
    BlockItemRegistry::instance().initializeVanillaBlockItems();
    spdlog::info("Block items initialized");

    // 初始化资源系统
    spdlog::info("Initializing resource system...");
    auto resourceResult = initializeResources();
    if (resourceResult.failed()) {
        spdlog::warn("Failed to initialize resource system: {}. Using default rendering.",
                     resourceResult.error().toString());
    }

    // 初始化按键绑定
    m_settings.initializeKeyBindings();
    spdlog::info("Key bindings initialized");

    // 创建窗口
    WindowConfig windowConfig;
    windowConfig.width = 1280;
    windowConfig.height = 720;
    windowConfig.title = "Minecraft Reborn";
    windowConfig.fullscreen = m_settings.fullscreen.get();
    windowConfig.vsync = m_settings.vsync.get();
    windowConfig.samples = m_settings.antiAliasing.get() ? 4 : 1;

    if (m_settings.antiAliasing.get()) {
        spdlog::info("Anti-aliasing enabled (MSAA x{})", windowConfig.samples);
    }

    auto windowResult = m_window.create(windowConfig);
    if (windowResult.failed()) {
        spdlog::error("Failed to create window: {}", windowResult.error().toString());
        return windowResult.error();
    }

    // 初始化输入管理器
    m_input.initialize(m_window.handle());

    // 设置按键绑定
    setupInputBindings();

    // 设置设置变更回调
    setupSettingCallbacks();

    // 设置窗口大小变化回调
    m_window.setResizeCallback([](i32 width, i32 height, void* userData) {
        auto* app = static_cast<ClientApplication*>(userData);
        spdlog::info("Window resized: {}x{}", width, height);
        if (app && app->m_renderer) {
            auto result = app->m_renderer->onResize(static_cast<u32>(width), static_cast<u32>(height));
            if (result.failed()) {
                spdlog::error("Failed to handle resize: {}", result.error().toString());
            }
            // 更新相机宽高比
            app->m_camera.setAspectRatio(static_cast<f32>(width) / static_cast<f32>(height));
            // 更新 KageroEngine 尺寸
            if (app->m_kageroEngine) {
                app->m_kageroEngine->resize(width, height);
            }
            if (app->m_canvas) {
                app->m_canvas->resize(width, height);
            }
        }
    }, this);

    // 初始化Trident渲染引擎
    spdlog::info("Initializing Trident renderer...");
    m_renderer = std::make_unique<renderer::trident::TridentEngine>();

    renderer::api::RenderEngineConfig rendererConfig;
    rendererConfig.appName = "Minecraft Reborn";
    rendererConfig.enableValidation = true; // Debug模式启用验证层
    rendererConfig.enableVSync = m_settings.vsync.get();
    rendererConfig.enableAntiAliasing = m_settings.antiAliasing.get();
    rendererConfig.msaaSamples = static_cast<u32>(windowConfig.samples);
    rendererConfig.initialWindowWidth = static_cast<u32>(windowConfig.width);
    rendererConfig.initialWindowHeight = static_cast<u32>(windowConfig.height);

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
        spdlog::info("ResourceManager exists, atlas built: {}", m_resourceManager->isAtlasBuilt());
        if (m_resourceManager->isAtlasBuilt()) {
            const auto& atlasResult = m_resourceManager->atlasResult();
            spdlog::info("Atlas pixels size: {}, width: {}, height: {}",
                        atlasResult.pixels.size(), atlasResult.width, atlasResult.height);
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

    // 初始化 Trident 子渲染器
    {
        auto skyInitResult = m_renderer->initializeSkyRenderer();
        if (skyInitResult.failed()) {
            spdlog::warn("Failed to initialize sky renderer: {}", skyInitResult.error().toString());
        }

        auto guiInitResult = m_renderer->initializeGuiRenderer();
        if (guiInitResult.failed()) {
            spdlog::warn("Failed to initialize GUI renderer: {}", guiInitResult.error().toString());
        }

        // 初始化 GUI 纹理管理器（用于背包屏幕等容器GUI）
        if (m_renderer->isGuiRendererInitialized()) {
            spdlog::info("Initializing GUI texture manager...");
            m_guiTextureManager = std::make_unique<renderer::trident::gui::GuiTextureManager>();
            auto textureMgrInit = m_guiTextureManager->initialize(
                m_renderer->device(),
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

                // 注册到 GuiRenderer
                auto registerResult = m_guiTextureManager->registerToRenderer(m_renderer->guiRenderer());
                if (registerResult.success()) {
                    spdlog::info("GUI texture manager initialized with atlas slot {}", registerResult.value());
                } else {
                    spdlog::warn("Failed to register GUI texture manager: {}", registerResult.error().toString());
                }
            } else {
                spdlog::warn("Failed to initialize GUI texture manager: {}", textureMgrInit.error().toString());
                m_guiTextureManager.reset();
            }
        }

        // 实体渲染器必须先初始化（创建 EntityPipeline）
        auto entityInitResult = m_renderer->initializeEntityRenderer();
        if (entityInitResult.failed()) {
            spdlog::warn("Failed to initialize entity renderer: {}", entityInitResult.error().toString());
        }

        // 实体纹理图集在 EntityPipeline 创建后初始化
        if (m_resourceManager) {
            spdlog::info("Initializing entity texture atlas...");
            auto entityAtlasResult = m_renderer->initializeEntityTextureAtlas(m_resourceManager.get());
            if (entityAtlasResult.failed()) {
                spdlog::warn("Failed to initialize entity texture atlas: {}", entityAtlasResult.error().toString());
            }
        }

        if (m_resourceManager) {
            auto itemInitResult = m_renderer->initializeItemRenderer(m_resourceManager.get());
            if (itemInitResult.failed()) {
                spdlog::warn("Failed to initialize item renderer: {}", itemInitResult.error().toString());
            }
        }

        // 初始化雾效果管理器
        auto fogInitResult = m_renderer->initializeFogManager();
        if (fogInitResult.failed()) {
            spdlog::warn("Failed to initialize fog manager: {}", fogInitResult.error().toString());
        }

        // 初始化云渲染器
        auto cloudInitResult = m_renderer->initializeCloudRenderer(m_resourceManager.get());
        if (cloudInitResult.failed()) {
            spdlog::warn("Failed to initialize cloud renderer: {}", cloudInitResult.error().toString());
        }

        // 初始化粒子管理器
        auto particleInitResult = m_renderer->initializeParticleManager();
        if (particleInitResult.failed()) {
            spdlog::warn("Failed to initialize particle manager: {}", particleInitResult.error().toString());
        }

        // 初始化天气渲染器
        auto weatherInitResult = m_renderer->initializeWeatherRenderer();
        if (weatherInitResult.failed()) {
            spdlog::warn("Failed to initialize weather renderer: {}", weatherInitResult.error().toString());
        }

        // 初始化破坏进度渲染器
        auto breakProgressInitResult = m_renderer->initializeBreakProgressRenderer(m_resourceManager.get());
        if (breakProgressInitResult.failed()) {
            spdlog::warn("Failed to initialize break progress renderer: {}", breakProgressInitResult.error().toString());
        }

        // 初始化第一人称手部渲染器
        auto firstPersonInitResult = m_renderer->initializeFirstPersonRenderer();
        if (firstPersonInitResult.failed()) {
            spdlog::warn("Failed to initialize first person renderer: {}", firstPersonInitResult.error().toString());
        }
    }

    // 初始化声音系统
    spdlog::info("Initializing sound system...");
    m_soundHandler = std::make_unique<sound::SoundHandler>(m_resourcePackList);
    m_soundEngine = std::make_unique<sound::SoundEngine>(*m_soundHandler, m_settings);

    auto soundInitResult = m_soundEngine->initialize();
    if (soundInitResult.failed()) {
        spdlog::warn("Failed to initialize sound engine: {}. Audio will be disabled.",
                     soundInitResult.error().toString());
        m_soundEngine.reset();
        m_soundHandler.reset();
    } else {
        // 添加环境音效处理器
        m_biomeAmbientHandler = std::make_unique<sound::BiomeAmbientHandler>();
        m_soundEngine->addAmbientHandler(std::move(m_biomeAmbientHandler));

        m_underwaterAmbientHandler = std::make_unique<sound::UnderwaterAmbientHandler>();
        m_soundEngine->addAmbientHandler(std::move(m_underwaterAmbientHandler));

        spdlog::info("Sound system initialized successfully");
    }

    // 启动内置服务端
    if (!params.skipIntegratedServer) {
        spdlog::info("Starting integrated server...");

        m_integratedServer = std::make_unique<server::IntegratedServer>();
        server::IntegratedServerConfig serverConfig;
        serverConfig.seed = 12345;
        serverConfig.viewDistance = m_settings.renderDistance.get();
        serverConfig.defaultGameMode = GameMode::Creative;

        auto serverResult = m_integratedServer->initialize(serverConfig);
        if (serverResult.failed()) {
            spdlog::error("Failed to initialize integrated server: {}", serverResult.error().toString());
            m_renderer->destroy();
            m_window.destroy();
            return serverResult.error();
        }

        // 初始化网络客户端
        m_networkClient = std::make_unique<NetworkClient>();
        setupNetworkCallbacks();

        NetworkClientConfig clientConfig;
        clientConfig.username = m_settings.username.get();
        auto clientResult = m_networkClient->connectLocal(m_integratedServer->getClientEndpoint(), clientConfig);
        if (clientResult.failed()) {
            spdlog::error("Failed to connect to integrated server: {}", clientResult.error().toString());
            m_integratedServer->stop();
            m_renderer->destroy();
            m_window.destroy();
            return clientResult.error();
        }

        m_useIntegratedServer = true;
        spdlog::info("Connected to integrated server");
    } else {
        m_useIntegratedServer = false;
    }

    // 初始化世界
    spdlog::info("Initializing world...");
    auto worldResult = m_world.initialize(12345); // 使用固定种子
    if (worldResult.failed()) {
        spdlog::error("Failed to initialize world: {}", worldResult.error().toString());
        if (m_integratedServer) {
            m_integratedServer->stop();
        }
        m_renderer->destroy();
        m_window.destroy();
        return worldResult.error();
    }

    // 初始化网格构建系统（执行器 + 独立调度器）
    spdlog::info("Initializing mesh build system...");
    MeshSchedulerConfig schedulerConfig;
    schedulerConfig.maxDispatchedTaskCount = std::max(32, m_settings.renderDistance.get() * 12);
    schedulerConfig.reprioritizeIntervalFrames = 6;
    schedulerConfig.cameraMoveThreshold = 2.0f;
    schedulerConfig.cameraDirectionDotThreshold = 0.96f;
    schedulerConfig.behindCancelDotThreshold = -0.35f;
    schedulerConfig.behindCancelDistanceChunks = static_cast<f32>(std::max(6, m_settings.renderDistance.get() / 2));
    m_world.initializeMeshSystem(-1, schedulerConfig);

    // 设置区块卸载回调，通知 ChunkRenderer 释放 GPU 缓冲区
    m_world.setChunkUnloadCallback([this](const ChunkId& chunkId) {
        if (m_renderer && m_renderer->isChunkRendererInitialized()) {
            m_renderer->chunkRenderer().removeChunk(chunkId);
        }
    });

    // 设置实体渲染回调
    m_renderer->setEntityRenderCallback([this](VkCommandBuffer cmd, f64 partialTick) {
        m_world.entityManager().forEachEntity([&](client::ClientEntity& entity) {
            m_renderer->entityRendererManager().renderWithPipeline(cmd, entity, partialTick);
        });
    });

    // 设置第一人称手部渲染回调
    m_renderer->setFirstPersonRenderCallback([this](VkCommandBuffer cmd, VkDescriptorSet cameraSet, f64 partialTick) {
        if (!m_renderer || !m_player || !m_renderer->isFirstPersonRendererInitialized()) {
            return;
        }

        renderer::trident::firstperson::FirstPersonRenderer::RenderContext renderContext;
        renderContext.player = m_player.get();
        renderContext.partialTick = partialTick;

        m_renderer->firstPersonRenderer().render(cmd, cameraSet, renderContext);
    });

    // 初始化方块碰撞注册表
    spdlog::info("Initializing block collision registry...");

    // 创建物理引擎
    m_physicsEngine = std::make_unique<PhysicsEngine>(m_world);

    // 创建玩家实体
    m_player = std::make_unique<Player>(static_cast<EntityId>(1), m_settings.username.get());
    m_player->setPosition(8.0, 50.0, 8.0);  // 初始位置在地面上方
    m_player->setPhysicsEngine(m_physicsEngine.get());
    // 默认创造模式并启用飞行
    m_player->setGameMode(GameMode::Creative);
    m_player->setCreativeModeInventory();
    m_player->abilities().flying = true;
    spdlog::info("Player created at (8, 50, 8)");

    spdlog::info("Client initialized successfully");
    spdlog::info("Window: {}x{}", m_window.width(), m_window.height());
    spdlog::info("Controls: WASD to move, Space to jump/fly up, Shift to sneak/fly down, mouse to look");
    spdlog::info("Press F to toggle flying, F3 to toggle debug screen, ALT to toggle mouse capture");

    // 初始化 Kagero UI 引擎
    if (m_renderer->isGuiRendererInitialized()) {
        auto* guiFont = m_renderer->guiRenderer().font();
        if (guiFont == nullptr) {
            spdlog::error("Failed to get GUI font for KageroEngine");
        } else {
            // 初始化 GUI 精灵图集（双图集架构）
            // 重要：纹理加载顺序 - 先初始化图集，再加载纹理（设置正确尺寸），最后注册精灵

            // icons.png: 心形、饥饿、盔甲、经验条等
            m_iconsAtlas = std::make_unique<renderer::trident::gui::GuiSpriteAtlas>();
            auto iconsAtlasResult = m_iconsAtlas->initialize(
                m_renderer->context()->device(),
                m_renderer->context()->physicalDevice(),
                m_renderer->commandPool(),
                m_renderer->graphicsQueue()
            );
            if (iconsAtlasResult.failed()) {
                spdlog::warn("Failed to initialize icons atlas: {}. Using fallback colors.",
                             iconsAtlasResult.error().toString());
                m_iconsAtlas.reset();
            }

            // widgets.png: 快捷栏、按钮等
            m_widgetsAtlas = std::make_unique<renderer::trident::gui::GuiSpriteAtlas>();
            auto widgetsAtlasResult = m_widgetsAtlas->initialize(
                m_renderer->context()->device(),
                m_renderer->context()->physicalDevice(),
                m_renderer->commandPool(),
                m_renderer->graphicsQueue()
            );
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
                spdlog::info("[GUI] ResourceManager has {} resource packs", m_resourceManager->resourcePackCount());

                // 添加启用的资源包到加载器
                auto enabledPacks = m_resourcePackList.getEnabledPacks();
                spdlog::info("[GUI] ResourcePackList has {} enabled packs", enabledPacks.size());
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
                spdlog::info("[GUI] TextureLoader has {} resource packs", textureLoader.resourcePackCount());

                // 加载 icons.png 到 iconsAtlas
                if (m_iconsAtlas) {
                    spdlog::info("[GUI] Loading icons.png to iconsAtlas...");
                    auto loadResult = textureLoader.loadGuiTexture(*m_iconsAtlas, "minecraft:textures/gui/icons.png");
                    if (loadResult.failed()) {
                        spdlog::warn("[GUI] Failed to load icons.png: {}. Using default textures.", loadResult.error().toString());
                        (void)m_iconsAtlas->loadDefaultTextures();
                    } else {
                        spdlog::info("[GUI] Loaded icons.png from resource pack ({}x{})",
                                    m_iconsAtlas->atlasWidth(), m_iconsAtlas->atlasHeight());
                    }
                    // 纹理加载后注册精灵（使用正确的图集尺寸计算UV）
                    renderer::trident::gui::GuiSpriteRegistry::registerIconsSprites(*m_iconsAtlas);
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

                // 加载 widgets.png 到 widgetsAtlas
                if (m_widgetsAtlas) {
                    spdlog::info("[GUI] Loading widgets.png to widgetsAtlas...");
                    auto loadResult = textureLoader.loadGuiTexture(*m_widgetsAtlas, "minecraft:textures/gui/widgets.png");
                    if (loadResult.failed()) {
                        spdlog::warn("[GUI] Failed to load widgets.png: {}. Using default textures.", loadResult.error().toString());
                        (void)m_widgetsAtlas->loadDefaultTextures();
                    } else {
                        spdlog::info("[GUI] Loaded widgets.png from resource pack ({}x{})",
                                    m_widgetsAtlas->atlasWidth(), m_widgetsAtlas->atlasHeight());
                    }
                    // 纹理加载后注册精灵（使用正确的图集尺寸计算UV）
                    renderer::trident::gui::GuiSpriteRegistry::registerWidgetsSprites(*m_widgetsAtlas);
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
                        spdlog::warn("[GUI] Failed to register widgets atlas: {}", widgetsSlotResult.error().toString());
                    }
                }
            } else {
                // 无资源包，使用默认纹理
                if (m_iconsAtlas) {
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
            m_canvas = std::make_unique<ui::TridentCanvas>(
                m_renderer->guiRenderer(),
                *guiFont
            );
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

                // 层 Z=20: 聊天框
                auto chatWidget = std::make_unique<ui::minecraft::widgets::ChatWidget>();
                chatWidget->setFont(guiFont);
                chatWidget->setGuiRenderer(&m_renderer->guiRenderer());
                chatWidget->setCommandCallback([this](const String& input) {
                    handleChatCommand(input);
                });
                m_chatLayerId = m_kageroEngine->addLayer(std::move(chatWidget), 20);

                // 层 Z=30: Screen 栈
                auto screenStackWidget = std::make_unique<ui::minecraft::widgets::ScreenStackWidget>();
                screenStackWidget->setGuiRenderer(&m_renderer->guiRenderer());

                // 设置 ScreenManager 后端
                ScreenManager::instance().setScreenStackWidget(screenStackWidget.get());

                m_screenStackLayerId = m_kageroEngine->addLayer(std::move(screenStackWidget), 30);

                // 层 Z=100: 调试屏幕
                auto debugWidget = std::make_unique<ui::minecraft::DebugScreenWidget>();
                debugWidget->setTextWidthCallback([this](const std::string& text) -> f32 {
                    return static_cast<f32>(m_renderer->guiRenderer().getTextWidth(text));
                });
                debugWidget->setLineHeight(static_cast<i32>(m_renderer->guiRenderer().getFontHeight()) + 2);
                debugWidget->setCamera(&m_camera);
                debugWidget->setWorld(&m_world);
                debugWidget->setEntityManager(&m_world.entityManager());
                debugWidget->setNetworkClient(m_networkClient.get());
                debugWidget->setRenderDistance(m_settings.renderDistance.get());
                if (m_player) {
                    debugWidget->setPlayer(m_player.get());
                }

                // 设置GPU信息
                {
                    auto* context = m_renderer->context();
                    if (context != nullptr) {
                        DebugGpuInfo gpuInfo = getGpuInfo(
                            context->deviceProperties(),
                            context->memoryProperties());

                        debugWidget->setGpuInfo(gpuInfo);
                        debugWidget->setVersion("Minecraft Reborn 0.1.0");
                        debugWidget->setRendererInfo(gpuInfo.name);
                    }
                }
                m_debugScreenLayerId = m_kageroEngine->addLayer(std::move(debugWidget), 100);

                spdlog::info("KageroEngine layers configured: crosshair={}, hud={}, chat={}, screenStack={}, debug={}",
                             m_crosshairLayerId, m_hudLayerId, m_chatLayerId, m_screenStackLayerId, m_debugScreenLayerId);
            }
        }

        // 设置字符输入回调 - 通过 KageroEngine 分发
        m_input.setCharCallback([this](u32 codepoint) {
            if (m_kageroEngine && m_kageroEngine->handleChar(codepoint)) {
                return;
            }
        });

        // 设置键盘事件回调 - 通过 KageroEngine 分发
        m_input.setKeyEventCallback([this](i32 key, i32 action, i32 mods) {
            // F3 切换调试屏幕
            if (key == GLFW_KEY_F3 && action == GLFW_PRESS) {
                m_debugScreenVisible = !m_debugScreenVisible;
                if (m_kageroEngine) {
                    m_kageroEngine->setLayerVisible(m_debugScreenLayerId, m_debugScreenVisible);
                }
                return;
            }

            if (m_kageroEngine && m_kageroEngine->handleKey(key, 0, action, mods)) {
                return;
            }

            // 游戏输入处理
            if (action == GLFW_PRESS && !ScreenManager::instance().hasScreen()) {
                captureMouseAfterScreens(m_input, m_mouseCaptured);
            }
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

    m_initialized = true;
    return Result<void>::ok();
}

Result<void> ClientApplication::run()
{
    if (!m_initialized) {
        return Error(ErrorCode::InvalidArgument, "Client not initialized");
    }

    if (m_running) {
        return Error(ErrorCode::AlreadyExists, "Client already running");
    }

    spdlog::info("Starting client main loop...");
    m_running = true;

    try {
        mainLoop();
    } catch (const std::exception& e) {
        spdlog::critical("Client crashed: {}", e.what());
        m_running = false;
        return Error(ErrorCode::Unknown, e.what());
    }

    return Result<void>::ok();
}

void ClientApplication::stop()
{
    if (!m_running) {
        return;
    }

    spdlog::info("Stopping client...");
    m_running = false;
}

void ClientApplication::mainLoop()
{
    using clock = std::chrono::steady_clock;

    spdlog::info("Client is now running!");
    spdlog::info("Press ESC to exit");

    // 初始捕获鼠标
    toggleMouseCapture();

    m_lastFrameTime = glfwGetTime();

    while (m_running && !m_window.shouldClose()) {
        MC_TRACE_EVENT("rendering.frame", "Frame");

        const auto frameStart = clock::now();

        // 计算帧时间
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 处理事件
        {
            MC_TRACE_EVENT("rendering.frame", "HandleEvents");
            handleEvents();
        }

        // 更新
        {
            MC_TRACE_EVENT("rendering.frame", "Update");
            update(deltaTime);
        }

        // 渲染
        {
            MC_TRACE_EVENT("rendering.frame", "Render");
            render();
        }

        // 清理本帧的瞬时输入状态
        m_input.endFrame();

        // 帧计数
        ++m_frameCount;

#if MC_ENABLE_TRACING
        // 追踪 FPS
        const f32 safeDeltaTime = std::max(deltaTime, 0.0001f);
        const i32 fps = static_cast<i32>(1.0f / safeDeltaTime);
        if (fps < 1000) { // 过滤掉异常值
            MC_TRACE_COUNTER("rendering.frame", "FPS", fps);
        }

        // 追踪内存信息
        MC_TRACE_COUNTER("memory", "ProcessMemory", static_cast<int64_t>(util::PlatformInfo::getProcessMemoryMB()));
#endif

        // 帧率限制（0=不限制）
        const i32 fpsLimit = m_settings.framerateLimit.get();
        if (fpsLimit > 0) {
            const auto minFrameDuration = std::chrono::duration<f64>(1.0 / static_cast<f64>(fpsLimit));
            const auto frameElapsed = clock::now() - frameStart;
            if (frameElapsed < minFrameDuration) {
                MC_TRACE_EVENT("rendering.frame", "FrameRateLimitSleep");
                std::this_thread::sleep_for(minFrameDuration - frameElapsed);
            }
        }
    }

    shutdown();
}

void ClientApplication::handleEvents()
{
    m_window.pollEvents();
    m_input.update();

    // 处理聊天框键盘输入（优先于游戏输入）
    // 检查聊天框是否打开
    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;
    if (chatWidget && chatWidget->isOpen()) {
        // 聊天框打开时，ESC 关闭聊天框
        if (m_input.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            chatWidget->close();
            m_input.setMouseLocked(true);
            m_mouseCaptured = true;
        }
        return;
    }

    // 处理 Screen 栈事件
    auto* screenStack = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId)) : nullptr;
    if (screenStack && screenStack->hasScreen()) {
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleClick(
                static_cast<i32>(m_input.mouseX()),
                static_cast<i32>(m_input.mouseY()),
                GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleClick(
                static_cast<i32>(m_input.mouseX()),
                static_cast<i32>(m_input.mouseY()),
                GLFW_MOUSE_BUTTON_RIGHT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleRelease(
                static_cast<i32>(m_input.mouseX()),
                static_cast<i32>(m_input.mouseY()),
                GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleRelease(
                static_cast<i32>(m_input.mouseX()),
                static_cast<i32>(m_input.mouseY()),
                GLFW_MOUSE_BUTTON_RIGHT);
        }

        if (!screenStack->hasScreen()) {
            captureMouseAfterScreens(m_input, m_mouseCaptured);
        }
        return;
    }

    // 检查ALT键切换鼠标捕获
    if (m_input.isKeyJustPressed(GLFW_KEY_LEFT_ALT) ||
        m_input.isKeyJustPressed(GLFW_KEY_RIGHT_ALT)) {
        toggleMouseCapture();
    }

    // T 键打开聊天框
    if (m_input.isKeyJustPressed(GLFW_KEY_T)) {
        if (chatWidget) {
            chatWidget->open(false);
            if (m_mouseCaptured) {
                m_input.setMouseLocked(false);
                m_mouseCaptured = false;
            }
        }
        return;
    }

    // / 键打开命令框
    if (m_input.isKeyJustPressed(GLFW_KEY_SLASH)) {
        if (chatWidget) {
            chatWidget->open(true);
            if (m_mouseCaptured) {
                m_input.setMouseLocked(false);
                m_mouseCaptured = false;
            }
        }
        return;
    }

    if (m_input.isKeyJustPressed(GLFW_KEY_E) && m_player) {
        releaseMouseForScreen(m_input, m_mouseCaptured);

        // 创建背包屏幕并设置渲染器
        auto inventoryScreen = std::make_unique<InventoryCraftingScreen>(
            std::make_unique<InventoryCraftingMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory()));

        // 设置渲染器
        if (m_renderer && m_renderer->isGuiRendererInitialized()) {
            inventoryScreen->setRenderers(
                &m_renderer->guiRenderer(),
                m_guiTextureManager.get(),
                m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
            inventoryScreen->setScreenSize(static_cast<i32>(m_window.width()), static_cast<i32>(m_window.height()));
        }

        ScreenManager::instance().openScreen(std::move(inventoryScreen));
        return;
    }

    // 飞行模式切换（F键）
    if (m_input.isKeyJustPressed(GLFW_KEY_F)) {
        if (m_player && m_player->abilities().canFly) {
            m_player->toggleFlying();
        }
    }

    // 传递键盘输入到玩家和鼠标控制
    if (m_mouseCaptured && m_player) {
        // 鼠标视角控制 - 更新玩家朝向（使用设置中的灵敏度）
        // InputManager 返回 f64 (GLFW)，转换为 f32 用于内部计算
        f32 sensitivity = m_settings.mouseSensitivity.get() * 0.2f;
        f32 deltaYaw = static_cast<f32>(m_input.mouseDeltaX()) * sensitivity;
        f32 deltaPitch = static_cast<f32>(m_input.mouseDeltaY()) * sensitivity;
        m_player->rotate(deltaYaw, -deltaPitch);  // pitch方向相反

        // 收集移动输入
        f32 forward = 0.0f;
        f32 strafe = 0.0f;
        bool jumping = false;
        bool sneaking = false;

        if (m_input.isKeyPressed(GLFW_KEY_W)) forward += 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_S)) forward -= 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_A)) strafe -= 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_D)) strafe += 1.0f;
        if (m_input.isKeyPressed(GLFW_KEY_SPACE)) jumping = true;
        if (m_input.isKeyPressed(GLFW_KEY_LEFT_SHIFT)) sneaking = true;

        // 传递输入给玩家
        m_player->handleMovementInput(forward, strafe, jumping, sneaking);

        // 滚轮选择物品栏槽位（scrollDeltaY 返回 f64）
        const f64 scrollDelta = m_input.scrollDeltaY();
        if (scrollDelta != 0.0) {
            i32 selectedSlot = m_player->inventory().getSelectedSlot();
            i32 delta = scrollDelta > 0.0 ? -1 : 1;
            selectedSlot = (selectedSlot + delta + PlayerInventory::HOTBAR_SIZE) % PlayerInventory::HOTBAR_SIZE;
            m_player->inventory().setSelectedSlot(selectedSlot);
            if (m_networkClient && m_networkClient->isLoggedIn()) {
                m_networkClient->sendHotbarSelect(selectedSlot);
            }
        }

    }
}

void ClientApplication::update(f32 deltaTime)
{
    // 更新网络客户端（处理服务端数据包）
    if (m_networkClient) {
        m_networkClient->poll();
    }

    // 更新破坏进度管理器
    {
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().tick(deltaTime, m_world.gameTime());
    }

    if (m_renderer && m_renderer->isFirstPersonRendererInitialized()) {
        m_renderer->firstPersonRenderer().tick();
    }

    // 更新玩家物理
    if (m_player) {
        // 应用物理（重力、碰撞检测）
        m_player->updatePhysics();

        // 同步相机位置到玩家眼睛位置
        m_camera.setPosition(
            static_cast<f32>(m_player->x()),
            static_cast<f32>(m_player->y() + m_player->eyeHeight()),
            static_cast<f32>(m_player->z())
        );
        m_camera.setYaw(m_player->yaw());
        m_camera.setPitch(m_player->pitch());
        m_camera.update(deltaTime);

        // 更新声音系统听者位置
        if (m_soundEngine) {
            m_soundEngine->updateListener(
                m_camera.position(),
                m_camera.forward(),
                m_camera.up()
            );
        }
    } else {
        // 后备：更新相机控制器（这会调用 Camera::update 更新矩阵）
        m_cameraController.update(deltaTime);
    }

    // 发送玩家位置到服务端（限制频率）
    if (m_networkClient && m_networkClient->isLoggedIn() && m_player) {
        m_positionSendAccumulator += deltaTime;
        if (m_positionSendAccumulator >= POSITION_SEND_INTERVAL) {
            m_positionSendAccumulator = 0.0f;
            sendPlayerPosition();
        }
    }

    // 更新 ScreenStackWidget 的 partialTick 和鼠标位置（用于 IScreen::render）
    auto* screenStack = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId)) : nullptr;
    if (screenStack) {
        screenStack->setPartialTick(0.0f);  // TODO: 使用实际的 partialTick
        screenStack->setMousePosition(
            static_cast<i32>(m_input.mouseX()),
            static_cast<i32>(m_input.mouseY())
        );
    }

    // 更新 KageroEngine
    if (m_kageroEngine) {
        m_kageroEngine->update(deltaTime);
    }

    // 更新调试屏幕数据
    auto* debugWidget = m_kageroEngine ?
        static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId)) : nullptr;

    // 执行射线检测
    if (m_player && m_mouseCaptured) {
        // 获取玩家眼睛位置
        glm::vec3 eyePos = m_camera.position();

        // 获取视线方向
        glm::vec3 forward = m_camera.forward();

        // 创建射线
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);
        mc::Ray ray(origin, direction);

        // 执行射线检测（创造模式使用更远的距离）
        mc::RaycastContext context(ray, 5.0f);  // 生存模式5格
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mc::raycastBlocks(context, blockReader);

        // 更新调试屏幕的目标方块
        if (debugWidget) {
            debugWidget->setTargetBlock(&m_raycastResult);
        }
    } else {
        // 没有玩家时清除目标方块
        if (debugWidget) {
            debugWidget->setTargetBlock(nullptr);
        }
    }

    handleBlockInteractionInput(deltaTime);

    // 处理方块放置输入
    handleBlockPlacementInput(deltaTime);

    MeshSchedulerViewState meshViewState;
    meshViewState.cameraPosition = glm::vec3(m_camera.position());
    meshViewState.cameraForward = glm::vec3(m_camera.forward());
    meshViewState.viewProjectionMatrix = m_camera.viewProjectionMatrix();
    meshViewState.renderDistanceChunks = m_settings.renderDistance.get();
    meshViewState.minBuildHeight = m_world.getMinBuildHeight();
    meshViewState.maxBuildHeight = m_world.getMaxBuildHeight();

    m_world.update(meshViewState);

    // 更新客户端实体（每tick调用）
    m_world.entityManager().tick();

    // 更新声音系统
    if (m_soundEngine) {
        // 检查是否暂停（游戏暂停时不更新声音）
        bool isPaused = !m_mouseCaptured;  // 鼠标未捕获时认为游戏暂停
        m_soundEngine->tick(isPaused);
    }

    // 更新实体动画状态（用于渲染插值）
    constexpr f32 partialTick = 0.0f;  // TODO: 从主循环获取实际的部分tick
    m_world.entityManager().updateAnimations(partialTick);

    // 处理异步网格构建结果
    m_world.processMeshBuildResults(4);  // 每帧最多处理 4 个网格

    // 同步时间到渲染器（驱动天空、太阳、月亮、星空变化）
    // 客户端每帧平滑推进时间，同时在收到服务端同步时纠正
    if (m_renderer) {
        constexpr i64 DAY_LENGTH_TICKS = 24000;

        // 每帧推进时间（无论是否有服务端同步）
        // 这确保天空、太阳、月亮在每帧平滑变化
        m_renderTickAccumulator += deltaTime * 20.0f;
        while (m_renderTickAccumulator >= 1.0f) {
            m_renderTickAccumulator -= 1.0f;
            ++m_renderGameTime;
            m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;
        }

        // 当有服务端同步时，逐渐纠正到服务端时间（避免跳变）
        if (m_hasServerTimeSync) {
            const i64 serverDayTime = m_world.dayTime();
            const i64 serverGameTime = m_world.gameTime();

            // 计算时间差（处理 dayTime 循环）
            i64 dayTimeDiff = serverDayTime - m_renderDayTime;
            if (dayTimeDiff > DAY_LENGTH_TICKS / 2) {
                dayTimeDiff -= DAY_LENGTH_TICKS;
            } else if (dayTimeDiff < -DAY_LENGTH_TICKS / 2) {
                dayTimeDiff += DAY_LENGTH_TICKS;
            }

            // 平滑纠正：每帧纠正差值的 1%，避免跳变（TODO:根据用户设定的帧率调整纠正率。CORRECTION_RATE = 1 / 用户设定FPS）
            constexpr f32 CORRECTION_RATE = 0.01f;
            const i64 correction = static_cast<i64>(dayTimeDiff * CORRECTION_RATE);
            if (correction != 0) {
                m_renderDayTime = (m_renderDayTime + correction + DAY_LENGTH_TICKS) % DAY_LENGTH_TICKS;
            }

            // gameTime 同步纠正
            i64 gameTimeDiff = serverGameTime - m_renderGameTime;
            m_renderGameTime += static_cast<i64>(gameTimeDiff * CORRECTION_RATE);
        }

        m_renderer->updateTime(m_renderDayTime, m_renderGameTime, m_renderTickAccumulator);

        // 更新天气状态到渲染器
        m_renderer->updateWeather(
            m_world.weather().rainStrength(m_renderTickAccumulator),
            m_world.weather().thunderStrength(m_renderTickAccumulator)
        );

        // 更新液体状态（用于水下/岩浆雾效果）
        if (m_player) {
            bool inWater = m_player->isInWater();
            bool inLava = m_player->isInLava();
            u32 waterFogColor = world::biome::BiomeEffects::DEFAULT_WATER_FOG_COLOR;

            // 获取当前生物群系的水下雾颜色
            if (inWater) {
                const auto* biome = m_world.getBiomeAtBlock(
                    static_cast<i32>(std::floor(m_player->x())),
                    static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
                    static_cast<i32>(std::floor(m_player->z()))
                );
                if (biome) {
                    waterFogColor = biome->waterFogColor();
                }
            }

            m_renderer->updateLiquidState(inWater, inLava, waterFogColor);
        }
    }

    // 上传网格到 GPU（只处理已完成异步构建的网格）
    if (m_renderer->isChunkRendererInitialized()) {
        auto& chunkRenderer = m_renderer->chunkRenderer();
        m_world.forEachDirtyMesh([&chunkRenderer](const ChunkId& id, ClientChunk& chunk) {
            // 两层都为空时，清理 GPU 缓冲并结束本次更新。
            if (chunk.solidMesh.empty() && chunk.transparentMesh.empty()) {
                chunkRenderer.removeChunk(id);
                chunk.needsMeshUpdate = false;
                return;
            }

            // 上传双层网格到 GPU
            auto result = chunkRenderer.updateChunk(id, chunk.solidMesh, chunk.transparentMesh);
            if (result.success()) {
                chunk.needsMeshUpdate = false;

                // 上传成功后释放 CPU 侧网格缓存，避免与 GPU 数据重复占用内存。
                chunk.solidMesh.clear();
                chunk.transparentMesh.clear();
                std::vector<Vertex>().swap(chunk.solidMesh.vertices);
                std::vector<u32>().swap(chunk.solidMesh.indices);
                std::vector<Vertex>().swap(chunk.transparentMesh.vertices);
                std::vector<u32>().swap(chunk.transparentMesh.indices);
            } else {
                spdlog::error("Failed to update chunk mesh: {}", result.error().toString());
            }
        });
    }
}

void ClientApplication::render()
{
    if (!m_renderer || m_renderer->isMinimized()) {
        return;
    }

    auto result = m_renderer->render();
    if (result.failed()) {
        spdlog::error("Render error: {}", result.error().toString());
    }
}

void ClientApplication::shutdown()
{
    spdlog::info("Shutting down client...");

    // 保存设置
    const auto savePath = m_settingsPath.empty()
        ? ClientSettings::getSettingsPath("minecraft-reborn")
        : m_settingsPath;
    auto saveResult = m_settings.saveSettings(savePath);
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 关闭声音系统
    if (m_soundEngine) {
        m_soundEngine->shutdown();
        m_soundEngine.reset();
    }
    m_soundHandler.reset();
    m_biomeAmbientHandler.reset();
    m_underwaterAmbientHandler.reset();

    // 断开网络连接
    if (m_networkClient) {
        m_networkClient->disconnect("Client shutdown");
        m_networkClient.reset();
    }

    // 停止内置服务端
    if (m_integratedServer) {
        m_integratedServer->stop();
        m_integratedServer.reset();
    }

    // 先清理依赖渲染资源的 UI/图集对象，避免在渲染器销毁后析构访问无效资源
    if (m_kageroEngine) {
        m_kageroEngine.reset();
    }
    if (m_canvas) {
        m_canvas.reset();
    }
    if (m_iconsAtlas) {
        m_iconsAtlas.reset();
    }
    if (m_widgetsAtlas) {
        m_widgetsAtlas.reset();
    }
    if (m_guiTextureManager) {
        m_guiTextureManager.reset();
    }

    // 清理渲染器
    if (m_renderer) {
        m_renderer->destroy();
        m_renderer.reset();
    }

    // 清理玩家
    m_player.reset();
    m_physicsEngine.reset();

    // 清理世界（包括关闭网格构建线程池）
    m_world.destroy();

    m_window.destroy();

    // 关闭性能追踪
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

    spdlog::info("Client stopped.");
}

// 设置相关方法

Result<void> ClientApplication::loadSettings(const String& path)
{
    m_settingsPath = std::filesystem::path(path);

    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 确保设置目录存在（使用当前实际设置路径，避免写到默认目录）
    const auto settingsDir = m_settingsPath.parent_path();
    if (!settingsDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(settingsDir, ec);
        if (ec) {
            spdlog::warn("Failed to create settings directory: {}", settingsDir.string());
        }
    }

    // 启用自动保存
    m_settings.enableAutoSave(m_settingsPath);

    spdlog::info("Client settings path: {}", m_settingsPath.string());

    return Result<void>::ok();
}

void ClientApplication::applySettings()
{
    // 设置变更回调在 setupSettingCallbacks 中设置
    // 这里应用初始设置值

    // 同步渲染器运行时参数
    if (m_renderer) {
        m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());
        m_renderer->setLandFogDensity(m_settings.fogDensity.get());
        m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(m_settings.clouds.get()));
    }

    // 应用光照模式（环境光遮蔽）
    ChunkMesher::syncFromSettings(static_cast<client::AmbientOcclusionMode>(
        m_settings.ambientOcclusion.get()));

    // 应用生物群系颜色混合半径
    ChunkMesher::setBiomeBlendRadius(m_settings.biomeBlendRadius.get());
}

void ClientApplication::setupSettingCallbacks()
{
    // 渲染距离变更
    m_settings.renderDistance.onChange([this](i32 value) {
        spdlog::info("Render distance changed to: {}", value);
        if (m_renderer) {
            m_renderer->setRenderDistanceChunks(value);
        }
        auto* debugWidget = m_kageroEngine ?
            static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId)) : nullptr;
        if (debugWidget) {
            debugWidget->setRenderDistance(value);
        }
        // 世界更新时会使用新值
    });

    // 全屏模式变更
    m_settings.fullscreen.onChange([this](bool value) {
        spdlog::info("Fullscreen changed to: {}", value);
        m_window.setFullscreen(value);
    });

    // VSync 变更
    m_settings.vsync.onChange([this](bool value) {
        spdlog::info("VSync changed to: {}", value);
        m_window.setVSync(value);
        if (m_renderer) {
            auto result = m_renderer->setVSyncEnabled(value);
            if (result.failed()) {
                spdlog::warn("Failed to apply VSync change: {}", result.error().toString());
            }
        }
    });

    // 云模式变更
    m_settings.clouds.onChange([this](u8 value) {
        spdlog::info("Cloud mode changed to: {}", static_cast<i32>(value));
        if (m_renderer) {
            m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(value));
        }
    });

    // 鼠标灵敏度变更
    m_settings.mouseSensitivity.onChange([this](f32 value) {
        spdlog::info("Mouse sensitivity changed to: {}", value);
        // 鼠标灵敏度在 handleEvents 中应用
    });

    // FOV 变更
    m_settings.fov.onChange([this](f32 value) {
        spdlog::info("FOV changed to: {}", value);
        m_camera.setFOV(value);
    });

    // 雾气密度变更
    m_settings.fogDensity.onChange([this](f32 value) {
        spdlog::info("Fog density changed to: {}", value);
        if (m_renderer) {
            m_renderer->setLandFogDensity(value);
        }
    });

    // 光照模式（环境光遮蔽）变更
    m_settings.ambientOcclusion.onChange([this](u8 value) {
        auto mode = static_cast<client::AmbientOcclusionMode>(value);
        spdlog::info("Ambient occlusion changed to: {}", static_cast<i32>(mode));
        ChunkMesher::syncFromSettings(mode);
    });

    // 生物群系颜色混合半径变更
    m_settings.biomeBlendRadius.onChange([this](i32 value) {
        spdlog::info("Biome blend radius changed to: {} ({}x{} area)",
                     value, value * 2 + 1, value * 2 + 1);
        ChunkMesher::setBiomeBlendRadius(value);
    });

    m_settings.antiAliasing.onChange([](bool enabled) {
        spdlog::info("Anti-aliasing changed to: {} (restart required)", enabled);
    });
}

// 辅助函数实现

void ClientApplication::setupInputBindings()
{
    m_input.bindKeyAction(GLFW_KEY_ESCAPE, "exit");

    m_input.bindActionCallback("exit", [this]() {
        auto* chatWidget = m_kageroEngine ?
            static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;

        if (chatWidget && chatWidget->isOpen()) {
            return;
        }

        if (ScreenManager::instance().hasScreen()) {
            ScreenManager::instance().closeScreen();
            captureMouseAfterScreens(m_input, m_mouseCaptured);
            return;
        }

        spdlog::info("Exit key pressed");
        stop();
    });
}

void ClientApplication::setupCamera()
{
    // 设置相机配置
    CameraConfig cameraConfig;
    cameraConfig.fov = m_settings.fov.get();
    cameraConfig.aspectRatio = static_cast<f32>(m_window.width()) / static_cast<f32>(m_window.height());
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 1000.0f;
    cameraConfig.moveSpeed = 10.0f;      // 移动速度
    cameraConfig.mouseSensitivity = m_settings.mouseSensitivity.get() * 0.2f; // 鼠标灵敏度

    m_camera = Camera(cameraConfig);

    // 设置初始位置（在测试区块上方）
    m_camera.setPosition(8.0f, 50.0f, 8.0f);
    m_camera.setYaw(45.0f);
    m_camera.update(0.0f);

    // 设置相机控制器
    m_cameraController.setCamera(&m_camera);
}

void ClientApplication::setupNetworkCallbacks()
{
    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess = [this](PlayerId playerId, const String& username) {
        spdlog::info("Login successful: playerId={}, username={}", playerId, username);
    };

    callbacks.onLoginFailed = [this](const String& reason) {
        spdlog::error("Login failed: {}", reason);
        stop();
    };

    callbacks.onDisconnected = [this](const String& reason) {
        spdlog::warn("Disconnected: {}", reason);
    };

    callbacks.onChunkData = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension, const std::vector<u8>& data) {
        MC_UNUSED(dimension);  // TODO: 多维度支持
        m_world.onChunkData(x, z, std::vector<u8>(data));
    };

    callbacks.onChunkUnload = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension) {
        MC_UNUSED(dimension);  // TODO: 多维度支持
        m_world.onChunkUnload(x, z);
    };

    callbacks.onTeleport = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 /*teleportId*/) {
        if (m_player) {
            // 网络协议使用 f64，内部使用 f32
            m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            m_player->setRotation(yaw, pitch);
        }
        m_camera.setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        m_camera.setYaw(yaw);
        m_camera.setPitch(pitch);
        m_camera.update(0.0f);  // 立即更新相机矩阵
        // 注意：sendTeleportConfirm 已在 NetworkClient::handleTeleport 中调用
    };

    callbacks.onBlockUpdate = [this](i32 bx, i32 by, i32 bz, u32 blockStateId) {
        m_world.setBlock(bx, by, bz, Block::getBlockState(blockStateId));
    };

    callbacks.onTimeUpdate = [this](i64 gameTime, i64 dayTime, bool daylightCycleEnabled) {
        m_world.onTimeUpdate(gameTime, dayTime, daylightCycleEnabled);
        m_renderGameTime = gameTime;
        m_renderDayTime = dayTime;
        m_renderTickAccumulator = 0.0f;
        m_hasServerTimeSync = true;
    };

    callbacks.onPlayerInventory = [this](i32 selectedSlot, const std::vector<ItemStack>& items) {
        if (!m_player) {
            return;
        }

        m_player->inventory().clear();
        const i32 maxSlots = std::min(static_cast<i32>(items.size()), PlayerInventory::TOTAL_SIZE);
        for (i32 slot = 0; slot < maxSlots; ++slot) {
            m_player->inventory().setItem(slot, items[slot]);
        }
        m_player->inventory().setSelectedSlot(selectedSlot);
    };

    callbacks.onOpenContainer = [this](const OpenContainerPacket& packet) {
        if (!m_player) {
            return;
        }

        if (static_cast<ContainerType>(packet.type()) != ContainerType::CraftingTable) {
            spdlog::debug("Unhandled open container type {}", packet.type());
            return;
        }

        auto clickSender = [this](ContainerId containerId, i32 slotIndex, i32 button, ClickAction action,
                                  const ItemStack& cursorItem) {
            if (m_networkClient) {
                m_networkClient->sendContainerClick(
                    ContainerClickPacket(containerId, slotIndex, button, action, cursorItem));
            }
        };

        auto closeSender = [this](ContainerId containerId) {
            if (m_networkClient) {
                m_networkClient->sendCloseContainer(containerId);
            }
        };

        releaseMouseForScreen(m_input, m_mouseCaptured);
        ScreenManager::instance().openScreen(
            std::make_unique<CraftingScreen>(
                std::make_unique<CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr),
                clickSender,
                closeSender));
    };

    callbacks.onContainerContent = [this](const ContainerContentPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == packet.containerId()) {
                applyContainerContents(craftingScreen->getMenu(), packet.items());
            }
        }
    };

    callbacks.onContainerSlot = [this](const ContainerSlotPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == packet.containerId()) {
                applyContainerSlot(craftingScreen->getMenu(), packet.slotIndex(), packet.item());
            }
        }
    };

    callbacks.onCloseContainer = [this](ContainerId containerId) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (auto* craftingScreen = dynamic_cast<CraftingScreen*>(screen)) {
            if (craftingScreen->getMenu() != nullptr && craftingScreen->getMenu()->getId() == containerId) {
                ScreenManager::instance().closeScreen();
                if (!ScreenManager::instance().hasScreen()) {
                    captureMouseAfterScreens(m_input, m_mouseCaptured);
                }
            }
        }
    };

    // ========== 实体事件回调 ==========
    callbacks.onSpawnMob = [this](u32 entityId, const String& typeId,
                                   f32 x, f32 y, f32 z,
                                   f32 yaw, f32 pitch, f32 headYaw) {
        auto* entity = m_world.entityManager().spawnEntity(
            static_cast<EntityId>(entityId), typeId);
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
            entity->setHeadRotation(headYaw);
            // spdlog::info("Client received SpawnMob: {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})",
            //               typeId, entityId, x, y, z);
        }
    };

    callbacks.onSpawnEntity = [this](u32 entityId, const String& typeId,
                                      f32 x, f32 y, f32 z,
                                      f32 yaw, f32 pitch,
                                      const ItemStack* itemStack) {
        auto* entity = m_world.entityManager().spawnEntity(
            static_cast<EntityId>(entityId), typeId);
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);

            // 如果是 ItemEntity，设置物品数据
            if (itemStack != nullptr && !itemStack->isEmpty()) {
                entity->setItemStack(*itemStack);
                spdlog::debug("Client received ItemEntity {} with item: {} x{}",
                              entityId, itemStack->getItem()->itemId(), itemStack->getCount());
            }
        }
    };

    callbacks.onEntityDestroy = [this](const std::vector<u32>& entityIds) {
        for (u32 id : entityIds) {
            m_world.entityManager().removeEntity(static_cast<EntityId>(id));
            spdlog::debug("Destroyed entity {}", id);
        }
    };

    callbacks.onEntityMove = [this](u32 entityId, f32 dx, f32 dy, f32 dz) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setTargetPosition(
                entity->x() + dx,
                entity->y() + dy,
                entity->z() + dz);
        }
    };

    callbacks.onEntityTeleport = [this](u32 entityId, f32 x, f32 y, f32 z,
                                         f32 yaw, f32 pitch) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
        }
    };

    callbacks.onEntityVelocity = [this](u32 entityId, i16 vx, i16 vy, i16 vz) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            // 速度单位转换: 1/8000 block/tick -> block/tick
            entity->setVelocity(
                static_cast<f32>(vx) / 8000.0f,
                static_cast<f32>(vy) / 8000.0f,
                static_cast<f32>(vz) / 8000.0f);
        }
    };

    callbacks.onEntityHeadLook = [this](u32 entityId, f32 headYaw) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setHeadRotation(headYaw);
        }
    };

    callbacks.onEntityAnimation = [this](u32 entityId, u8 /*animation*/) {
        // TODO: 处理实体动画（挥手等）
        (void)entityId;
    };

    callbacks.onEntityStatus = [this](u32 entityId, u8 /*status*/) {
        // TODO: 处理实体状态（受伤、死亡等）
        (void)entityId;
    };

    // ========== 天气回调 ==========
    callbacks.onRainStrengthChange = [this](f32 strength) {
        m_world.onRainStrengthChange(strength);
    };

    callbacks.onThunderStrengthChange = [this](f32 strength) {
        m_world.onThunderStrengthChange(strength);
    };

    callbacks.onBeginRaining = [this]() {
        m_world.onBeginRaining();
    };

    callbacks.onEndRaining = [this]() {
        m_world.onEndRaining();
    };

    // ========== 游戏模式回调 ==========
    callbacks.onGameModeChange = [this](GameMode mode) {
        spdlog::info("Game mode changed to {}", static_cast<int>(mode));
        // 更新本地玩家的游戏模式
        if (m_player) {
            m_player->setGameMode(mode);
        }
    };

    // ========== 玩家能力回调 ==========
    callbacks.onPlayerAbilities = [this](bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed) {
        spdlog::debug("Player abilities updated: invulnerable={}, flying={}, canFly={}, creativeMode={}",
                      invulnerable, flying, canFly, creativeMode);
        // 更新本地玩家能力
        if (m_player) {
            PlayerAbilities& abilities = m_player->abilities();
            abilities.invulnerable = invulnerable;
            abilities.flying = flying;
            abilities.canFly = canFly;
            abilities.creativeMode = creativeMode;
            abilities.flySpeed = flySpeed;
            abilities.walkSpeed = walkSpeed;
        }
    };

    // ========== 光照更新回调 ==========
    callbacks.onLightUpdate = [this](i32 chunkX, i32 chunkZ, i32 sectionY,
                                      const std::vector<u8>& skyLight,
                                      const std::vector<u8>& blockLight,
                                      bool trustEdges) {
        m_world.onLightUpdate(chunkX, chunkZ, sectionY, skyLight, blockLight, trustEdges);
    };

    // ========== 方块破坏动画回调 ==========
    callbacks.onBlockBreakAnim = [this](EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage) {
        // 使用 BreakProgressManager 更新远程玩家的挖掘进度
        using namespace mc::client::renderer::trident::block;
        auto& manager = BreakProgressManager::instance();

        BlockPos pos(x, y, z);
        u64 currentTick = m_world.gameTime();  // 使用世界时间

        if (stage < 0) {
            // stage = -1 表示移除破坏效果
            manager.removeRemoteProgress(breakerEntityId);
        } else {
            // stage = 0-9 表示破坏阶段
            manager.updateRemoteProgress(breakerEntityId, pos, stage, currentTick);
        }
    };

    // ========== 声音回调 ==========
    callbacks.onPlaySound = [this](const ResourceLocation& soundEventId,
                                   mc::sound::SoundCategory category,
                                   f32 x,
                                   f32 y,
                                   f32 z,
                                   f32 volume,
                                   f32 pitch) {
        if (!m_soundEngine) {
            spdlog::warn("Received sound event '{}' but sound engine is not initialized", soundEventId.toString());
            return;
        }

        auto sound = sound::SoundInstance::createLocated(
            soundEventId,
            category,
            x,
            y,
            z,
            volume,
            pitch);

        (void)m_soundEngine->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
        // MC_TRACE_CLIENT_SOUND_EVENT("OnPlaySound_Result", "soundId", id);
    };

    callbacks.onStopSound = [this](const Optional<ResourceLocation>& soundEventId,
                                   const Optional<mc::sound::SoundCategory>& category) {
        if (!m_soundEngine) {
            return;
        }

        if (!soundEventId.has_value() && !category.has_value()) {
            m_soundEngine->stopAll();
            return;
        }

        if (soundEventId.has_value()) {
            m_soundEngine->stop(*soundEventId);
            return;
        }

        if (category.has_value()) {
            m_soundEngine->stop(*category);
        }
    };

    m_networkClient->setCallbacks(callbacks);
}

void ClientApplication::toggleMouseCapture()
{
    m_mouseCaptured = !m_mouseCaptured;
    m_input.setMouseLocked(m_mouseCaptured);

    if (m_mouseCaptured) {
        spdlog::debug("Mouse captured - first person mode");
    } else {
        spdlog::debug("Mouse released - UI mode");
    }
}

void ClientApplication::handleBlockInteractionInput(f32 deltaTime)
{
    enum class MiningInputState : i32 {
        Active = 0,
        NoMouseOrPlayer,
        AttackNotPressed,
        NoTargetBlock,
        InvalidTargetState,
    };

    static MiningInputState s_lastMiningState = MiningInputState::Active;
    auto setMiningState = [](MiningInputState state, const char* reason) {
        MC_TRACE_INSTANT("client.input.mining", "setMiningState", "state", static_cast<i32>(state), "reason", reason);
        if (s_lastMiningState != state) {
            s_lastMiningState = state;
        }
    };

    auto abortBreakingBlock = [this]() {
        if (!m_breakingBlockActive) {
            return;
        }

        sendBlockInteraction(network::BlockInteractionAction::AbortDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);

        // 清除本地破坏进度
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().stopBreaking();

        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;

        MC_TRACE_INSTANT("client.input.mining", "abortBreakingBlock", "state", static_cast<i32>(MiningInputState::Active), "reason", "abort local breaking state");
    };

    if (!m_mouseCaptured || !m_player) {
        // 如果用户没有挖掘操作，那么这个路径每帧都会走
        setMiningState(MiningInputState::NoMouseOrPlayer, "mouse not captured or player missing");
        abortBreakingBlock();
        return;
    }

    const bool attackPressed = m_input.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool attackJustPressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
    const bool hasTargetBlock = m_raycastResult.isHit();

    if (!attackPressed) {
        setMiningState(MiningInputState::AttackNotPressed, "attack button released");
        abortBreakingBlock();
        return;
    }

    if (!hasTargetBlock) {
        setMiningState(MiningInputState::NoTargetBlock, "raycast miss while attack pressed");
        abortBreakingBlock();
        return;
    }

    setMiningState(MiningInputState::Active, "attack pressed and target hit");

    const BlockPos currentTargetPos = m_raycastResult.blockPos();
    const Direction currentTargetFace = m_raycastResult.face();
    const bool targetChanged = !m_breakingBlockActive ||
        currentTargetPos != m_breakingBlockPos ||
        currentTargetFace != m_breakingBlockFace;

    if (targetChanged) {
        abortBreakingBlock();
        m_breakingBlockPos = currentTargetPos;
        m_breakingBlockFace = currentTargetFace;
        m_breakingBlockActive = true;
        m_breakingBlockProgress = 0.0f;

        // 开始挖掘 - 更新 BreakProgressManager
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().startBreaking(m_breakingBlockPos);

        MC_TRACE_INSTANT("client.input.mining",
            "startBreaking",
            "pos",
            fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
            "face",
            static_cast<i32>(m_breakingBlockFace),
            "justPressed", attackJustPressed,
            [flow = ::perfetto::Flow::ProcessScoped(m_breakingBlockPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

        sendBlockInteraction(network::BlockInteractionAction::StartDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
    }

    const BlockState* targetState = m_world.getBlockState(
        m_breakingBlockPos.x,
        m_breakingBlockPos.y,
        m_breakingBlockPos.z);
    if (targetState == nullptr || targetState->isAir() || targetState->hardness() < 0.0f) {
        setMiningState(MiningInputState::InvalidTargetState, "target state is null/air/unbreakable");
        abortBreakingBlock();
        return;
    }

    if (m_player->gameMode() == GameMode::Creative || targetState->hardness() == 0.0f) {
        if (!attackJustPressed) {
            return;
        }

        // 创造模式直接破坏
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().stopBreaking();

        sendBlockInteraction(network::BlockInteractionAction::StopDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;
        MC_TRACE_INSTANT("client.input.mining",
            "instantBreak",
            "pos",
            fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
            "face",
            static_cast<i32>(m_breakingBlockFace)
        );
        return;
    }

    m_breakingBlockProgress += deltaTime * constants::TICK_RATE *
        calculateBlockBreakingDelta(*m_player, *targetState);

    // 更新本地破坏进度
    {
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().updateLocalProgress(m_breakingBlockPos, m_breakingBlockProgress);
    }

    if (m_breakingBlockProgress >= 1.0f) {
        // 方块被破坏
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().stopBreaking();

        MC_TRACE_INSTANT("client.input.mining",
            "breakComplete",
            "pos",
            fmt::format("({}, {}, {})", m_breakingBlockPos.x, m_breakingBlockPos.y, m_breakingBlockPos.z),
            "face",
            static_cast<i32>(m_breakingBlockFace),
            "progress", m_breakingBlockProgress,
            [flow = ::perfetto::Flow::ProcessScoped(m_breakingBlockPos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

        sendBlockInteraction(network::BlockInteractionAction::StopDestroyBlock,
                             m_breakingBlockPos,
                             m_breakingBlockFace);
        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;
    }
}

void ClientApplication::handleBlockPlacementInput(f32 deltaTime)
{
    enum class PlaceInputState : i32 {
        Active = 0,
        NoMouseOrPlayer,
        NoUsePress,
        Cooldown,
        RaycastMiss,
    };

    static PlaceInputState s_lastPlaceState = PlaceInputState::Active;
    auto setPlaceState = [](PlaceInputState state, const char* reason) {
        if (s_lastPlaceState != state) {
            s_lastPlaceState = state;
            spdlog::info("[PlaceInput] state={} reason={}", static_cast<i32>(state), reason);
        }
    };

    m_placeCooldown = std::max(0.0f, m_placeCooldown - deltaTime);

    if (!m_mouseCaptured || !m_player) {
        setPlaceState(PlaceInputState::NoMouseOrPlayer, "mouse not captured or player missing");
        return;
    }

    // 检查右键是否刚刚按下
    const bool usePressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
    if (!usePressed) {
        setPlaceState(PlaceInputState::NoUsePress, "right button not just pressed");
        return;
    }

    // 检查放置冷却
    if (m_placeCooldown > 0.0f) {
        setPlaceState(PlaceInputState::Cooldown, "placement cooldown active");
        return;
    }

    // 检查射线是否击中方块
    if (m_raycastResult.isMiss()) {
        setPlaceState(PlaceInputState::RaycastMiss, "raycast miss on use");
        return;
    }

    setPlaceState(PlaceInputState::Active, "right button pressed and target hit");

    // 计算击中点相对坐标
    BlockPos pos = m_raycastResult.blockPos();
    Direction face = m_raycastResult.face();
    Vector3 hitPos = m_raycastResult.hitPosition();
    Vector3 blockPosFloat(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    Vector3 relativeHit = hitPos - blockPosFloat;  // 转换为方块内相对坐标

    // 发送放置包
    sendBlockPlacement(pos, face, relativeHit);
    m_placeCooldown = PLACE_COOLDOWN_TIME;
}

void ClientApplication::sendBlockPlacement(const BlockPos& pos, Direction face, const Vector3& hitPos)
{
    if (!m_networkClient || !m_networkClient->isLoggedIn()) {
        spdlog::info("[Place] Skip sending block placement because client is not logged in");
        return;
    }

    spdlog::info("[Place] Send placement pos=({}, {}, {}) face={} hit=({:.2f}, {:.2f}, {:.2f})",
                 pos.x, pos.y, pos.z,
                 static_cast<i32>(face),
                 hitPos.x, hitPos.y, hitPos.z);

    m_networkClient->sendBlockPlacement(pos.x, pos.y, pos.z, face,
                                        hitPos.x, hitPos.y, hitPos.z);
}

void ClientApplication::sendBlockInteraction(network::BlockInteractionAction action,
                                             const BlockPos& pos,
                                             Direction face)
{
    if (!m_networkClient || !m_networkClient->isLoggedIn()) {
        spdlog::debug("[Mining] Skip sending block interaction because client is not logged in");
        return;
    }

    // spdlog::info("[Mining] Send action={} pos=({}, {}, {}) face={}",
    //              static_cast<i32>(action),
    //              pos.x,
    //              pos.y,
    //              pos.z,
    //              static_cast<i32>(face));
    MC_TRACE_INSTANT("client.input.mining", "sendBlockInteraction",
        "action", static_cast<i32>(action),
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "face", static_cast<i32>(face),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
                flow(ctx);
        });

    m_networkClient->sendBlockInteraction(action, pos.x, pos.y, pos.z, face);
}

void ClientApplication::sendPlayerPosition()
{
    if (!m_networkClient || !m_networkClient->isLoggedIn() || !m_player) {
        return;
    }

    const auto& pos = m_player->position();

    // 检查是否需要发送（位置或旋转变化）
    bool positionChanged =
        std::abs(pos.x - m_lastSentX) > 0.001f ||
        std::abs(pos.y - m_lastSentY) > 0.001f ||
        std::abs(pos.z - m_lastSentZ) > 0.001f;

    bool rotationChanged =
        std::abs(m_player->yaw() - m_lastSentYaw) > 0.01f ||
        std::abs(m_player->pitch() - m_lastSentPitch) > 0.01f;

    network::PlayerPosition playerPos;
    playerPos.x = pos.x;
    playerPos.y = pos.y;
    playerPos.z = pos.z;
    playerPos.yaw = m_player->yaw();
    playerPos.pitch = m_player->pitch();
    playerPos.onGround = m_player->onGround();

    network::PlayerMovePacket::MoveType type;
    if (positionChanged && rotationChanged) {
        type = network::PlayerMovePacket::MoveType::Full;
    } else if (positionChanged) {
        type = network::PlayerMovePacket::MoveType::Position;
    } else if (rotationChanged) {
        type = network::PlayerMovePacket::MoveType::Rotation;
    } else {
        // 无变化，只发送地面状态
        type = network::PlayerMovePacket::MoveType::GroundOnly;
    }

    m_networkClient->sendPlayerMove(playerPos, type);

    // 更新上次发送的位置
    m_lastSentX = pos.x;
    m_lastSentY = pos.y;
    m_lastSentZ = pos.z;
    m_lastSentYaw = m_player->yaw();
    m_lastSentPitch = m_player->pitch();
}

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

    // 2. 扫描资源包目录
    String resourcePackDir = m_settings.resourcePackDir.get();
    if (resourcePackDir.empty()) {
        resourcePackDir = "resourcepacks";
    }

    spdlog::info("Scanning resource pack directory: {}", resourcePackDir);
    auto scanResult = m_resourcePackList.scanDirectory(std::filesystem::path(resourcePackDir));
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
    bool hasResources = false;
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->loadAllResources();
        if (loadResult.failed()) {
            spdlog::warn("Failed to load resources: {}", loadResult.error().toString());
        } else {
            hasResources = true;
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
        spdlog::info("Block model cache initialized with {} appearances",
                    m_modelCache.cachedAppearanceCount());
        // 设置 ChunkMesher 使用 BlockModelCache
        ChunkMesher::setModelCache(&m_modelCache);
    } else {
        spdlog::warn("Failed to initialize block model cache");
    }

    // 8. 设置资源包变更回调
    m_resourcePackList.onChange([this]() {
        spdlog::info("Resource packs changed, reloading...");
        reloadResources();
    });

    return Result<void>::ok();
}

void ClientApplication::reloadResources()
{
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
            spdlog::info("Reloaded resources: {} appearances cached",
                        m_modelCache.cachedAppearanceCount());
        }

        if (m_renderer) {
            auto atlasUpdateResult = m_renderer->updateTextureAtlas(atlasResult.value());
            if (atlasUpdateResult.failed()) {
                spdlog::error("Failed to update renderer texture atlas after reload: {}",
                              atlasUpdateResult.error().toString());
            }

            auto reloadCloudResult = m_renderer->reloadCloudTexture(m_resourceManager.get());
            if (reloadCloudResult.failed()) {
                spdlog::warn("Failed to reload cloud texture after resource reload: {}",
                             reloadCloudResult.error().toString());
            }
        }

        m_world.forEachChunk([](const ChunkId&, ClientChunk& chunk) {
            chunk.needsMeshUpdate = true;
        });
        spdlog::info("Marked loaded chunks dirty after resource reload");
    }
}

void ClientApplication::handleChatCommand(const String& input)
{
    if (input.empty()) {
        return;
    }

    // 获取 ChatWidget
    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;

    // 添加到聊天历史
    if (chatWidget) {
        chatWidget->addMessage(input, 0xFFFFFFFF);
    }

    // 检查是否为命令（以 / 开头）
    if (input[0] == '/') {
        String command = input.substr(1);

        spdlog::info("Chat command received: {}", std::string(command.begin(), command.end()));

        // 发送到服务端
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 本地回显
            if (chatWidget) {
                chatWidget->addSystemMessage("Command executed locally (not connected to server)");
            }
        }
    } else {
        // 普通聊天消息，发送到服务端
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 本地回显
            if (chatWidget) {
                chatWidget->addSystemMessage("Message sent locally (not connected to server)");
            }
        }
    }
}

} // namespace mc::client
