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

#include "ClientApplication.hpp"
#include "client/application/features/ClientApplicationHelpers.hpp"
#include "client/command/ClientCommandManager.hpp"
#include "client/renderer/trident/block/BreakProgressManager.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/EntityRendererManager.hpp"
#include "client/renderer/trident/firstperson/FirstPersonRenderer.hpp"
#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "client/renderer/trident/gui/GuiSpriteAtlas.hpp"
#include "client/renderer/trident/gui/GuiSpriteRegistry.hpp"
#include "client/renderer/trident/gui/GuiTextureLoader.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/renderer/trident/item/ItemRenderer.hpp"
#include "client/renderer/util/GpuInfo.hpp"
#include "client/resource/ResourceManager.hpp"
#include "client/resource/TextureAtlasBuilder.hpp"
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/Font.hpp"
#include "client/ui/GuiScale.hpp"
#include "client/ui/minecraft/screens/DebugScreenWidget.hpp"
#include "client/ui/minecraft/targetinfo/TargetInfoWidget.hpp"
#include "client/ui/minecraft/widgets/ChatWidget.hpp"
#include "client/ui/minecraft/widgets/CrosshairWidget.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/screen/ChestScreen.hpp"
#include "client/ui/screen/FurnaceScreen.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/profiler/ProfilerManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/VanillaResources.hpp"
#include "common/resource/pack/FolderResourcePack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/PlatformInfo.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "minecraft-reborn/version.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <thread>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

