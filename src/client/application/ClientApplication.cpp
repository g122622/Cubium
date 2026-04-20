#include "ClientApplication.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/resource/VanillaResources.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/FolderResourcePack.hpp"
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
#include "client/ui/GuiScale.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/screen/ChestScreen.hpp"
#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/ui/minecraft/widgets/CrosshairWidget.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoResolver.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/sound/SoundCategory.hpp"
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

    const size_t slotCount = std::min(static_cast<size_t>(menu->getSlotCount()), items.size());
    for (size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        Slot* slot = menu->getSlot(static_cast<i32>(slotIndex));
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

template <typename ScreenT>
bool isMatchingContainerScreen(IScreen* screen, ContainerId containerId) {
    auto* typedScreen = dynamic_cast<ScreenT*>(screen);
    if (typedScreen == nullptr || typedScreen->getMenu() == nullptr) {
        return false;
    }

    return typedScreen->getMenu()->getId() == containerId;
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
 * @brief ClientWorld 鍒?IBlockReader 鐨勮交閲忛€傞厤鍣? *
 * 灏勭嚎妫€娴嬫帴鍙ｅ綋鍓嶈姹?IBlockReader锛? * 鑰?ClientWorld 瀹炵幇鐨勬槸 ICollisionWorld锛堟柟娉曠鍚嶅吋瀹癸級銆? */
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

    // IWorld 鎺ュ彛瀹炵幇 - 濮旀墭鍒?ClientWorld
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
    [[nodiscard]] Difficulty difficulty() const override;

private:
    const ClientWorld& m_world;
};

// IWorld 鎺ュ彛瀹炵幇
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
Difficulty ClientWorldBlockReader::difficulty() const { return Difficulty::Normal; }

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
    // 鍒濆鍖栨€ц兘杩借釜
    mc::perfetto::TraceConfig traceConfig;
    traceConfig.outputPath = "client_trace.perfetto-trace";
    traceConfig.bufferSizeKb = 65536; // 64MB
    mc::perfetto::PerfettoManager::instance().initialize(traceConfig);
    mc::perfetto::PerfettoManager::instance().startTracing();

    // 璁剧疆杩涚▼鍜屼富绾跨▼鍚嶇О
    mc::perfetto::PerfettoManager::instance().setProcessName("MinecraftClient");
    mc::perfetto::PerfettoManager::instance().setThreadName("ClientMainThread");
    spdlog::info("Perfetto tracing initialized");

    MC_TRACE_EVENT("client.initialization", "ClientApplication::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Client already initialized");
    }

    // 鍔犺浇璁剧疆
    String settingsPath = params.settingsPath.value_or(
        ClientSettings::getSettingsPath("minecraft-reborn").string());
    auto settingsResult = loadSettings(settingsPath);
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
                     settingsPath, settingsResult.error().toString());
    }

    // 搴旂敤鍛戒护琛岃鐩?
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

    // 搴旂敤璁剧疆鍒扮郴缁?
    applySettings();

    // 璁剧疆鏃ュ織绾у埆
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

    // 鍒濆鍖栨柟鍧楁敞鍐岃〃
    {
        MC_TRACE_EVENT("client.initialization", "InitializeVanillaBlocks");
        VanillaBlocks::initialize();
        spdlog::info("Vanilla blocks initialized");
    }

    // 鍒濆鍖栫墿鍝佹敞鍐岃〃
    {
        MC_TRACE_EVENT("client.initialization", "InitializeVanillaItems");
        Items::initialize();
        spdlog::info("Vanilla items initialized");
    }

    // 娉ㄥ唽瀹炰綋绫诲瀷
    {
        MC_TRACE_EVENT("client.initialization", "RegisterVanillaEntities");
        entity::VanillaEntities::registerAll();
        spdlog::info("Entity types registered");
    }

    // 鍒濆鍖栨柟鍧楃墿鍝佹敞鍐岃〃
    {
        MC_TRACE_EVENT("client.initialization", "InitializeVanillaBlockItems");
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        spdlog::info("Block items initialized");
    }

    // 鍒濆鍖栧０闊崇郴缁?
    {
        MC_TRACE_EVENT("client.initialization", "InitializeSoundSystem");

        spdlog::info("Initializing sound system...");

        // 娣诲姞鍐呯疆璧勬簮鍖呭埌 ResourcePackList锛堢敤浜?sounds.json锛?
        // 鍐呯疆璧勬簮鍖呭叿鏈夋渶浣庝紭鍏堢骇锛?1锛夛紝澶栭儴璧勬簮鍖呭彲浠ヨ鐩?
        auto builtinPackResult = m_resourcePackList.addPack(
            std::filesystem::path("resources/data/minecraft"), true, -1);
        if (builtinPackResult.success() && builtinPackResult.value().initialized) {
            spdlog::info("Added built-in resources pack to sound system");
        } else {
            spdlog::warn("Failed to add built-in resources pack: {}",
                         builtinPackResult.success() ? builtinPackResult.value().error : builtinPackResult.error().toString());
        }

        m_audioService = std::make_unique<sound::AudioService>(m_resourcePackList, m_settings);

        auto soundInitResult = m_audioService->initialize();
        if (soundInitResult.failed()) {
            spdlog::warn("Failed to initialize sound engine: {}. Audio will be disabled.",
                        soundInitResult.error().toString());
            m_audioService.reset();
        } else {
            // 娣诲姞鐜闊虫晥澶勭悊鍣?
            spdlog::info("Sound system initialized successfully");
        }
    }

    // 鍒濆鍖栬祫婧愮郴缁?
    {
        MC_TRACE_EVENT("client.initialization", "InitializeResources");
        spdlog::info("Initializing resource system...");
        auto resourceResult = initializeResources();
        if (resourceResult.failed()) {
            spdlog::warn("Failed to initialize resource system: {}. Using default rendering.",
                        resourceResult.error().toString());
        }
    }

    // 鍒濆鍖栨寜閿粦瀹?
    m_settings.initializeKeyBindings();
    spdlog::info("Key bindings initialized");

    // 鍒涘缓绐楀彛
    WindowConfig windowConfig;

    {
        MC_TRACE_EVENT("client.initialization", "CreateWindow");

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
    }

    // 鍒濆鍖栬緭鍏ョ鐞嗗櫒
    m_input.initialize(m_window.handle());

    // 璁剧疆鎸夐敭缁戝畾
    setupInputBindings();

    // 璁剧疆璁剧疆鍙樻洿鍥炶皟
    setupSettingCallbacks();

    // 璁剧疆绐楀彛澶у皬鍙樺寲鍥炶皟
    m_window.setResizeCallback([](i32 width, i32 height, void* userData) {
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
    }, this);

    // 鍒濆鍖朤rident娓叉煋寮曟搸
    spdlog::info("Initializing Trident renderer...");
    m_renderer = std::make_unique<renderer::trident::TridentEngine>();

    renderer::api::RenderEngineConfig rendererConfig;
    rendererConfig.appName = "Minecraft Reborn";
    rendererConfig.enableValidation = true; // Debug妯″紡鍚敤楠岃瘉灞?
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

    // 浠?settings 鍚屾杩愯鏃舵覆鏌撳弬鏁?
    m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());
    m_renderer->setLandFogDensity(m_settings.fogDensity.get());
    m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(m_settings.clouds.get()));

    // 璁剧疆鐩告満
    setupCamera();

    // 灏嗙浉鏈鸿缃粰娓叉煋鍣?
    m_renderer->setCamera(&m_camera);

    // 鏇存柊娓叉煋鍣ㄧ汗鐞嗗浘闆嗭紙浣跨敤 ResourceManager 鏋勫缓鐨勭汗鐞嗭級
    if (m_resourceManager) {
        MC_TRACE_EVENT("client.initialization", "UpdateRendererTextureAtlas");

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

    // 鍒濆鍖?Trident 瀛愭覆鏌撳櫒
    {
        MC_TRACE_EVENT("client.initialization", "InitializeTridentSubRenderers");

        auto skyInitResult = m_renderer->initializeSkyRenderer();
        if (skyInitResult.failed()) {
            spdlog::warn("Failed to initialize sky renderer: {}", skyInitResult.error().toString());
        }

        auto guiInitResult = m_renderer->initializeGuiRenderer();
        if (guiInitResult.failed()) {
            spdlog::warn("Failed to initialize GUI renderer: {}", guiInitResult.error().toString());
        }

        // 鍒濆鍖?GUI 绾圭悊绠＄悊鍣紙鐢ㄤ簬鑳屽寘灞忓箷绛夊鍣℅UI锛?
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
                // 鍔犺浇鑳屽寘绾圭悊
                auto loadResult = m_guiTextureManager->loadInventoryTexture();
                if (loadResult.failed()) {
                    spdlog::warn("Failed to load inventory texture: {}", loadResult.error().toString());
                }

                // 娉ㄥ唽鍒?GuiRenderer
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

        // 瀹炰綋娓叉煋鍣ㄥ繀椤诲厛鍒濆鍖栵紙鍒涘缓 EntityPipeline锛?
        auto entityInitResult = m_renderer->initializeEntityRenderer();
        if (entityInitResult.failed()) {
            spdlog::warn("Failed to initialize entity renderer: {}", entityInitResult.error().toString());
        }

        // 瀹炰綋绾圭悊鍥鹃泦鍦?EntityPipeline 鍒涘缓鍚庡垵濮嬪寲
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

        // 鍒濆鍖栭浘鏁堟灉绠＄悊鍣?
        auto fogInitResult = m_renderer->initializeFogManager();
        if (fogInitResult.failed()) {
            spdlog::warn("Failed to initialize fog manager: {}", fogInitResult.error().toString());
        }

        // 鍒濆鍖栦簯娓叉煋鍣?
        auto cloudInitResult = m_renderer->initializeCloudRenderer(m_resourceManager.get());
        if (cloudInitResult.failed()) {
            spdlog::warn("Failed to initialize cloud renderer: {}", cloudInitResult.error().toString());
        }

        // 鍒濆鍖栫矑瀛愮鐞嗗櫒
        auto particleInitResult = m_renderer->initializeParticleManager();
        if (particleInitResult.failed()) {
            spdlog::warn("Failed to initialize particle manager: {}", particleInitResult.error().toString());
        }

        // 鍒濆鍖栧ぉ姘旀覆鏌撳櫒
        auto weatherInitResult = m_renderer->initializeWeatherRenderer();
        if (weatherInitResult.failed()) {
            spdlog::warn("Failed to initialize weather renderer: {}", weatherInitResult.error().toString());
        }

        // 鍒濆鍖栫牬鍧忚繘搴︽覆鏌撳櫒
        auto breakProgressInitResult = m_renderer->initializeBreakProgressRenderer(m_resourceManager.get());
        if (breakProgressInitResult.failed()) {
            spdlog::warn("Failed to initialize break progress renderer: {}", breakProgressInitResult.error().toString());
        }

        // 鍒濆鍖栫涓€浜虹О鎵嬮儴娓叉煋鍣?
        auto firstPersonInitResult = m_renderer->initializeFirstPersonRenderer();
        if (firstPersonInitResult.failed()) {
            spdlog::warn("Failed to initialize first person renderer: {}", firstPersonInitResult.error().toString());
        }
    }

    // 鍚姩鍐呯疆鏈嶅姟绔?
    if (!params.skipIntegratedServer) {
        MC_TRACE_EVENT("client.initialization", "StartIntegratedServer");

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

        // 鍒濆鍖栫綉缁滃鎴风
        m_networkClient = std::make_unique<NetworkClient>();
        m_commandManager = std::make_unique<command::ClientCommandManager>();
        m_commandManager->setPlayerNameProvider([this]() {
            return collectPlayerCompletionCandidates();
        });
        m_commandManager->setEntityNameProvider([this]() {
            return collectEntityCompletionCandidates();
        });
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

    // 鍒濆鍖栦笘鐣?
    {
        MC_TRACE_EVENT("client.initialization", "InitializeWorld");
        spdlog::info("Initializing world...");
        auto worldResult = m_world.initialize(12345); // 浣跨敤鍥哄畾绉嶅瓙
        if (worldResult.failed()) {
            spdlog::error("Failed to initialize world: {}", worldResult.error().toString());
            if (m_integratedServer) {
                m_integratedServer->stop();
            }
            m_renderer->destroy();
            m_window.destroy();
            return worldResult.error();
        }
    }

    // 鍒濆鍖栫綉鏍兼瀯寤虹郴缁燂紙鎵ц鍣?+ 鐙珛璋冨害鍣級
    spdlog::info("Initializing mesh build system...");
    MeshSchedulerConfig schedulerConfig;
    schedulerConfig.maxDispatchedTaskCount = std::max(32, m_settings.renderDistance.get() * 12);
    schedulerConfig.reprioritizeIntervalFrames = 6;
    schedulerConfig.cameraMoveThreshold = 2.0f;
    schedulerConfig.cameraDirectionDotThreshold = 0.96f;
    schedulerConfig.behindCancelDotThreshold = -0.35f;
    schedulerConfig.behindCancelDistanceChunks = static_cast<f32>(std::max(6, m_settings.renderDistance.get() / 2));
    m_world.initializeMeshSystem(-1, schedulerConfig);

    // 璁剧疆鍖哄潡鍗歌浇鍥炶皟锛岄€氱煡 ChunkRenderer 閲婃斁 GPU 缂撳啿鍖?
    m_world.setChunkUnloadCallback([this](const ChunkId& chunkId) {
        if (m_renderer && m_renderer->isChunkRendererInitialized()) {
            m_renderer->chunkRenderer().removeChunk(chunkId);
        }
    });

    // 璁剧疆瀹炰綋娓叉煋鍥炶皟
    m_renderer->setEntityRenderCallback([this](VkCommandBuffer cmd, f64 partialTick) {
        m_world.entityManager().forEachEntity([&](client::ClientEntity& entity) {
            m_renderer->entityRendererManager().renderWithPipeline(cmd, entity, partialTick);
        });
    });

    // 璁剧疆绗竴浜虹О鎵嬮儴娓叉煋鍥炶皟
    m_renderer->setFirstPersonRenderCallback([this](VkCommandBuffer cmd, VkDescriptorSet cameraSet, f64 partialTick) {
        if (!m_renderer || !m_player || !m_renderer->isFirstPersonRendererInitialized()) {
            return;
        }

        renderer::trident::firstperson::FirstPersonRenderer::RenderContext renderContext;
        renderContext.player = m_player.get();
        renderContext.partialTick = partialTick;

        m_renderer->firstPersonRenderer().render(cmd, cameraSet, renderContext);
    });

    // 鍒濆鍖栨柟鍧楃鎾炴敞鍐岃〃
    spdlog::info("Initializing block collision registry...");

    // 鍒涘缓鐗╃悊寮曟搸
    m_physicsEngine = std::make_unique<PhysicsEngine>(m_world);

    // 鍒涘缓鐜╁瀹炰綋
    m_player = std::make_unique<Player>(static_cast<EntityId>(1), m_settings.username.get());
    m_player->setPosition(8.0, 50.0, 8.0);  // 鍒濆浣嶇疆鍦ㄥ湴闈笂鏂?
    m_player->setPhysicsEngine(m_physicsEngine.get());
    // 榛樿鍒涢€犳ā寮忓苟鍚敤椋炶
    m_player->setGameMode(GameMode::Creative);
    m_player->setCreativeModeInventory();
    m_player->abilities().flying = true;
    spdlog::info("Player created at (8, 50, 8)");

    spdlog::info("Client initialized successfully");
    spdlog::info("Window: {}x{}", m_window.width(), m_window.height());
    spdlog::info("Controls: WASD to move, Space to jump/fly up, Shift to sneak/fly down, mouse to look");
    spdlog::info("Press F to toggle flying, F3 to toggle debug screen, ALT to toggle mouse capture");

    // 鍒濆鍖?Kagero UI 寮曟搸
    if (m_renderer->isGuiRendererInitialized()) {
        MC_TRACE_EVENT("client.initialization", "InitializeGuiRenderer");

        auto* guiFont = m_renderer->guiRenderer().font();
        if (guiFont == nullptr) {
            spdlog::error("Failed to get GUI font for KageroEngine");
        } else {
            MC_TRACE_EVENT("client.initialization", "CreateKageroEngine");

            // 鍒濆鍖?GUI 绮剧伒鍥鹃泦锛堝弻鍥鹃泦鏋舵瀯锛?            // 閲嶈锛氱汗鐞嗗姞杞介『搴?- 鍏堝垵濮嬪寲鍥鹃泦锛屽啀鍔犺浇绾圭悊锛堣缃纭昂瀵革級锛屾渶鍚庢敞鍐岀簿鐏?
            // icons.png: 蹇冨舰銆侀ゥ楗裤€佺洈鐢层€佺粡楠屾潯绛?
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

            // widgets.png: 蹇嵎鏍忋€佹寜閽瓑
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

            // 鍑嗗绾圭悊鍔犺浇鍣?
            renderer::trident::gui::GuiTextureLoader textureLoader;
            bool hasTextureLoader = false;

            // 浠庤祫婧愬寘鍔犺浇绾圭悊
            if (m_resourceManager && m_resourceManager->resourcePackCount() > 0) {
                MC_TRACE_EVENT("client.initialization", "LoadGuiTexturesFromResourcePacks");
                spdlog::info("[GUI] ResourceManager has {} resource packs", m_resourceManager->resourcePackCount());

                // 娣诲姞鍚敤鐨勮祫婧愬寘鍒板姞杞藉櫒
                auto enabledPacks = m_resourcePackList.getEnabledPacks();
                spdlog::info("[GUI] ResourcePackList has {} enabled packs", enabledPacks.size());
                for (const auto& pack : enabledPacks) {
                    if (pack) {
                        spdlog::info("[GUI] Adding enabled resource pack: {}", pack->name());
                        textureLoader.addResourcePack(pack);
                        hasTextureLoader = true;
                    }
                }

                // 濡傛灉娌℃湁浠庤缃惎鐢ㄧ殑璧勬簮鍖咃紝浣跨敤璧勬簮绠＄悊鍣ㄤ腑鐨勮祫婧愬寘
                if (!hasTextureLoader) {
                    spdlog::info("[GUI] No enabled packs from settings, using ResourceManager packs");
                    // 鑾峰彇璧勬簮绠＄悊鍣ㄤ腑鎵€鏈夎祫婧愬寘
                    for (size_t i = 0; i < m_resourceManager->resourcePackCount(); ++i) {
                        auto* pack = m_resourceManager->getResourcePack(i);
                        if (pack) {
                            spdlog::info("[GUI] Adding ResourceManager pack [{}]: {}", i, pack->name());
                            // 娉ㄦ剰锛氳繖閲屼娇鐢ㄧ┖鍒犻櫎鍣紝鍥犱负璧勬簮鍖呯敓鍛藉懆鏈熺敱ResourceManager绠＄悊
                            textureLoader.addResourcePack(
                                std::shared_ptr<mc::IResourcePack>(pack, [](mc::IResourcePack*) {}));
                            hasTextureLoader = true;
                        }
                    }
                }
            } else {
                spdlog::info("[GUI] ResourceManager is null or has no resource packs");
            }

            // 鍔犺浇绾圭悊骞舵敞鍐岀簿鐏?            // 鍏抽敭椤哄簭锛氬厛鍔犺浇绾圭悊锛堣缃纭殑鍥鹃泦灏哄锛夛紝鍐嶆敞鍐岀簿鐏碉紙璁＄畻姝ｇ‘鐨刄V锛?
            if (hasTextureLoader) {
                MC_TRACE_EVENT("client.initialization", "LoadGuiTextures");
                spdlog::info("[GUI] TextureLoader has {} resource packs", textureLoader.resourcePackCount());

                // 鍔犺浇 icons.png 鍒?iconsAtlas
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
                    // 绾圭悊鍔犺浇鍚庢敞鍐岀簿鐏碉紙浣跨敤姝ｇ‘鐨勫浘闆嗗昂瀵歌绠桿V锛?
                    renderer::trident::gui::GuiSpriteRegistry::registerIconsSprites(*m_iconsAtlas);
                    spdlog::info("[GUI] Icons atlas: {} sprites registered, texture={}",
                                m_iconsAtlas->spriteCount(),
                                m_iconsAtlas->hasTexture() ? "yes" : "no");

                    // 娉ㄥ唽鍥鹃泦鍒?GuiRenderer 骞惰缃Ы浣?
                    auto iconsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "icons", m_iconsAtlas->imageView(), m_iconsAtlas->sampler());
                    if (iconsSlotResult.success()) {
                        m_iconsAtlas->setAtlasSlot(static_cast<u8>(iconsSlotResult.value()));
                        spdlog::info("[GUI] Icons atlas registered at slot {}", iconsSlotResult.value());
                    } else {
                        spdlog::warn("[GUI] Failed to register icons atlas: {}", iconsSlotResult.error().toString());
                    }
                }

                // 鍔犺浇 widgets.png 鍒?widgetsAtlas
                if (m_widgetsAtlas) {
                    MC_TRACE_EVENT("client.initialization", "LoadWidgetsTexture");
                    spdlog::info("[GUI] Loading widgets.png to widgetsAtlas...");

                    auto loadResult = textureLoader.loadGuiTexture(*m_widgetsAtlas, "minecraft:textures/gui/widgets.png");
                    if (loadResult.failed()) {
                        spdlog::warn("[GUI] Failed to load widgets.png: {}. Using default textures.", loadResult.error().toString());
                        (void)m_widgetsAtlas->loadDefaultTextures();
                    } else {
                        spdlog::info("[GUI] Loaded widgets.png from resource pack ({}x{})",
                                    m_widgetsAtlas->atlasWidth(), m_widgetsAtlas->atlasHeight());
                    }
                    // 绾圭悊鍔犺浇鍚庢敞鍐岀簿鐏碉紙浣跨敤姝ｇ‘鐨勫浘闆嗗昂瀵歌绠桿V锛?
                    renderer::trident::gui::GuiSpriteRegistry::registerWidgetsSprites(*m_widgetsAtlas);
                    spdlog::info("[GUI] Widgets atlas: {} sprites registered, texture={}",
                                m_widgetsAtlas->spriteCount(),
                                m_widgetsAtlas->hasTexture() ? "yes" : "no");

                    // 娉ㄥ唽鍥鹃泦鍒?GuiRenderer 骞惰缃Ы浣?
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
                // 鏃犺祫婧愬寘锛屼娇鐢ㄩ粯璁ょ汗鐞?
                if (m_iconsAtlas) {
                    MC_TRACE_EVENT("client.initialization", "LoadDefaultIconsTexture");

                    (void)m_iconsAtlas->loadDefaultTextures();
                    // 浣跨敤榛樿256x256灏哄娉ㄥ唽绮剧伒
                    renderer::trident::gui::GuiSpriteRegistry::registerIconsSprites(*m_iconsAtlas);
                    // 娉ㄥ唽鍥鹃泦鍒?GuiRenderer
                    auto iconsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "icons", m_iconsAtlas->imageView(), m_iconsAtlas->sampler());
                    if (iconsSlotResult.success()) {
                        m_iconsAtlas->setAtlasSlot(static_cast<u8>(iconsSlotResult.value()));
                    }
                }
                if (m_widgetsAtlas) {
                    MC_TRACE_EVENT("client.initialization", "LoadDefaultWidgetsTexture");

                    (void)m_widgetsAtlas->loadDefaultTextures();
                    // 浣跨敤榛樿256x256灏哄娉ㄥ唽绮剧伒
                    renderer::trident::gui::GuiSpriteRegistry::registerWidgetsSprites(*m_widgetsAtlas);
                    // 娉ㄥ唽鍥鹃泦鍒?GuiRenderer
                    auto widgetsSlotResult = m_renderer->guiRenderer().registerAtlas(
                        "widgets", m_widgetsAtlas->imageView(), m_widgetsAtlas->sampler());
                    if (widgetsSlotResult.success()) {
                        m_widgetsAtlas->setAtlasSlot(static_cast<u8>(widgetsSlotResult.value()));
                    }
                }
            }

            // 鍒涘缓 TridentCanvas
            m_canvas = std::make_unique<ui::TridentCanvas>(
                m_renderer->guiRenderer(),
                *guiFont
            );
            m_canvas->resize(m_window.width(), m_window.height());

            // 鍒涘缓 KageroEngine
            m_kageroEngine = std::make_unique<ui::kagero::KageroEngine>();
            ui::kagero::KageroConfig kageroConfig;
            kageroConfig.screenWidth = m_window.width();
            kageroConfig.screenHeight = m_window.height();
            auto kageroInitResult = m_kageroEngine->initialize(*m_canvas, kageroConfig);
            if (kageroInitResult.failed()) {
                spdlog::error("Failed to initialize KageroEngine: {}", kageroInitResult.error().toString());
            } else {
                MC_TRACE_EVENT("client.initialization", "ConfigureKageroLayers");

                spdlog::info("KageroEngine initialized");

                // 灞?Z=0: 鍑嗘槦
                auto crosshairWidget = std::make_unique<ui::minecraft::widgets::CrosshairWidget>();
                m_crosshairLayerId = m_kageroEngine->addLayer(std::move(crosshairWidget), 0);

                // 灞?Z=10: HUD
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

                // 灞?Z=20: 鑱婂ぉ妗?
                auto chatWidget = std::make_unique<ui::minecraft::widgets::ChatWidget>();
                chatWidget->setFont(guiFont);
                chatWidget->setGuiRenderer(&m_renderer->guiRenderer());
                chatWidget->setCommandManager(m_commandManager.get());
                chatWidget->setCommandCallback([this](const String& input) {
                    handleChatCommand(input);
                });
                m_chatLayerId = m_kageroEngine->addLayer(std::move(chatWidget), 20);

                // 灞?Z=30: Screen 鏍?
                auto screenStackWidget = std::make_unique<ui::minecraft::widgets::ScreenStackWidget>();
                screenStackWidget->setGuiRenderer(&m_renderer->guiRenderer());

                // 璁剧疆 ScreenManager 鍚庣
                ScreenManager::instance().setScreenStackWidget(screenStackWidget.get());

                m_screenStackLayerId = m_kageroEngine->addLayer(std::move(screenStackWidget), 30);

                // 灞?Z=100: 璋冭瘯灞忓箷
                auto debugWidget = std::make_unique<ui::minecraft::DebugScreenWidget>();
                debugWidget->setTextWidthCallback([this](const std::string& text) -> f32 {
                    const f64 guiScale = static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1));
                    return static_cast<f32>(m_renderer->guiRenderer().getTextWidth(text) / guiScale);
                });
                debugWidget->setLineHeight(
                    static_cast<i32>(m_renderer->guiRenderer().getFontHeight() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))) + 2);
                debugWidget->setCamera(&m_camera);
                debugWidget->setWorld(&m_world);
                debugWidget->setEntityManager(&m_world.entityManager());
                debugWidget->setNetworkClient(m_networkClient.get());
                debugWidget->setRenderDistance(m_settings.renderDistance.get());
                if (m_player) {
                    debugWidget->setPlayer(m_player.get());
                }

                // 璁剧疆GPU淇℃伅
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

                spdlog::info("KageroEngine layers configured: crosshair={}, hud={}, targetInfo={}, chat={}, screenStack={}, debug={}",
                             m_crosshairLayerId, m_hudLayerId, m_targetInfoLayerId, m_chatLayerId, m_screenStackLayerId, m_debugScreenLayerId);
            }
        }

        applyGuiScale();

        // 璁剧疆瀛楃杈撳叆鍥炶皟 - 閫氳繃 KageroEngine 鍒嗗彂
        m_input.setCharCallback([this](u32 codepoint) {
            if (m_kageroEngine && m_kageroEngine->handleChar(codepoint)) {
                return;
            }
        });

        // 璁剧疆閿洏浜嬩欢鍥炶皟 - 閫氳繃 KageroEngine 鍒嗗彂
        m_input.setKeyEventCallback([this](i32 key, i32 action, i32 mods) {
            // F3 鍒囨崲璋冭瘯灞忓箷
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

            // 娓告垙杈撳叆澶勭悊
            if (action == GLFW_PRESS && !ScreenManager::instance().hasScreen()) {
                captureMouseAfterScreens(m_input, m_mouseCaptured);
            }
        });

        // 璁剧疆GUI娓叉煋鍥炶皟 - 瀹屽叏閫氳繃 KageroEngine
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

    // 鍒濆鎹曡幏榧犳爣
    toggleMouseCapture();

    m_lastFrameTime = glfwGetTime();

    while (m_running && !m_window.shouldClose()) {
        MC_TRACE_EVENT("rendering.frame", "Frame", "phase", "frame");

        const auto frameStart = clock::now();

        // 璁＄畻甯ф椂闂?
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 澶勭悊浜嬩欢
        {
            MC_TRACE_EVENT("rendering.frame", "HandleEvents", "phase", "handle_events");
            handleEvents();
        }

        // 鏇存柊
        {
            MC_TRACE_EVENT("rendering.frame", "Update", "phase", "update");
            update(deltaTime);
        }

        // 娓叉煋
        {
            MC_TRACE_EVENT("rendering.frame", "Render", "phase", "render");
            render();
        }

        // 娓呯悊鏈抚鐨勭灛鏃惰緭鍏ョ姸鎬?
        m_input.endFrame();

        // 甯ц鏁?        ++m_frameCount;

#if MC_ENABLE_TRACING
        // 杩借釜 FPS
        const f32 safeDeltaTime = std::max(deltaTime, 0.0001f);
        const i32 fps = static_cast<i32>(1.0f / safeDeltaTime);
        if (fps < 1000) { // 杩囨护鎺夊紓甯稿€?
        MC_TRACE_COUNTER("rendering.frame", "FPS", fps);
        }

        // 杩借釜鍐呭瓨淇℃伅
        MC_TRACE_COUNTER("memory", "ProcessMemory", static_cast<int64_t>(util::PlatformInfo::getProcessMemoryMB()));
#endif

        // 甯х巼闄愬埗锛?=涓嶉檺鍒讹級
        const i32 fpsLimit = m_settings.framerateLimit.get();
        if (fpsLimit > 0) {
            const auto minFrameDuration = std::chrono::duration<f64>(1.0 / static_cast<f64>(fpsLimit));
            const auto frameElapsed = clock::now() - frameStart;
            if (frameElapsed < minFrameDuration) {
                MC_TRACE_EVENT("rendering.frame", "FrameRateLimitSleep", "phase", "sleep");
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

    // 澶勭悊鑱婂ぉ妗嗛敭鐩樿緭鍏ワ紙浼樺厛浜庢父鎴忚緭鍏ワ級
    // 妫€鏌ヨ亰澶╂鏄惁鎵撳紑
    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;
    if (chatWidget && chatWidget->isOpen()) {
        // 鑱婂ぉ妗嗘墦寮€鏃讹紝ESC 鍏抽棴鑱婂ぉ妗?
        if (m_input.isKeyJustPressed(GLFW_KEY_ESCAPE)) {
            chatWidget->close();
            m_input.setMouseLocked(true);
            m_mouseCaptured = true;
        }
        return;
    }

    // 澶勭悊 Screen 鏍堜簨浠?
    auto* screenStack = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId)) : nullptr;
    if (screenStack && screenStack->hasScreen()) {
        const i32 guiMouseX = static_cast<i32>(m_input.mouseX() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)));
        const i32 guiMouseY = static_cast<i32>(m_input.mouseY() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)));

        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleClick(
                guiMouseX,
                guiMouseY,
                GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleClick(
                guiMouseX,
                guiMouseY,
                GLFW_MOUSE_BUTTON_RIGHT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_LEFT)) {
            m_kageroEngine->handleRelease(
                guiMouseX,
                guiMouseY,
                GLFW_MOUSE_BUTTON_LEFT);
        }
        if (m_input.isMouseButtonJustReleased(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_kageroEngine->handleRelease(
                guiMouseX,
                guiMouseY,
                GLFW_MOUSE_BUTTON_RIGHT);
        }

        if (!screenStack->hasScreen()) {
            captureMouseAfterScreens(m_input, m_mouseCaptured);
        }
        return;
    }

    // 妫€鏌LT閿垏鎹㈤紶鏍囨崟鑾?
    if (m_input.isKeyJustPressed(GLFW_KEY_LEFT_ALT) ||
        m_input.isKeyJustPressed(GLFW_KEY_RIGHT_ALT)) {
        toggleMouseCapture();
    }

    // T 閿墦寮€鑱婂ぉ妗?
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

    // / 閿墦寮€鍛戒护妗?
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

        // 鍒涘缓鑳屽寘灞忓箷骞惰缃覆鏌撳櫒
        auto inventoryScreen = std::make_unique<InventoryCraftingScreen>(
            std::make_unique<InventoryCraftingMenu>(inventory::PLAYER_CONTAINER_ID, &m_player->inventory()));

        // 璁剧疆娓叉煋鍣?
        if (m_renderer && m_renderer->isGuiRendererInitialized()) {
            inventoryScreen->setRenderers(
                &m_renderer->guiRenderer(),
                m_guiTextureManager.get(),
                m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
            inventoryScreen->setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
        }

        ScreenManager::instance().openScreen(std::move(inventoryScreen));
        return;
    }

    // 椋炶妯″紡鍒囨崲锛團閿級
    if (m_input.isKeyJustPressed(GLFW_KEY_F)) {
        if (m_player && m_player->abilities().canFly) {
            m_player->toggleFlying();
        }
    }

    // 浼犻€掗敭鐩樿緭鍏ュ埌鐜╁鍜岄紶鏍囨帶鍒?
    if (m_mouseCaptured && m_player) {
        // 榧犳爣瑙嗚鎺у埗 - 鏇存柊鐜╁鏈濆悜锛堜娇鐢ㄨ缃腑鐨勭伒鏁忓害锛?        // InputManager 杩斿洖 f64 (GLFW)锛岃浆鎹负 f32 鐢ㄤ簬鍐呴儴璁＄畻
        f32 sensitivity = m_settings.mouseSensitivity.get() * 0.2f;
        f32 deltaYaw = static_cast<f32>(m_input.mouseDeltaX()) * sensitivity;
        f32 deltaPitch = static_cast<f32>(m_input.mouseDeltaY()) * sensitivity;
        m_player->rotate(deltaYaw, -deltaPitch);  // pitch鏂瑰悜鐩稿弽

        // 鏀堕泦绉诲姩杈撳叆
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

        // 浼犻€掕緭鍏ョ粰鐜╁
        m_player->handleMovementInput(forward, strafe, jumping, sneaking);

        // 婊氳疆閫夋嫨鐗╁搧鏍忔Ы浣嶏紙scrollDeltaY 杩斿洖 f64锛?
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
    // 鏇存柊缃戠粶瀹㈡埛绔紙澶勭悊鏈嶅姟绔暟鎹寘锛?
    if (m_networkClient) {
        m_networkClient->poll();
    }

    // 鏇存柊鐮村潖杩涘害绠＄悊鍣?
    {
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().tick(deltaTime, static_cast<u64>(m_world.gameTime()));
    }

    if (m_renderer && m_renderer->isFirstPersonRendererInitialized()) {
        m_renderer->firstPersonRenderer().tick();
    }

    // 鏇存柊鐜╁鐗╃悊
    if (m_player) {
        // 搴旂敤鐗╃悊锛堥噸鍔涖€佺鎾炴娴嬶級
        m_player->updatePhysics();

        // 澶勭悊鑴氭澹板拰娓告吵澹?        // updateMoveDistance 鍦?Player::updatePhysics 涓皟鐢?        // 杩欓噷妫€鏌ユ槸鍚﹂渶瑕佹挱鏀惧０闊?
        if (m_audioService && !m_player->isSilent()) {
            // 澶勭悊娓告吵澹?
            if (m_player->shouldPlaySwimSound()) {
                auto swimSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createLocated(
                        ResourceLocation("minecraft:entity.player.swim"),
                        sound::SoundCategory::Players,
                        m_player->x(), m_player->y(), m_player->z(),
                        m_player->swimSoundVolume() * 0.15f,  // MC 闊抽噺绯绘暟
                        1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f  // 闅忔満闊宠皟鍙樺寲
                    )
                );
                m_audioService->play(std::move(swimSound));
            }
            // 澶勭悊鑴氭澹?
            else if (m_player->shouldPlayStepSound()) {
                // 鑾峰彇鑴氫笅鏂瑰潡鐨?BlockState 鏉ラ€夋嫨姝ｇ‘鐨勫０闊?
                const auto* blockState = m_world.getBlockState(
                    m_player->stepSoundPos().x,
                    m_player->stepSoundPos().y,
                    m_player->stepSoundPos().z
                );

                if (blockState) {
                    const auto& soundType = blockState->getSoundType();
                    const auto& stepSoundId = soundType.getStepSound();

                    auto stepSound = std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(
                            stepSoundId,
                            sound::SoundCategory::Players,
                            m_player->x(), m_player->y(), m_player->z(),
                            soundType.getVolume() * 0.15f,  // MC 闊抽噺绯绘暟
                            soundType.getPitch() * (0.8f + m_random.nextFloat() * 0.4f)  // 闊宠皟闅忔満鍙樺寲
                        )
                    );
                    m_audioService->play(std::move(stepSound));
                } else {
                    // 榛樿浣跨敤鐭冲ご鑴氭澹?
                    auto stepSound = std::make_unique<sound::SoundInstance>(
                        sound::SoundInstance::createLocated(
                            ResourceLocation("minecraft:block.stone.step"),
                            sound::SoundCategory::Players,
                            m_player->x(), m_player->y(), m_player->z(),
                            0.15f,
                            1.0f + (m_random.nextFloat() - m_random.nextFloat()) * 0.4f
                        )
                    );
                    m_audioService->play(std::move(stepSound));
                }
            }
        }

        // 璁＄畻瑙嗛噹鏅冨姩
        // 鍙傝€?MC 1.16.5 GameRenderer.getCameraPosition()
        f32 bobX = 0.0f;
        f32 bobY = 0.0f;

        if (m_settings.viewBobbing.get()) {
            // 鑾峰彇绉诲姩璺濈鍙樺寲
            f32 distanceWalked = m_player->moveDistanceWalked();
            f32 prevDistanceWalked = m_player->prevMoveDistanceWalked();
            f32 distanceSwam = m_player->moveDistanceSwam();
            f32 prevDistanceSwam = m_player->prevMoveDistanceSwam();

            // 璁＄畻璺濈宸?
            f32 walkedDelta = distanceWalked - prevDistanceWalked;
            f32 swamDelta = distanceSwam - prevDistanceSwam;

            // 绱鏅冨姩瑙掑害
            m_bobAngle += walkedDelta * 0.5f;
            m_bobPhase += swamDelta * 0.5f;

            // 璁＄畻 X 鏅冨姩锛堝乏鍙虫憜鍔級
            bobX = std::sin(m_bobAngle) * 0.1f;

            // 璁＄畻 Y 鏅冨姩锛堜笂涓嬫檭鍔級
            // MC 浣跨敤 cos 鐨勭粷瀵瑰€兼潵浜х敓涓婁笅鏅冨姩
            bobY = std::abs(std::cos(m_bobAngle)) * 0.1f;

            // 娓告吵鏃剁殑棰濆鏅冨姩
            if (m_player->isSwimming()) {
                // 娓告吵鏃舵湁棰濆鐨勫乏鍙虫檭鍔?
                bobX += std::sin(m_bobPhase * 2.0f) * 0.05f;
                bobY += std::abs(std::cos(m_bobPhase)) * 0.05f;
            }
        }

        // 鍚屾鐩告満浣嶇疆鍒扮帺瀹剁溂鐫涗綅缃紙甯﹁閲庢檭鍔ㄥ亸绉伙級
        m_camera.setPosition(
            static_cast<f32>(m_player->x()) + bobX,
            static_cast<f32>(m_player->y() + m_player->eyeHeight()) - bobY,
            static_cast<f32>(m_player->z())
        );
        m_camera.setYaw(m_player->yaw());
        m_camera.setPitch(m_player->pitch());
        m_camera.update(deltaTime);

        // 鏇存柊澹伴煶绯荤粺鍚€呬綅缃?
        if (m_audioService) {
            m_audioService->updateListener(
                m_camera.position(),
                m_camera.forward(),
                m_camera.up()
            );
        }
    } else {
        // 鍚庡锛氭洿鏂扮浉鏈烘帶鍒跺櫒锛堣繖浼氳皟鐢?Camera::update 鏇存柊鐭╅樀锛?
        m_cameraController.update(deltaTime);
    }

    // 鍙戦€佺帺瀹朵綅缃埌鏈嶅姟绔紙闄愬埗棰戠巼锛?
    if (m_networkClient && m_networkClient->isLoggedIn() && m_player) {
        m_positionSendAccumulator += deltaTime;
        if (m_positionSendAccumulator >= POSITION_SEND_INTERVAL) {
            m_positionSendAccumulator = 0.0f;
            sendPlayerPosition();
        }
    }

    // 鏇存柊 ScreenStackWidget 鐨?partialTick 鍜岄紶鏍囦綅缃紙鐢ㄤ簬 IScreen::render锛?
    auto* screenStack = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ScreenStackWidget*>(m_kageroEngine->getLayer(m_screenStackLayerId)) : nullptr;
    if (screenStack) {
        screenStack->setPartialTick(0.0f);  // TODO: 浣跨敤瀹為檯鐨?partialTick
        screenStack->setMousePosition(
            static_cast<i32>(m_input.mouseX() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1))),
            static_cast<i32>(m_input.mouseY() / static_cast<f64>(std::max(m_guiScaleState.scaleFactor, 1)))
        );
    }

    // 鏇存柊 KageroEngine
    if (m_kageroEngine) {
        m_kageroEngine->update(deltaTime);
    }

    // 鏇存柊璋冭瘯灞忓箷鏁版嵁
    auto* debugWidget = m_kageroEngine ?
        static_cast<ui::minecraft::DebugScreenWidget*>(m_kageroEngine->getLayer(m_debugScreenLayerId)) : nullptr;
    auto* targetInfoWidget = m_kageroEngine ?
        static_cast<ui::minecraft::targetinfo::TargetInfoWidget*>(m_kageroEngine->getLayer(m_targetInfoLayerId)) : nullptr;

    // 鎵ц灏勭嚎妫€娴?
    if (m_player && m_mouseCaptured) {
        // 鑾峰彇鐜╁鐪肩潧浣嶇疆
        glm::vec3 eyePos = m_camera.position();

        // 鑾峰彇瑙嗙嚎鏂瑰悜
        glm::vec3 forward = m_camera.forward();

        // 鍒涘缓灏勭嚎
        mc::Vector3 origin(eyePos.x, eyePos.y, eyePos.z);
        mc::Vector3 direction(forward.x, forward.y, forward.z);
        mc::Ray ray(origin, direction);

        // 鎵ц灏勭嚎妫€娴嬶紙鍒涢€犳ā寮忎娇鐢ㄦ洿杩滅殑璺濈锛?
        mc::RaycastContext raycastContext{ray, 5.0f};  // 鐢熷瓨妯″紡5鏍?
        ClientWorldBlockReader blockReader(m_world);
        m_raycastResult = mc::raycastBlocks(raycastContext, blockReader);

        if (targetInfoWidget) {
            targetInfoWidget->setTargetInfo(
                ui::minecraft::targetinfo::TargetInfoResolver::resolve(
                    origin,
                    direction,
                    m_world,
                    m_world.entityManager(),
                    m_raycastResult,
                    5.0f,
                    [this](EntityId entityId) -> String {
                        const auto it = m_knownPlayerNames.find(static_cast<PlayerId>(entityId));
                        if (it == m_knownPlayerNames.end()) {
                            return {};
                        }
                        return it->second;
                    }));
        }

        // 鏇存柊璋冭瘯灞忓箷鐨勭洰鏍囨柟鍧?
        if (debugWidget) {
            debugWidget->setTargetBlock(&m_raycastResult);
        }
    } else {
        // 娌℃湁鐜╁鏃舵竻闄ょ洰鏍囨柟鍧?
        if (debugWidget) {
            debugWidget->setTargetBlock(nullptr);
        }
        if (targetInfoWidget) {
            targetInfoWidget->setTargetInfo(ui::minecraft::targetinfo::TargetInfoSnapshot::none());
        }
    }

    handleBlockInteractionInput(deltaTime);

    // 澶勭悊鏂瑰潡鏀剧疆杈撳叆
    handleBlockPlacementInput(deltaTime);

    MeshSchedulerViewState meshViewState;
    meshViewState.cameraPosition = glm::vec3(m_camera.position());
    meshViewState.cameraForward = glm::vec3(m_camera.forward());
    meshViewState.viewProjectionMatrix = m_camera.viewProjectionMatrix();
    meshViewState.renderDistanceChunks = m_settings.renderDistance.get();
    meshViewState.minBuildHeight = m_world.getMinBuildHeight();
    meshViewState.maxBuildHeight = m_world.getMaxBuildHeight();

    m_world.update(meshViewState);

    // 鏇存柊瀹㈡埛绔疄浣擄紙姣弔ick璋冪敤锛?
    m_world.entityManager().tick();

    // 鏇存柊澹伴煶绯荤粺
    if (m_audioService) {
        // 妫€鏌ユ槸鍚︽殏鍋滐紙娓告垙鏆傚仠鏃朵笉鏇存柊澹伴煶锛?
        bool isPaused = !m_mouseCaptured;  // 榧犳爣鏈崟鑾锋椂璁や负娓告垙鏆傚仠
        m_audioService->setPaused(isPaused);
    }

    // 鏇存柊瀹炰綋鍔ㄧ敾鐘舵€侊紙鐢ㄤ簬娓叉煋鎻掑€硷級
    constexpr f32 partialTick = 0.0f;  // TODO: 浠庝富寰幆鑾峰彇瀹為檯鐨勯儴鍒唗ick
    m_world.entityManager().updateAnimations(partialTick);

    // 澶勭悊寮傛缃戞牸鏋勫缓缁撴灉
    m_world.processMeshBuildResults(4);  // 姣忓抚鏈€澶氬鐞?4 涓綉鏍?
    // 鍚屾鏃堕棿鍒版覆鏌撳櫒锛堥┍鍔ㄥぉ绌恒€佸お闃炽€佹湀浜€佹槦绌哄彉鍖栵級
    // 瀹㈡埛绔瘡甯у钩婊戞帹杩涙椂闂达紝鍚屾椂鍦ㄦ敹鍒版湇鍔＄鍚屾鏃剁籂姝?
    if (m_renderer) {
        constexpr i64 DAY_LENGTH_TICKS = 24000;

        // 姣忓抚鎺ㄨ繘鏃堕棿锛堟棤璁烘槸鍚︽湁鏈嶅姟绔悓姝ワ級
        // 杩欑‘淇濆ぉ绌恒€佸お闃炽€佹湀浜湪姣忓抚骞虫粦鍙樺寲
        m_renderTickAccumulator += deltaTime * 20.0f;
        while (m_renderTickAccumulator >= 1.0f) {
            m_renderTickAccumulator -= 1.0f;
            ++m_renderGameTime;
            m_renderDayTime = (m_renderDayTime + 1) % DAY_LENGTH_TICKS;
        }

        // 褰撴湁鏈嶅姟绔悓姝ユ椂锛岄€愭笎绾犳鍒版湇鍔＄鏃堕棿锛堥伩鍏嶈烦鍙橈級
        if (m_hasServerTimeSync) {
            const i64 serverDayTime = m_world.dayTime();
            const i64 serverGameTime = m_world.gameTime();

            // 璁＄畻鏃堕棿宸紙澶勭悊 dayTime 寰幆锛?
            i64 dayTimeDiff = serverDayTime - m_renderDayTime;
            if (dayTimeDiff > DAY_LENGTH_TICKS / 2) {
                dayTimeDiff -= DAY_LENGTH_TICKS;
            } else if (dayTimeDiff < -DAY_LENGTH_TICKS / 2) {
                dayTimeDiff += DAY_LENGTH_TICKS;
            }

            // 骞虫粦绾犳锛氭瘡甯х籂姝ｅ樊鍊肩殑 1%锛岄伩鍏嶈烦鍙橈紙TODO:鏍规嵁鐢ㄦ埛璁惧畾鐨勫抚鐜囪皟鏁寸籂姝ｇ巼銆侰ORRECTION_RATE = 1 / 鐢ㄦ埛璁惧畾FPS锛?
            constexpr f32 CORRECTION_RATE = 0.01f;
            const i64 correction = static_cast<i64>(dayTimeDiff * CORRECTION_RATE);
            if (correction != 0) {
                m_renderDayTime = (m_renderDayTime + correction + DAY_LENGTH_TICKS) % DAY_LENGTH_TICKS;
            }

            // gameTime 鍚屾绾犳
            i64 gameTimeDiff = serverGameTime - m_renderGameTime;
            m_renderGameTime += static_cast<i64>(gameTimeDiff * CORRECTION_RATE);
        }

        m_renderer->updateTime(m_renderDayTime, m_renderGameTime, m_renderTickAccumulator);

        // 鏇存柊澶╂皵鐘舵€佸埌娓叉煋鍣?
        m_renderer->updateWeather(
            m_world.weather().rainStrength(m_renderTickAccumulator),
            m_world.weather().thunderStrength(m_renderTickAccumulator)
        );

        // 鏇存柊娑蹭綋鐘舵€?
        if (m_player) {
            bool inWater = m_player->isInWater();
            bool inLava = m_player->isInLava();
            u32 waterFogColor = world::biome::BiomeEffects::DEFAULT_WATER_FOG_COLOR;

            // 鑾峰彇褰撳墠鐢熺墿缇ょ郴鐨勬按涓嬮浘棰滆壊
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

            // 鍏ユ按/鍑烘按闊虫晥瑙﹀彂
            if (m_audioService && inWater && !m_wasPlayerInWater) {
                MC_TRACE_INSTANT("client.entity", "EnterWater");
                // 鍏ユ按闊虫晥
                auto enterSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createGlobal(
                        ResourceLocation("minecraft:ambient.underwater.enter"),
                        sound::SoundCategory::Ambient,
                        1.0f,  // 闊抽噺
                        1.0f   // 闊宠皟
                    )
                );
                m_audioService->play(std::move(enterSound));
            } else if (m_audioService && !inWater && m_wasPlayerInWater) {
                // 鍑烘按闊虫晥
                auto exitSound = std::make_unique<sound::SoundInstance>(
                    sound::SoundInstance::createGlobal(
                        ResourceLocation("minecraft:ambient.underwater.exit"),
                        sound::SoundCategory::Ambient,
                        1.0f,  // 闊抽噺
                        1.0f   // 闊宠皟
                    )
                );
                m_audioService->play(std::move(exitSound));
            }

            // 鏇存柊姘翠笅鐜闊虫晥澶勭悊鍣?
            if (m_audioService) {
                const auto* biome = m_world.getBiomeAtBlock(
                    static_cast<i32>(std::floor(m_player->x())),
                    static_cast<i32>(std::floor(m_player->y() + m_player->eyeHeight())),
                    static_cast<i32>(std::floor(m_player->z()))
                );
                m_audioService->setBiomeId(biome ? static_cast<u32>(biome->id()) : 0u);
                m_audioService->setUnderwater(inWater);
            }

            m_wasPlayerInWater = inWater;
            m_wasPlayerInLava = inLava;
        }
    }

    // 涓婁紶缃戞牸鍒?GPU锛堝彧澶勭悊宸插畬鎴愬紓姝ユ瀯寤虹殑缃戞牸锛?
    if (m_renderer->isChunkRendererInitialized()) {
        auto& chunkRenderer = m_renderer->chunkRenderer();
        m_world.forEachDirtyMesh([&chunkRenderer](const ChunkId& id, ClientChunk& chunk) {
            // 涓ゅ眰閮戒负绌烘椂锛屾竻鐞?GPU 缂撳啿骞剁粨鏉熸湰娆℃洿鏂般€?
            if (chunk.solidMesh.empty() && chunk.transparentMesh.empty()) {
                chunkRenderer.removeChunk(id);
                chunk.needsMeshUpdate = false;
                return;
            }

            // 涓婁紶鍙屽眰缃戞牸鍒?GPU
            auto result = chunkRenderer.updateChunk(id, chunk.solidMesh, chunk.transparentMesh);
            if (result.success()) {
                chunk.needsMeshUpdate = false;

                // 涓婁紶鎴愬姛鍚庨噴鏀?CPU 渚х綉鏍肩紦瀛橈紝閬垮厤涓?GPU 鏁版嵁閲嶅鍗犵敤鍐呭瓨銆?
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

    // 淇濆瓨璁剧疆
    const auto savePath = m_settingsPath.empty()
        ? ClientSettings::getSettingsPath("minecraft-reborn")
        : m_settingsPath;
    auto saveResult = m_settings.saveSettings(savePath);
    if (saveResult.failed()) {
        spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
    }

    // 鍏抽棴澹伴煶绯荤粺
    if (m_audioService) {
        m_audioService->shutdown();
        m_audioService.reset();
    }

    // 鏂紑缃戠粶杩炴帴
    if (m_networkClient) {
        m_networkClient->disconnect("Client shutdown");
        m_networkClient.reset();
    }

    // 鍋滄鍐呯疆鏈嶅姟绔?
    if (m_integratedServer) {
        m_integratedServer->stop();
        m_integratedServer.reset();
    }

    // 鍏堟竻鐞嗕緷璧栨覆鏌撹祫婧愮殑 UI/鍥鹃泦瀵硅薄锛岄伩鍏嶅湪娓叉煋鍣ㄩ攢姣佸悗鏋愭瀯璁块棶鏃犳晥璧勬簮
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

    // 娓呯悊娓叉煋鍣?
    if (m_renderer) {
        m_renderer->destroy();
        m_renderer.reset();
    }

    // 娓呯悊鐜╁
    m_player.reset();
    m_physicsEngine.reset();

    // 娓呯悊涓栫晫锛堝寘鎷叧闂綉鏍兼瀯寤虹嚎绋嬫睜锛?
    m_world.destroy();

    m_window.destroy();

    // 鍏抽棴鎬ц兘杩借釜
    mc::perfetto::PerfettoManager::instance().stopTracing();
    mc::perfetto::PerfettoManager::instance().shutdown();
    spdlog::info("Perfetto tracing stopped");

    spdlog::info("Client stopped.");
}

// 璁剧疆鐩稿叧鏂规硶

Result<void> ClientApplication::loadSettings(const String& path)
{
    MC_TRACE_EVENT("client.initialization", "LoadSettings", "path", path);

    m_settingsPath = std::filesystem::path(path);

    auto result = m_settings.loadSettings(path);
    if (result.failed()) {
        return result;
    }

    // 纭繚璁剧疆鐩綍瀛樺湪锛堜娇鐢ㄥ綋鍓嶅疄闄呰缃矾寰勶紝閬垮厤鍐欏埌榛樿鐩綍锛?
    const auto settingsDir = m_settingsPath.parent_path();
    if (!settingsDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(settingsDir, ec);
        if (ec) {
            spdlog::warn("Failed to create settings directory: {}", settingsDir.string());
        }
    }

    // 鍚敤鑷姩淇濆瓨
    m_settings.enableAutoSave(m_settingsPath);

    spdlog::info("Client settings path: {}", m_settingsPath.string());

    return Result<void>::ok();
}

void ClientApplication::applySettings()
{
    MC_TRACE_EVENT("client.initialization", "ApplySettings");

    // 璁剧疆鍙樻洿鍥炶皟鍦?setupSettingCallbacks 涓缃?    // 杩欓噷搴旂敤鍒濆璁剧疆鍊?
    // 鍚屾娓叉煋鍣ㄨ繍琛屾椂鍙傛暟
    if (m_renderer) {
        m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());
        m_renderer->setLandFogDensity(m_settings.fogDensity.get());
        m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(m_settings.clouds.get()));
    }

    // 搴旂敤鍏夌収妯″紡锛堢幆澧冨厜閬斀锛?
    ChunkMesher::syncFromSettings(static_cast<client::AmbientOcclusionMode>(
        m_settings.ambientOcclusion.get()));

    // 搴旂敤鐢熺墿缇ょ郴棰滆壊娣峰悎鍗婂緞
    ChunkMesher::setBiomeBlendRadius(m_settings.biomeBlendRadius.get());
}

void ClientApplication::setupSettingCallbacks()
{
    MC_TRACE_EVENT("client.initialization", "SetupSettingCallbacks");

    // 娓叉煋璺濈鍙樻洿
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
        // 涓栫晫鏇存柊鏃朵細浣跨敤鏂板€?
        });

    // 鍏ㄥ睆妯″紡鍙樻洿
    m_settings.fullscreen.onChange([this](bool value) {
        spdlog::info("Fullscreen changed to: {}", value);
        m_window.setFullscreen(value);
    });

    m_settings.guiScale.onChange([this](i32 value) {
        spdlog::info("GUI scale changed to: {}", value);
        applyGuiScale();
    });

    // VSync 鍙樻洿
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

    // 浜戞ā寮忓彉鏇?
    m_settings.clouds.onChange([this](u8 value) {
        spdlog::info("Cloud mode changed to: {}", static_cast<i32>(value));
        if (m_renderer) {
            m_renderer->setCloudMode(static_cast<renderer::trident::cloud::CloudMode>(value));
        }
    });

    // 榧犳爣鐏垫晱搴﹀彉鏇?
    m_settings.mouseSensitivity.onChange([this](f32 value) {
        spdlog::info("Mouse sensitivity changed to: {}", value);
        // 榧犳爣鐏垫晱搴﹀湪 handleEvents 涓簲鐢?
        });

    // FOV 鍙樻洿
    m_settings.fov.onChange([this](f32 value) {
        spdlog::info("FOV changed to: {}", value);
        m_camera.setFOV(value);
    });

    // 闆炬皵瀵嗗害鍙樻洿
    m_settings.fogDensity.onChange([this](f32 value) {
        spdlog::info("Fog density changed to: {}", value);
        if (m_renderer) {
            m_renderer->setLandFogDensity(value);
        }
    });

    // 鍏夌収妯″紡锛堢幆澧冨厜閬斀锛夊彉鏇?
    m_settings.ambientOcclusion.onChange([this](u8 value) {
        auto mode = static_cast<client::AmbientOcclusionMode>(value);
        spdlog::info("Ambient occlusion changed to: {}", static_cast<i32>(mode));
        ChunkMesher::syncFromSettings(mode);
    });

    // 鐢熺墿缇ょ郴棰滆壊娣峰悎鍗婂緞鍙樻洿
    m_settings.biomeBlendRadius.onChange([this](i32 value) {
        spdlog::info("Biome blend radius changed to: {} ({}x{} area)",
                     value, value * 2 + 1, value * 2 + 1);
        ChunkMesher::setBiomeBlendRadius(value);
    });

    m_settings.antiAliasing.onChange([](bool enabled) {
        spdlog::info("Anti-aliasing changed to: {} (restart required)", enabled);
    });
}

