#include "ClientApplication.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/biome/BiomeEffects.hpp"
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
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
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
#include "client/application/features/ClientApplicationHelpers.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/GuiScale.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "client/ui/screen/CraftingScreen.hpp"
#include "client/ui/screen/ChestScreen.hpp"
#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/ui/screen/CreativeScreen.hpp"
#include "client/ui/minecraft/widgets/CrosshairWidget.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"
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

ClientApplication::ClientApplication() = default;

ClientApplication::~ClientApplication()
{
    if (m_running) {
        stop();
    }
}

Result<void> ClientApplication::initialize(const ClientLaunchParams& params)
{
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

    MC_TRACE_EVENT("client.initialization", "ClientApplication::initialize");

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

    // 新的分层初始化流程
    initializeCoreRegistries();

    {
        auto audioResult = initializeAudio();
        if (audioResult.failed()) {
            spdlog::error("Failed to initialize audio system: {}", audioResult.error().toString());
        }
    }

    {
        MC_TRACE_EVENT("client.initialization", "InitializeResources");
        spdlog::info("Initializing resource system...");
        auto resourceResult = initializeResources();
        if (resourceResult.failed()) {
            spdlog::error("Failed to initialize resource system: {}. Using default rendering.",
                        resourceResult.error().toString());
        }
    }

    auto windowResult = initializeWindowAndInput();
    if (windowResult.failed()) {
        return windowResult.error();
    }

    auto rendererResult = initializeRenderer();
    if (rendererResult.failed()) {
        return rendererResult.error();
    }

    auto gameplayResult = initializeGameplaySystems(params);
    if (gameplayResult.failed()) {
        return gameplayResult.error();
    }

    initializeUi();

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
        MC_TRACE_EVENT("rendering.frame", "Frame", "phase", "frame");

        const auto frameStart = clock::now();

        // 计算帧时间
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 处理事件
        {
            MC_TRACE_EVENT("rendering.frame", "HandleEvents", "phase", "handle_events");
            handleEvents();
        }

        // 更新
        {
            MC_TRACE_EVENT("rendering.frame", "Update", "phase", "update");
            update(deltaTime);
        }

        // 渲染
        {
            MC_TRACE_EVENT("rendering.frame", "Render", "phase", "render");
            render();
        }

        // 重建皮肤纹理图集（如果有新皮肤）
        if (m_skinManager && m_skinManager->needsAtlasRebuild()) {
            auto rebuildResult = m_skinManager->rebuildAtlas();
            if (rebuildResult.failed()) {
                spdlog::warn("Failed to rebuild skin atlas: {}", rebuildResult.error().toString());
            } else {
                spdlog::debug("Skin atlas rebuilt successfully");
            }
        }

        // 清理本帧的瞬时输入状态
        m_input.endFrame();

    // 处理异步网格构建结果

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
                MC_TRACE_EVENT("rendering.frame", "FrameRateLimitSleep", "phase", "sleep");
                std::this_thread::sleep_for(minFrameDuration - frameElapsed);
            }
        }
    }

    shutdown();
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
        BreakProgressManager::instance().tick(deltaTime, static_cast<u64>(m_world.gameTime()));
    }

    if (m_renderer && m_renderer->isFirstPersonRendererInitialized()) {
        m_renderer->firstPersonRenderer().tick();
    }

    // 更新玩家物理
    if (m_player) {
        // 应用物理（重力、碰撞检测）
        m_player->updatePhysics();

        // 处理脚步声和游泳声
        // updateMoveDistance 在 Player::updatePhysics 中调用
        // 这里检查是否需要播放音频
        updatePlayerAudio();

        // 计算视野晃动
        // 参考 MC 1.16.5 GameRenderer.getCameraPosition()
        f32 bobX = 0.0f;
        f32 bobY = 0.0f;

        if (m_settings.viewBobbing.get()) {
            // 获取移动距离变化
            f32 distanceWalked = m_player->moveDistanceWalked();
            f32 prevDistanceWalked = m_player->prevMoveDistanceWalked();
            f32 distanceSwam = m_player->moveDistanceSwam();
            f32 prevDistanceSwam = m_player->prevMoveDistanceSwam();

            // 计算距离差
            f32 walkedDelta = distanceWalked - prevDistanceWalked;
            f32 swamDelta = distanceSwam - prevDistanceSwam;

            // 累计晃动角度
            m_bobAngle += walkedDelta * 0.5f;
            m_bobPhase += swamDelta * 0.5f;

            // 计算 X 晃动（左右摆动）
            bobX = std::sin(m_bobAngle) * 0.1f;

            // 计算 Y 晃动（上下摆动）
            // MC 使用 cos 的绝对值来产生上下晃动
            bobY = std::abs(std::cos(m_bobAngle)) * 0.1f;

            // 游泳时的额外晃动
            if (m_player->isSwimming()) {
                // 游泳时有额外的左右晃动
                bobX += std::sin(m_bobPhase * 2.0f) * 0.05f;
                bobY += std::abs(std::cos(m_bobPhase)) * 0.05f;
            }
        }

        // 同步相机位置到玩家眼睛位置
        m_camera.setPosition(
            static_cast<f32>(m_player->x()) + bobX,
            static_cast<f32>(m_player->y() + m_player->eyeHeight()) - bobY,
            static_cast<f32>(m_player->z())
        );
        m_camera.setYaw(m_player->yaw());
        m_camera.setPitch(m_player->pitch());
        m_camera.update(deltaTime);

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

    // 更新每帧 UI 状态（ScreenStackWidget、KageroEngine 等）
    updateUiFrameState(deltaTime);

    // 更新射线检测结果
    updateRaycastResult();

    updateTargetInfoUi();

    handleBlockMiningInput(deltaTime);

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

    // 音频暂停状态在 updateAudioPauseState() 中统一处理，避免重复投递命令。

    // 更新实体平滑插值（每帧调用）
    m_world.entityManager().updateInterpolation(deltaTime);

    // 更新实体动画状态（用于渲染插值）
    constexpr f32 partialTick = 0.0f;  // TODO: 从主循环获取实际的部分tick
    m_world.entityManager().updateAnimations(partialTick);

    // 更新世界音频状态（入水/出水、环境音等）
    updateWorldAudio();

    // 更新声音系统暂停状态
    updateAudioPauseState();

    m_world.processMeshBuildResults(16);

    // 同步时间到渲染器（驱动天空、太阳、月亮、星空变化）
    // 客户端每帧平滑推进时间，同时在收到服务端同步时纠正
    updateTimeAndWeather(deltaTime);

    // 上传网格到 GPU（只处理已完成异步构建的网格）
    if (m_renderer->isChunkRendererInitialized()) {
        MC_TRACE_EVENT("rendering.frame", "UploadMeshes");

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
    shutdownAudio();

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

} // namespace mc::client