using namespace mc::trace;

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
    const bool enablePerfettoTracing = !params.benchmarkExitAfterInitialize;
    if (enablePerfettoTracing) {
        // 初始化性能追踪
        mc::profiler::TraceConfig traceConfig;
        traceConfig.outputPath = "client_trace.perfetto-trace";
        traceConfig.bufferSizeKb = 65536 * 8;
        mc::profiler::ProfilerManager::instance().initialize(traceConfig);
        mc::profiler::ProfilerManager::instance().startTracing();

        // 设置进程和主线程名称
        mc::profiler::ProfilerManager::instance().setProcessName("MinecraftClient");
        mc::profiler::ProfilerManager::instance().setThreadName("ClientMainThread");
        spdlog::info("Perfetto tracing initialized");

        // 启动内存追踪线程
        m_memoryTraceThread.start();
    } else {
        spdlog::info("Benchmark initialize-only mode enabled, skipping client perfetto tracing");
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::initialize");

    if (m_initialized) {
        return Error(ErrorCode::AlreadyExists, "Client already initialized");
    }

    // 保存启动参数
    m_launchParams = params;

    // 设置状态机回调
    setupStateMachineCallbacks();

    // 加载设置（从配置文件路径推导游戏目录，未指定时使用默认路径）
    std::filesystem::path settingsFilePath;
    if (params.configPath.has_value()) {
        settingsFilePath = std::filesystem::path(*params.configPath);
    } else {
        settingsFilePath = GameDirectory::defaultDirectory().clientOptionsPath();
    }
    auto settingsResult = loadSettings(settingsFilePath.string());
    if (settingsResult.failed()) {
        spdlog::warn("Failed to load settings from {}: {}. Using defaults.",
            settingsFilePath.string(),
            settingsResult.error().toString());
    }

    // 从配置文件路径推导游戏目录，并确保目录结构存在
    m_gameDirectory = GameDirectory::fromConfigPath(settingsFilePath);
    auto dirResult = m_gameDirectory.ensureDirectoriesExist();
    if (dirResult.failed()) {
        spdlog::warn("Failed to create game directories: {}", dirResult.error().toString());
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
        spdlog::warn("Unknown log level '{}', defaulting to 'info'", logLevel);
        spdlog::set_level(spdlog::level::info);
    }

    spdlog::info("=== Cubium Client ===");
    spdlog::info("Version: {}.{}.{}", MC_VERSION_MAJOR, MC_VERSION_MINOR, MC_VERSION_PATCH);
    spdlog::info("Initializing client...");

    // 初始化外壳（不包含游戏会话）
    auto shellResult = initializeShell(params);
    if (shellResult.failed()) {
        return shellResult.error();
    }

    // 完成 Shell 初始化，进入主菜单
    if (!m_stateMachine.finishInitializing()) {
        return Error(ErrorCode::InvalidState, "Failed to transition to MainMenu state");
    }

    // benchmark 模式：只测 Shell 初始化，initialize 返回后立即退出
    if (params.benchmarkExitAfterInitialize) {
        spdlog::info("Benchmark initialize-only mode finished shell initialization, skipping main menu and gameplay");
        m_initialized = true;
        return Result<void>::ok();
    }

    // Quick-play 模式：直接启动世界
    if (params.quickPlayLevelId.has_value() || params.quickPlayNew) {
        spdlog::info("[QuickPlay] Starting world directly...");

        WorldLaunchConfig config;
        world::storage::GlobalStorageManager storageManager(m_gameDirectory);
        if (params.quickPlayNew) {
            config.levelId = "quick_play_world";
            config.displayName = "Quick Play World";
            config.seed = 42;
            config.isNewWorld = true;
            config.defaultGameMode = GameMode::Creative;
            // 原版语义：创造模式新世界默认允许作弊（对齐 LevelDatCodec 中
            // allowCommands = (gameMode == Creative) 的默认值），否则主机权限为 0、
            // /tp 等权限 2 命令不可用。
            config.allowCommands = true;
        } else {
            config.levelId = *params.quickPlayLevelId;
            auto summaryResult = storageManager.listWorlds();
            if (summaryResult.success()) {
                for (const auto& entry : summaryResult.value()) {
                    if (entry.levelId == config.levelId) {
                        config.displayName = entry.displayName;
                        config.seed = static_cast<i64>(entry.seed);
                        config.worldType = entry.worldType;
                        config.worldPresetId = entry.worldPresetId;
                        config.defaultGameMode = entry.gameMode;
                        config.difficulty = entry.difficulty;
                        config.hardcore = entry.hardcore;
                        config.allowCommands = entry.allowCommands;
                        break;
                    }
                }
            }
            if (config.displayName.empty()) {
                config.displayName = config.levelId;
            }
        }
        config.viewDistance = m_settings.renderDistance.get();

        auto worldResult = startIntegratedWorld(config);
        if (worldResult.failed()) {
            spdlog::error("[QuickPlay] Failed to start world: {}", worldResult.error().toString());
            // 回退到主菜单
            m_stateMachine.forceState(ClientAppState::MainMenu);
        }
    } else {
        // 正常模式：显示主菜单
        showMainMenu();
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
    }
    catch (const std::exception& e) {
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

    // 只在游戏中捕获鼠标（quick-play 模式）
    if (m_stateMachine.isInGame()) {
        toggleMouseCapture();
    }

    m_lastFrameTime = glfwGetTime();

    while (m_running && !m_window.shouldClose()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Frame");

        const auto frameStart = clock::now();

        // 计算帧时间
        const f64 currentTime = glfwGetTime();
        const f32 deltaTime = static_cast<f32>(currentTime - m_lastFrameTime);
        m_lastFrameTime = currentTime;

        // 处理事件
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "HandleEvents");
            handleEvents();
        }

        // 更新
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Update");
            update(deltaTime);
        }

        // 渲染
        {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "Render");
            render();
        }

        // 清理本帧的瞬时输入状态
        m_input.endFrame();

        // 处理异步网格构建结果

#if MC_ENABLE_TRACING
        // 追踪 FPS
        const f32 safeDeltaTime = std::max(deltaTime, 0.0001f);
        const i32 fps = static_cast<i32>(1.0f / safeDeltaTime);
        if (fps < 1000) { // 过滤掉异常值
            MC_TRACE_COUNTER(TraceEvents.Rendering.Frame, "FPS", fps);
        }
#endif

        // 帧率限制（0=不限制）
        const i32 fpsLimit = m_settings.framerateLimit.get();
        if (fpsLimit > 0) {
            const auto minFrameDuration = std::chrono::duration<f64>(1.0 / static_cast<f64>(fpsLimit));
            const auto frameElapsed = clock::now() - frameStart;
            if (frameElapsed < minFrameDuration) {
                MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "FrameRateLimitSleep", "phase", "sleep");
                std::this_thread::sleep_for(minFrameDuration - frameElapsed);
            }
        }
    }

    shutdown();
}