// 杈呭姪鍑芥暟瀹炵幇

void ClientApplication::setupInputBindings()
{
    MC_TRACE_EVENT("client.initialization", "SetupInputBindings");

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
    MC_TRACE_EVENT("client.initialization", "SetupCamera");

    // 璁剧疆鐩告満閰嶇疆
    CameraConfig cameraConfig;
    cameraConfig.fov = m_settings.fov.get();
    cameraConfig.aspectRatio = static_cast<f32>(m_window.width()) / static_cast<f32>(m_window.height());
    cameraConfig.nearPlane = 0.1f;
    cameraConfig.farPlane = 1000.0f;
    cameraConfig.moveSpeed = 10.0f;      // 绉诲姩閫熷害
    cameraConfig.mouseSensitivity = m_settings.mouseSensitivity.get() * 0.2f; // 榧犳爣鐏垫晱搴?
    m_camera = Camera(cameraConfig);

    // 璁剧疆鍒濆浣嶇疆锛堝湪娴嬭瘯鍖哄潡涓婃柟锛?
    m_camera.setPosition(8.0f, 50.0f, 8.0f);
    m_camera.setYaw(45.0f);
    m_camera.update(0.0f);

    // 璁剧疆鐩告満鎺у埗鍣?
    m_cameraController.setCamera(&m_camera);
}

    void ClientApplication::applyGuiScale()
    {
        m_guiScaleState = ui::calculateGuiScale(
            m_settings.guiScale.get(),
            static_cast<i32>(m_window.width()),
            static_cast<i32>(m_window.height()));

        if (m_renderer) {
            m_renderer->setGuiScaleFactor(static_cast<f64>(m_guiScaleState.scaleFactor));
        }

        if (m_canvas) {
            m_canvas->resize(m_guiScaleState.width, m_guiScaleState.height);
        }

        if (m_kageroEngine) {
            m_kageroEngine->resize(m_guiScaleState.width, m_guiScaleState.height);
        }
    }

void ClientApplication::setupNetworkCallbacks()
{
    MC_TRACE_EVENT("client.initialization", "SetupNetworkCallbacks");

    if (!m_networkClient) return;

    NetworkClientCallbacks callbacks;

    callbacks.onLoginSuccess = [this](PlayerId playerId, const String& username) {
        spdlog::info("Login successful: playerId={}, username={}", playerId, username);
        if (m_player) {
            m_player->setPlayerId(playerId);
        }
        m_knownPlayerNames[playerId] = username;
    };

    callbacks.onLoginFailed = [this](const String& reason) {
        spdlog::error("Login failed: {}", reason);

        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        stop();
    };

    callbacks.onDisconnected = [this](const String& reason) {
        spdlog::warn("Disconnected: {}", reason);
        m_knownPlayerNames.clear();
        if (m_commandManager) {
            m_commandManager->clear();
        }
        if (m_useIntegratedServer) {
            stop();
        }
    };

    callbacks.onCommandTree = [this](const String& treeJson) {
        if (!m_commandManager) {
            return;
        }

        auto result = m_commandManager->applyCommandTreeJson(treeJson);
        if (result.failed()) {
            spdlog::error("Failed to apply command tree: {}", result.error().toString());
        }
    };

    callbacks.onChunkData = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension, const std::vector<u8>& data) {
        MC_UNUSED(dimension);  // TODO: 澶氱淮搴︽敮鎸?
        m_world.onChunkData(x, z, std::vector<u8>(data));
    };

    callbacks.onChunkUnload = [this](ChunkCoord x, ChunkCoord z, DimensionId dimension) {
        MC_UNUSED(dimension);  // TODO: 澶氱淮搴︽敮鎸?
        m_world.onChunkUnload(x, z);
    };

    callbacks.onTeleport = [this](f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 /*teleportId*/) {
        if (m_player) {
            // 缃戠粶鍗忚浣跨敤 f64锛屽唴閮ㄤ娇鐢?f32
            m_player->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
            m_player->setRotation(yaw, pitch);
        }
        m_bobAngle = 0.0f;
        m_bobPhase = 0.0f;
        m_camera.setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        m_camera.setYaw(yaw);
        m_camera.setPitch(pitch);
        m_camera.update(0.0f);  // 绔嬪嵆鏇存柊鐩告満鐭╅樀
        // 娉ㄦ剰锛歴endTeleportConfirm 宸插湪 NetworkClient::handleTeleport 涓皟鐢?
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
        const size_t maxSlots = std::min(items.size(), static_cast<size_t>(PlayerInventory::TOTAL_SIZE));
        for (size_t slot = 0; slot < maxSlots; ++slot) {
            m_player->inventory().setItem(static_cast<i32>(slot), items[slot]);
        }
        m_player->inventory().setSelectedSlot(selectedSlot);
    };

    callbacks.onOpenContainer = [this](const OpenContainerPacket& packet) {
        if (!m_player) {
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

        auto configureScreen = [this](auto& screen) {
            if (m_renderer != nullptr && m_renderer->isGuiRendererInitialized()) {
                screen.setRenderers(
                    &m_renderer->guiRenderer(),
                    m_guiTextureManager.get(),
                    m_renderer->isItemRendererInitialized() ? &m_renderer->itemRenderer() : nullptr);
                screen.setScreenSize(m_guiScaleState.width, m_guiScaleState.height);
            }
        };

        releaseMouseForScreen(m_input, m_mouseCaptured);

        const ContainerType type = static_cast<ContainerType>(packet.type());
        switch (type) {
            case ContainerType::CraftingTable: {
                auto screen = std::make_unique<CraftingScreen>(
                    std::make_unique<CraftingMenu>(packet.containerId(), &m_player->inventory(), nullptr),
                    clickSender,
                    closeSender);
                configureScreen(*screen);
                ScreenManager::instance().openScreen(std::move(screen));
                break;
            }
            case ContainerType::Chest: {
                i32 rows = (packet.slotCount() == mc::blockentity::ChestContainer::DOUBLE_CHEST_ROWS * mc::blockentity::ChestContainer::SLOTS_PER_ROW)
                    ? mc::blockentity::ChestContainer::DOUBLE_CHEST_ROWS
                    : mc::blockentity::ChestContainer::SINGLE_CHEST_ROWS;

                auto screen = std::make_unique<ChestScreen>(
                    packet.containerId(),
                    &m_player->inventory(),
                    rows,
                    clickSender,
                    closeSender);
                configureScreen(*screen);
                ScreenManager::instance().openScreen(std::move(screen));
                break;
            }
            case ContainerType::Furnace: {
                auto screen = std::make_unique<FurnaceScreen>(
                    packet.containerId(),
                    &m_player->inventory(),
                    clickSender,
                    closeSender);
                configureScreen(*screen);
                ScreenManager::instance().openScreen(std::move(screen));
                break;
            }
            default:
                spdlog::info("Unhandled open container type {}", packet.type());
                return;
        }
    };

    callbacks.onContainerContent = [this](const ContainerContentPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (isMatchingContainerScreen<CraftingScreen>(screen, packet.containerId())) {
            applyContainerContents(dynamic_cast<CraftingScreen*>(screen)->getMenu(), packet.items());
        } else if (isMatchingContainerScreen<ChestScreen>(screen, packet.containerId())) {
            applyContainerContents(dynamic_cast<ChestScreen*>(screen)->getMenu(), packet.items());
        } else if (isMatchingContainerScreen<FurnaceScreen>(screen, packet.containerId())) {
            applyContainerContents(dynamic_cast<FurnaceScreen*>(screen)->getMenu(), packet.items());
        }
    };

    callbacks.onContainerSlot = [this](const ContainerSlotPacket& packet) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (isMatchingContainerScreen<CraftingScreen>(screen, packet.containerId())) {
            applyContainerSlot(dynamic_cast<CraftingScreen*>(screen)->getMenu(), packet.slotIndex(), packet.item());
        } else if (isMatchingContainerScreen<ChestScreen>(screen, packet.containerId())) {
            applyContainerSlot(dynamic_cast<ChestScreen*>(screen)->getMenu(), packet.slotIndex(), packet.item());
        } else if (isMatchingContainerScreen<FurnaceScreen>(screen, packet.containerId())) {
            applyContainerSlot(dynamic_cast<FurnaceScreen*>(screen)->getMenu(), packet.slotIndex(), packet.item());
        }
    };

    callbacks.onCloseContainer = [this](ContainerId containerId) {
        IScreen* screen = ScreenManager::instance().getCurrentScreen();
        if (isMatchingContainerScreen<CraftingScreen>(screen, containerId) ||
            isMatchingContainerScreen<ChestScreen>(screen, containerId) ||
            isMatchingContainerScreen<FurnaceScreen>(screen, containerId)) {
            ScreenManager::instance().closeScreen();
            if (!ScreenManager::instance().hasScreen()) {
                captureMouseAfterScreens(m_input, m_mouseCaptured);
            }
        }
    };

    // ========== 瀹炰綋浜嬩欢鍥炶皟 ==========
    callbacks.onSpawnMob = [this](u32 entityId, const String& typeId,
                                   f32 x, f32 y, f32 z,
                                   f32 yaw, f32 pitch, f32 headYaw) {
        auto* entity = m_world.entityManager().spawnEntity(
            static_cast<EntityId>(entityId), typeId);
        if (entity) {
            entity->setPosition(x, y, z);
            entity->setRotation(yaw, pitch);
            entity->setHeadRotation(headYaw);
            // spdlog::info("Client received SpawnMob: {} (ID: {}) at ({:.1f}, {:.1f}, {:.1f})", typeId, entityId, x, y, z);
        }
    };

    callbacks.onPlayerSpawn = [this](PlayerId playerId, const String& username, f64 x, f64 y, f64 z) {
        m_knownPlayerNames[playerId] = username;

        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(playerId));
        if (entity == nullptr) {
            entity = m_world.entityManager().spawnEntity(static_cast<EntityId>(playerId), "player");
        }

        if (entity != nullptr) {
            entity->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
        }
    };

    callbacks.onPlayerDespawn = [this](PlayerId playerId) {
        m_knownPlayerNames.erase(playerId);
        m_world.entityManager().removeEntity(static_cast<EntityId>(playerId));
    };

    callbacks.onEntityMetadata = [this](u32 entityId, const std::vector<u8>& metadata) {
        auto* entity = m_world.entityManager().getEntity(static_cast<EntityId>(entityId));
        if (entity) {
            entity->setMetadata(metadata);
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

            // 濡傛灉鏄?ItemEntity锛岃缃墿鍝佹暟鎹?
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
            // 閫熷害鍗曚綅杞崲: 1/8000 block/tick -> block/tick
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
        // TODO: 澶勭悊瀹炰綋鍔ㄧ敾锛堟尌鎵嬬瓑锛?        (void)entityId;
    };

    callbacks.onEntityStatus = [this](u32 entityId, u8 /*status*/) {
        // TODO: 澶勭悊瀹炰綋鐘舵€侊紙鍙椾激銆佹浜＄瓑锛?        (void)entityId;
    };

    // ========== 澶╂皵鍥炶皟 ==========
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

    // ========== 娓告垙妯″紡鍥炶皟 ==========
    callbacks.onGameModeChange = [this](GameMode mode) {
        spdlog::info("Game mode changed to {}", static_cast<int>(mode));
        // 鏇存柊鏈湴鐜╁鐨勬父鎴忔ā寮?
        if (m_player) {
            m_player->setGameMode(mode);
        }
    };

    // ========== 鐜╁鑳藉姏鍥炶皟 ==========
    callbacks.onPlayerAbilities = [this](bool invulnerable, bool flying, bool canFly, bool creativeMode, f32 flySpeed, f32 walkSpeed) {
        spdlog::debug("Player abilities updated: invulnerable={}, flying={}, canFly={}, creativeMode={}",
                      invulnerable, flying, canFly, creativeMode);
        // 鏇存柊鏈湴鐜╁鑳藉姏
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

    // ========== 鍏夌収鏇存柊鍥炶皟 ==========
    callbacks.onLightUpdate = [this](i32 chunkX, i32 chunkZ, i32 sectionY,
                                      const std::vector<u8>& skyLight,
                                      const std::vector<u8>& blockLight,
                                      bool trustEdges) {
        m_world.onLightUpdate(chunkX, chunkZ, sectionY, skyLight, blockLight, trustEdges);
    };

    // ========== 鏂瑰潡鐮村潖鍔ㄧ敾鍥炶皟 ==========
    callbacks.onBlockBreakAnim = [this](EntityId breakerEntityId, i32 x, i32 y, i32 z, i8 stage) {
        // 浣跨敤 BreakProgressManager 鏇存柊杩滅▼鐜╁鐨勬寲鎺樿繘搴?
        using namespace mc::client::renderer::trident::block;
        auto& manager = BreakProgressManager::instance();

        BlockPos pos(x, y, z);
        u64 currentTick = static_cast<u64>(m_world.gameTime());  // 浣跨敤涓栫晫鏃堕棿

        if (stage < 0) {
            // stage = -1 琛ㄧず绉婚櫎鐮村潖鏁堟灉
            manager.removeRemoteProgress(breakerEntityId);
        } else {
            // stage = 0-9 琛ㄧず鐮村潖闃舵
            manager.updateRemoteProgress(breakerEntityId, pos, stage, currentTick);
        }
    };

    // ========== 澹伴煶鍥炶皟 ==========
    callbacks.onPlaySound = [this](const ResourceLocation& soundEventId,
                                   mc::sound::SoundCategory category,
                                   f32 x,
                                   f32 y,
                                   f32 z,
                                   f32 volume,
                                   f32 pitch) {
        if (!m_audioService) {
            spdlog::warn("Received sound event '{}' but audio service is not initialized", soundEventId.toString());
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

        m_audioService->play(std::make_unique<sound::SoundInstance>(std::move(sound)));
        // MC_TRACE_CLIENT_SOUND_EVENT("OnPlaySound_Result", "soundId", id);
    };

    callbacks.onStopSound = [this](const Optional<ResourceLocation>& soundEventId,
                                   const Optional<mc::sound::SoundCategory>& category) {
        if (!m_audioService) {
            return;
        }

        if (!soundEventId.has_value() && !category.has_value()) {
            m_audioService->stopAll();
            return;
        }

        if (soundEventId.has_value()) {
            m_audioService->stop(*soundEventId);
            return;
        }

        if (category.has_value()) {
            m_audioService->stop(*category);
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

        // 娓呴櫎鏈湴鐮村潖杩涘害
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().stopBreaking();

        m_breakingBlockActive = false;
        m_breakingBlockProgress = 0.0f;
        m_breakingBlockFace = Direction::None;

        MC_TRACE_INSTANT("client.input.mining", "abortBreakingBlock", "state", static_cast<i32>(MiningInputState::Active), "reason", "abort local breaking state");
    };

    if (!m_mouseCaptured || !m_player) {
        // 濡傛灉鐢ㄦ埛娌℃湁鎸栨帢鎿嶄綔锛岄偅涔堣繖涓矾寰勬瘡甯ч兘浼氳蛋
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

        // 寮€濮嬫寲鎺?- 鏇存柊 BreakProgressManager
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

        // 鍒涢€犳ā寮忕洿鎺ョ牬鍧?
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

    // 鏇存柊鏈湴鐮村潖杩涘害
    {
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().updateLocalProgress(m_breakingBlockPos, m_breakingBlockProgress);
    }

    if (m_breakingBlockProgress >= 1.0f) {
        // 鏂瑰潡琚牬鍧?
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

    // 妫€鏌ュ彸閿槸鍚﹀垰鍒氭寜涓?
    const bool usePressed = m_input.isMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
    if (!usePressed) {
        setPlaceState(PlaceInputState::NoUsePress, "right button not just pressed");
        return;
    }

    // 妫€鏌ユ斁缃喎鍗?
    if (m_placeCooldown > 0.0f) {
        setPlaceState(PlaceInputState::Cooldown, "placement cooldown active");
        return;
    }

    // 妫€鏌ュ皠绾挎槸鍚﹀嚮涓柟鍧?
    if (m_raycastResult.isMiss()) {
        setPlaceState(PlaceInputState::RaycastMiss, "raycast miss on use");
        return;
    }

    setPlaceState(PlaceInputState::Active, "right button pressed and target hit");

    // 璁＄畻鍑讳腑鐐圭浉瀵瑰潗鏍?
    BlockPos pos = m_raycastResult.blockPos();
    Direction face = m_raycastResult.face();
    Vector3 hitPos = m_raycastResult.hitPosition();
    Vector3 blockPosFloat(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
    Vector3 relativeHit = hitPos - blockPosFloat;  // 杞崲涓烘柟鍧楀唴鐩稿鍧愭爣

    // 鍙戦€佹斁缃寘
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

    // 妫€鏌ユ槸鍚﹂渶瑕佸彂閫侊紙浣嶇疆鎴栨棆杞彉鍖栵級
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
        // 鏃犲彉鍖栵紝鍙彂閫佸湴闈㈢姸鎬?
        type = network::PlayerMovePacket::MoveType::GroundOnly;
    }

    m_networkClient->sendPlayerMove(playerPos, type);

    // 鏇存柊涓婃鍙戦€佺殑浣嶇疆
    m_lastSentX = pos.x;
    m_lastSentY = pos.y;
    m_lastSentZ = pos.z;
    m_lastSentYaw = m_player->yaw();
    m_lastSentPitch = m_player->pitch();
}

std::vector<String> ClientApplication::collectPlayerCompletionCandidates() const
{
    std::vector<String> candidates;
    candidates.reserve(m_knownPlayerNames.size() + 1);

    for (const auto& [playerId, playerName] : m_knownPlayerNames) {
        MC_UNUSED(playerId);
        if (!playerName.empty()) {
            candidates.push_back(playerName);
        }
    }

    if (m_player) {
        const auto& username = m_player->username();
        if (!username.empty()) {
            candidates.push_back(username);
        }
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    return candidates;
}

std::vector<String> ClientApplication::collectEntityCompletionCandidates() const
{
    return collectPlayerCompletionCandidates();
}

Result<void> ClientApplication::initializeResources()
{
    // 1. 鍒涘缓 ResourceManager 骞堕鍏堟坊鍔犲唴缃祫婧愬寘锛堟渶浣庝紭鍏堢骇锛?
    m_resourceManager = std::make_unique<ResourceManager>();

    // 娣诲姞鍘熺増鍐呯疆璧勬簮鍖咃紙鎻愪緵鍩虹妯″瀷濡?cube_all, cube_column 绛夛級
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
        spdlog::info("Added vanilla built-in resource pack");
    } else {
        spdlog::warn("Failed to initialize vanilla resource pack: {}", vanillaResult.error().toString());
    }

    // 娣诲姞椤圭洰鍐呯疆璧勬簮鍖咃紙鎻愪緵 sounds.json銆侀煶鏁堟枃浠剁瓑锛?    // 璺緞: resources/data/minecraft/ -> assets/minecraft/
    auto builtinResourcesPack = std::make_shared<FolderResourcePack>("resources/data/minecraft");
    auto builtinResult = builtinResourcesPack->initialize();
    if (builtinResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(builtinResourcesPack));
        spdlog::info("Added built-in resources pack (sounds, etc.)");
    } else {
        spdlog::warn("Failed to initialize built-in resources pack: {}", builtinResult.error().toString());
    }

    // 2. 鎵弿璧勬簮鍖呯洰褰?
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

    // 3. 浠庤缃姞杞借祫婧愬寘閰嶇疆
    m_resourcePackList.loadFromSettings(m_settings.resourcePacks);

    // 4. 娣诲姞鍚敤鐨勮祫婧愬寘锛堝閮ㄨ祫婧愬寘浼樺厛绾ч珮浜庡唴缃級
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        } else {
            spdlog::info("Added resource pack: {}", pack->name());
        }
    }

    // 5. 鍔犺浇鎵€鏈夎祫婧愶紙濡傛灉鏈夎祫婧愬寘锛?
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->loadAllResources();
        if (loadResult.failed()) {
            spdlog::warn("Failed to load resources: {}", loadResult.error().toString());
        } else {
            spdlog::info("Loaded {} resource packs", m_resourceManager->resourcePackCount());
        }

        // 6. 鏋勫缓绾圭悊鍥鹃泦
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

    // 7. 鍒濆鍖?BlockModelCache锛堝嵆浣挎病鏈夎祫婧愬寘涔熻鍒濆鍖栵紝浣跨敤缂哄け妯″瀷锛?
    if (m_modelCache.initialize(*m_resourceManager)) {
        spdlog::info("Block model cache initialized with {} appearances",
                    m_modelCache.cachedAppearanceCount());
        // 璁剧疆 ChunkMesher 浣跨敤 BlockModelCache
        ChunkMesher::setModelCache(&m_modelCache);
    } else {
        spdlog::warn("Failed to initialize block model cache");
    }

    // 8. 璁剧疆璧勬簮鍖呭彉鏇村洖璋?
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
    MC_TRACE_EVENT("client.resource", "ReloadResources");

    if (!m_resourceManager) {
        return;
    }

    // 娓呴櫎璧勬簮绠＄悊鍣?
    m_resourceManager->clearResourcePacks();

    // 棣栧厛娣诲姞鍐呯疆璧勬簮鍖?
    auto vanillaPack = VanillaResources::createResourcePack();
    auto vanillaResult = vanillaPack->initialize();
    if (vanillaResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(vanillaPack));
    }

    // 娣诲姞椤圭洰鍐呯疆璧勬簮鍖咃紙鎻愪緵 sounds.json銆侀煶鏁堟枃浠剁瓑锛?
    auto builtinResourcesPack = std::make_shared<FolderResourcePack>("resources/data/minecraft");
    auto builtinResult = builtinResourcesPack->initialize();
    if (builtinResult.success()) {
        (void)m_resourceManager->addResourcePack(std::move(builtinResourcesPack));
    }

    // 閲嶆柊娣诲姞鍚敤鐨勮祫婧愬寘
    auto enabledPacks = m_resourcePackList.getEnabledPacks();
    for (const auto& pack : enabledPacks) {
        auto result = m_resourceManager->addResourcePack(pack);
        if (result.failed()) {
            spdlog::warn("Failed to add resource pack: {}", result.error().toString());
        }
    }

    // 閲嶆柊鍔犺浇璧勬簮
    if (m_resourceManager->resourcePackCount() > 0) {
        auto loadResult = m_resourceManager->reload();
        if (loadResult.failed()) {
            spdlog::error("Failed to reload resources: {}", loadResult.error().toString());
            return;
        }

        // 閲嶆柊鏋勫缓绾圭悊鍥鹃泦
        auto atlasResult = m_resourceManager->buildTextureAtlas();
        if (atlasResult.failed()) {
            spdlog::error("Failed to rebuild texture atlas: {}", atlasResult.error().toString());
            return;
        }

        // 閲嶅缓妯″瀷缂撳瓨
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

    // 鑾峰彇 ChatWidget
    auto* chatWidget = m_kageroEngine ?
        static_cast<ui::minecraft::widgets::ChatWidget*>(m_kageroEngine->getLayer(m_chatLayerId)) : nullptr;

    // 娣诲姞鍒拌亰澶╁巻鍙?
    if (chatWidget) {
        chatWidget->addMessage(input, 0xFFFFFFFF);
    }

    // 妫€鏌ユ槸鍚︿负鍛戒护锛堜互 / 寮€澶达級
    if (input[0] == '/') {
        String command = input.substr(1);

        spdlog::info("Chat command received: {}", std::string(command.begin(), command.end()));

        // 鍙戦€佸埌鏈嶅姟绔?
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 鏈湴鍥炴樉
            if (chatWidget) {
                chatWidget->addSystemMessage("Command executed locally (not connected to server)");
            }
        }
    } else {
        // 鏅€氳亰澶╂秷鎭紝鍙戦€佸埌鏈嶅姟绔?
        if (m_networkClient && m_networkClient->isLoggedIn()) {
            m_networkClient->sendChatMessage(input);
        } else {
            // 鏈湴鍥炴樉
            if (chatWidget) {
                chatWidget->addSystemMessage("Message sent locally (not connected to server)");
            }
        }
    }
}

} // namespace mc::client