glm::mat4 ClientApplication::buildViewBobbingTransform(f32 partialTick) const
{
    if (!m_player || !m_settings.viewBobbing.get()) {
        return glm::mat4(1.0f);
    }

    const f32 distanceDelta = m_player->moveDistanceWalked() - m_player->prevMoveDistanceWalked();
    const f32 phase = -(m_player->moveDistanceWalked() + distanceDelta * partialTick);
    const f32 cameraYaw = math::lerp(m_player->prevCameraYaw(), m_player->cameraYaw(), partialTick);
    const f32 sinPhase = std::sin(phase * math::PI);
    const f32 cosPhase = std::cos(phase * math::PI);

    glm::mat4 transform(1.0f);
    transform =
        glm::translate(transform, glm::vec3(sinPhase * cameraYaw * 0.5f, -std::abs(cosPhase * cameraYaw), 0.0f));
    transform = glm::rotate(transform, sinPhase * cameraYaw * 3.0f * math::DEG_TO_RAD, glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::rotate(transform,
        std::abs(std::cos(phase * math::PI - 0.2f) * cameraYaw) * 5.0f * math::DEG_TO_RAD,
        glm::vec3(1.0f, 0.0f, 0.0f));
    return transform;
}

void ClientApplication::update(f32 deltaTime)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update");

    // 根据状态决定更新逻辑
    if (!m_stateMachine.hasActiveGameSession()) {
        // 无活跃游戏会话：只更新 UI 和音频（无玩家物理，partialTick 为 0）
        updateUiFrameState(deltaTime, 0.0f);
        updateAudioPauseState();
        return;
    }

    // 有活跃游戏会话：更新网络、玩家、世界等

    // 更新网络客户端（处理服务端数据包）
    if (m_networkClient) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::NetworkPoll");
        m_networkClient->poll();
    }

    // 更新破坏进度管理器
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::BreakProgress");
        using namespace mc::client::renderer::trident::block;
        BreakProgressManager::instance().tick(deltaTime, static_cast<u64>(m_world.gameTime()));
    }

    if (m_renderer && m_renderer->isFirstPersonRendererInitialized()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::FirstPersonTick");
        m_renderer->firstPersonRenderer().tick(m_player.get());
    }

    // 只在非暂停状态下更新游戏逻辑
    if (m_stateMachine.needsGameTick()) {
        // 更新玩家物理
        if (m_player) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::PlayerPhysics");
            m_playerPhysicsAccumulator += std::min(deltaTime, PLAYER_PHYSICS_INTERVAL * 5.0f);
            bool physicsUpdated = false;
            i32 physicsSteps = 0;
            while (m_playerPhysicsAccumulator >= PLAYER_PHYSICS_INTERVAL && physicsSteps < 5) {
                m_playerPhysicsAccumulator -= PLAYER_PHYSICS_INTERVAL;
                m_player->updatePhysics();
                physicsUpdated = true;
                ++physicsSteps;
            }
            if (physicsSteps == 5) {
                m_playerPhysicsAccumulator = 0.0f;
            }

            if (physicsUpdated) {
                updatePlayerAudio();
            }
        }
    }

    // 计算 partialTick（用于渲染插值）
    // 在物理更新后计算，表示当前帧在两个 tick 之间的位置 [0.0, 1.0)
    const f32 partialTick = std::clamp(m_playerPhysicsAccumulator / PLAYER_PHYSICS_INTERVAL, 0.0f, 1.0f);

    // 更新相机（暂停时也更新，以便 UI 渲染）
    if (m_player) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::Camera");
        // 检查是否有旁观目标实体
        auto cameraTargetId = m_player->getCameraEntityId();
        if (cameraTargetId.has_value()) {
            // 旁观者模式：跟随目标实体的位置和旋转
            ClientEntity* targetEntity = m_world.entityManager().getEntity(cameraTargetId.value());
            if (targetEntity != nullptr) {
                const Vector3 renderPosition = targetEntity->prevPosition().lerp(targetEntity->position(), partialTick);
                // 使用 ClientEntity 的眼高接口，根据实体类型和姿态返回正确的眼高值
                // 非玩家实体（如末影龙、羊驼等）使用各自的注册表眼高，玩家实体根据姿态调整
                const f32 eyeHeight = targetEntity->eyeHeight();
                m_camera.setPosition(renderPosition.x, renderPosition.y + eyeHeight, renderPosition.z);
                m_camera.setYaw(targetEntity->yaw());
                m_camera.setPitch(targetEntity->pitch());
            } else {
                // 目标实体不存在，回退到自身视角
                const Vector3 renderPosition = m_player->prevPosition().lerp(m_player->position(), partialTick);
                m_camera.setPosition(
                    renderPosition.x, renderPosition.y + static_cast<f32>(m_player->eyeHeight()), renderPosition.z);
                m_camera.setYaw(m_player->yaw());
                m_camera.setPitch(m_player->pitch());
            }
            // 旁观者模式下不使用视野晃动
            m_camera.clearViewTransform();
        } else {
            // 正常视角：跟随玩家位置和旋转
            const Vector3 renderPosition = m_player->prevPosition().lerp(m_player->position(), partialTick);

            // 同步相机位置到玩家眼睛位置
            m_camera.setPosition(
                renderPosition.x, renderPosition.y + static_cast<f32>(m_player->eyeHeight()), renderPosition.z);
            m_camera.setYaw(m_player->yaw());
            m_camera.setPitch(m_player->pitch());
            m_camera.setViewTransform(buildViewBobbingTransform(partialTick));
        }
        m_camera.update(deltaTime);

    } else {
        // 后备：更新相机控制器（这会调用 Camera::update 更新矩阵）
        m_camera.clearViewTransform();
        m_cameraController.update(deltaTime);
    }

    // 只在非暂停状态下发送位置和更新游戏
    if (m_stateMachine.needsGameTick()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::GameTick");
        // 发送玩家位置到服务端（物理 tick 后最多 20 TPS）
        if (m_networkClient && m_networkClient->isLoggedIn() && m_player) {
            m_positionSendAccumulator += deltaTime;
            if (m_positionSendAccumulator >= POSITION_SEND_INTERVAL) {
                m_positionSendAccumulator = std::fmod(m_positionSendAccumulator, POSITION_SEND_INTERVAL);
                sendPlayerPosition();
            }
        }

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

        // 方块动画 tick：在玩家周围随机采样位置，调用方块的 animateTick 生成粒子和音效
        const auto& playerPos = m_player->position();
        m_world.animateTick(
            static_cast<i32>(playerPos.x), static_cast<i32>(playerPos.y), static_cast<i32>(playerPos.z));

        // 更新客户端实体（固定频率 tick，20 TPS）
        m_world.entityManager().fixedTick(deltaTime);

        // 更新动画纹理帧计数器（与实体 tick 同步）
        m_renderer->tickTextureAnimations();

        // 更新实体平滑插值（每帧调用）
        m_world.entityManager().updateInterpolation(deltaTime);

        // 更新实体动画状态（用于渲染插值）
        // 使用实体 tick 累积器计算 partialTick
        const f32 entityPartialTick = m_world.entityManager().tickAccumulator() / ClientEntityManager::tickInterval();
        m_world.entityManager().updateAnimations(entityPartialTick);

        // 更新世界音频状态（入水/出水、环境音等）
        updateWorldAudio();

        m_world.processMeshBuildResults(16);

        // 同步时间到渲染器（驱动天空、太阳、月亮、星空变化）
        // 客户端每帧平滑推进时间，同时在收到服务端同步时纠正
        updateTimeAndWeather(deltaTime);
    }

    // 更新每帧 UI 状态（ScreenStackWidget、KageroEngine 等）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::UiFrame");
        updateUiFrameState(deltaTime, partialTick);

        // 更新射线检测结果（暂停时也更新，以便 UI 显示）
        updateRaycastResult();

        updateTargetInfoUi();

        // 更新声音系统暂停状态
        updateAudioPauseState();
    }

    // 上传网格到 GPU（只处理已完成异步构建的网格）
    if (m_stateMachine.needsGameTick() && m_renderer->isChunkRendererInitialized()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Frame, "ClientApplication::update::UploadMeshes");

        auto& chunkRenderer = m_renderer->chunkRenderer();
        m_world.forEachDirtyMesh([&chunkRenderer, this](const ChunkId& id, ClientChunk& chunk) {
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

                // 上传成功后把 CPU 侧网格归还给回收池，复用其 capacity，
                // 避免下次构建从 0 重新 reserve。仅 clear()(保留 capacity)即可入池；
                // 回收池内 _shrinkIfBloated 会对异常膨胀的 capacity 做 shrink。
                chunk.solidMesh.clear();
                chunk.transparentMesh.clear();
                m_world.meshDataPool()->recycle(false, std::move(chunk.solidMesh));
                m_world.meshDataPool()->recycle(true, std::move(chunk.transparentMesh));
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

void ClientApplication::releaseRendererDependentResources()
{
    // 这些对象内部持有 Vulkan 句柄，必须在渲染器销毁前释放。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::releaseRendererDependentResources");

    if (m_skinManager) {
        m_skinManager->shutdown();
        m_skinManager.reset();
    }

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
}

void ClientApplication::shutdown()
{
    // 整体 shutdown 区间。注意：本函数末尾会调用 ProfilerManager::stopTracing()/
    // shutdown()，此 RAII 事件析构触发的 END 在会话已停止后是安全 no-op
    // （SDK 的 CallIfEnabled 检测到 instances==0 即立即返回），BEGIN 与中间子事件
    // 均正常记录，符合期望。
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown");

    spdlog::info("Shutting down client...");

    // 开始关闭流程
    if (!m_stateMachine.startShutdown()) {
        // 强制进入关闭状态
        m_stateMachine.forceState(ClientAppState::ShuttingDown);
    }

    // 保存设置
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::SaveSettings");
        const auto savePath =
            m_settingsPath.empty() ? GameDirectory::defaultDirectory().clientOptionsPath() : m_settingsPath;
        auto saveResult = m_settings.saveSettings(savePath);
        if (saveResult.failed()) {
            spdlog::warn("Failed to save settings: {}", saveResult.error().toString());
        }
    }

    // 关闭声音系统
    shutdownAudio();

    // 销毁游戏会话（如果有）
    if (m_stateMachine.hasActiveGameSession()) {
        destroyGameSession();
    }

    // 断开网络连接（如果在主菜单但有残留连接）
    if (m_networkClient) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::DisconnectNetwork");
        m_networkClient->disconnect("Client shutdown");
        m_networkClient.reset();
    }

    // 停止内置服务端（如果有残留）
    if (m_integratedServer) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::StopIntegratedServer");
        m_integratedServer->stop();
        m_integratedServer.reset();
    }

    // 先清理所有依赖渲染器设备的对象，避免析构时访问失效 VkDevice。
    releaseRendererDependentResources();

    // 清理渲染器
    if (m_renderer) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::DestroyRenderer");
        m_renderer->destroy();
        m_renderer.reset();
    }

    // 清理玩家
    {
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Client.Initialization, "ClientApplication::shutdown::DestroyPlayerAndPhysics");
        m_player.reset();
        m_physicsEngine.reset();
    }

    // 清理世界（包括关闭网格构建线程池）
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::DestroyWorld");
        m_world.destroy();
    }

    // 关闭客户端统一计算池（ClientCompute）。此时 mesh 系统已随 m_world.destroy() 关停
    // （shutdownMeshSystem 仅关 scheduler 并等在途归零，不关池），皮肤管理器已随
    // releaseRendererDependentResources 销毁，池已无消费者。晚到的回调持 weak_ptr
    // 到已 reset 的结果队列，lock 失败直接丢弃，安全。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::ShutdownComputePool");
        m_clientComputeWorkerPool.shutdown();
    }

    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "ClientApplication::shutdown::DestroyWindow");
        m_window.destroy();
    }

    if (!m_launchParams.benchmarkExitAfterInitialize) {
        // 停止内存追踪线程
        m_memoryTraceThread.stop();

        // 关闭性能追踪
        mc::profiler::ProfilerManager::instance().stopTracing();
        mc::profiler::ProfilerManager::instance().shutdown();
        spdlog::info("Perfetto tracing stopped");
    }

    spdlog::info("Client stopped.");
}

} // namespace mc::client
