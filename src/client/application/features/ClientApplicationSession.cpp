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
#include "client/sound/AudioService.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/ui/minecraft/screens/CreateWorldScreen.hpp"
#include "client/ui/minecraft/screens/LoadingScreen.hpp"
#include "client/ui/minecraft/screens/MainMenuScreen.hpp"
#include "client/ui/minecraft/screens/MessageScreen.hpp"
#include "client/ui/minecraft/screens/OptionsScreen.hpp"
#include "client/ui/minecraft/screens/PauseScreen.hpp"
#include "client/ui/minecraft/screens/WorldSelectionScreen.hpp"
#include "client/ui/minecraft/widgets/HudWidget.hpp"
#include "client/ui/minecraft/widgets/ScreenStackWidget.hpp"
#include "client/ui/screen/ScreenManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/storage/GlobalStorageManager.hpp"
#include "common/world/storage/list/WorldNameSanitizer.hpp"

#include <GLFW/glfw3.h>

using namespace mc::trace;

namespace mc::client {

// ========== 辅助函数实现 ==========

ui::minecraft::widgets::ScreenStackWidget* getScreenStackWidget(ClientApplication* app)
{
    if (!app || !app->m_kageroEngine) {
        return nullptr;
    }
    return static_cast<ui::minecraft::widgets::ScreenStackWidget*>(
        app->m_kageroEngine->getLayer(app->m_screenStackLayerId));
}

// ========== 状态机回调设置 ==========

void ClientApplication::setupStateMachineCallbacks()
{
    m_stateMachine.setOnStateChanged([this](ClientAppState from, ClientAppState to) { onStateChanged(from, to); });

    m_stateMachine.setLoadingProgressCallback([this](const std::string& stage, f32 progress) {
        spdlog::info("[Loading] {} - {:.0f}%", stage, progress * 100.0f);
        updateLoadingProgress(stage, progress);
    });
}

void ClientApplication::onStateChanged(ClientAppState from, ClientAppState to)
{
    spdlog::info("[StateMachine] State changed: {} -> {}",
        ClientAppStateMachine::stateToString(from),
        ClientAppStateMachine::stateToString(to));

    // 状态进入逻辑
    switch (to) {
        case ClientAppState::MainMenu:
            // 进入主菜单时释放鼠标
            if (m_mouseCaptured) {
                toggleMouseCapture();
            }
            // 隐藏加载屏幕（如果显示中）
            hideLoadingScreen();
            // Quick-play 模式下不显示主菜单
            if (m_launchParams.quickPlayLevelId.has_value() || m_launchParams.quickPlayNew) {
                spdlog::info("[Session] Quick-play mode, skipping main menu display");
                return;
            }
            // 显示主菜单
            showMainMenu();
            break;

        case ClientAppState::LoadingWorld:
            // 显示加载界面
            showLoadingScreen();
            break;

        case ClientAppState::InGame:
            // 进入游戏时捕获鼠标
            if (!m_mouseCaptured) {
                toggleMouseCapture();
            }
            // 隐藏加载屏幕
            hideLoadingScreen();
            break;

        case ClientAppState::Paused:
            // 暂停时显示暂停菜单（由 showPauseMenu 处理）
            break;

        case ClientAppState::LeavingWorld:
            // 离开世界时显示保存界面
            // TODO: 显示保存进度界面
            break;

        case ClientAppState::ShuttingDown:
            // 关闭时清理
            break;

        default:
            break;
    }
}

// ========== 游戏会话管理 ==========

Result<void> ClientApplication::startIntegratedWorld(const WorldLaunchConfig& config)
{
    if (!m_stateMachine.canStartWorld()) {
        return Error(ErrorCode::InvalidState,
            "Cannot start world in current state: " +
                std::string(ClientAppStateMachine::stateToString(m_stateMachine.state())));
    }

    spdlog::info("[Session] Starting integrated world: {} (seed={})", config.displayName, config.seed);

    // 转换到加载状态
    if (!m_stateMachine.startLoadingWorld(config)) {
        return Error(ErrorCode::InvalidState, "Failed to transition to LoadingWorld state");
    }

    // 执行会话初始化
    auto result = initializeGameSession(config);
    if (result.failed()) {
        spdlog::error("[Session] Failed to initialize game session: {}", result.error().toString());
        // 回退到主菜单
        m_stateMachine.forceState(ClientAppState::MainMenu);
        return result.error();
    }

    // 完成加载，进入游戏
    if (!m_stateMachine.finishLoadingWorld()) {
        return Error(ErrorCode::InvalidState, "Failed to transition to InGame state");
    }

    return Result<void>::ok();
}

Result<void> ClientApplication::initializeGameSession(const WorldLaunchConfig& config)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "InitializeGameSession");

    m_stateMachine.reportLoadingProgress("Starting server", 0.1f);

    // 启动内置服务端
    m_integratedServer = std::make_unique<server::IntegratedServer>();
    server::IntegratedServerParams serverParams;
    serverParams.gameDirectoryRoot = m_gameDirectory.root().string();
    serverParams.seed = config.seed;
    serverParams.viewDistance = config.viewDistance;
    serverParams.defaultGameMode = config.defaultGameMode;
    serverParams.worldName = config.levelId;
    serverParams.allowCommands = config.allowCommands;

    auto serverResult = m_integratedServer->initialize(serverParams);
    if (serverResult.failed()) {
        spdlog::error("[Session] Failed to initialize integrated server: {}", serverResult.error().toString());
        m_integratedServer.reset();
        return serverResult.error();
    }

    m_stateMachine.reportLoadingProgress("Connecting to server", 0.3f);

    // 初始化网络客户端
    m_networkClient = std::make_unique<NetworkClient>();
    m_commandManager = std::make_unique<command::ClientCommandManager>();
    m_commandManager->setPlayerNameProvider([this]() { return collectPlayerCompletionCandidates(); });
    m_commandManager->setEntityNameProvider([this]() { return collectEntityCompletionCandidates(); });
    setupNetworkCallbacks();

    // 初始化皮肤管理器
    m_skinManager = std::make_unique<skin::ClientSkinManager>();
    // 必须在 initialize() 之前注入资源包，否则 DefaultSkinProvider 无法从
    // 资源包读取 18 种默认皮肤 PNG 纹理，回退到零像素占位数据
    if (m_resourceManager && m_resourceManager->resourcePackCount() > 0) {
        m_skinManager->setResourcePack(m_resourceManager->getFirstResourcePack());
    } else {
        spdlog::warn("[Session] No resource pack available, default skin textures will fall back to zero-pixel data");
    }
    std::string skinCacheDir = (m_gameDirectory.cacheDir() / "skins").string();
    auto skinResult = m_skinManager->initialize(m_renderer->device(),
        m_renderer->physicalDevice(),
        m_renderer->commandPool(),
        m_renderer->graphicsQueue(),
        skinCacheDir);
    if (skinResult.failed()) {
        spdlog::warn("[Session] Failed to initialize skin manager: {}", skinResult.error().toString());
        // 皮肤管理器初始化失败不是致命错误
    } else {
        spdlog::info("[Session] Skin manager initialized");
    }

    m_stateMachine.reportLoadingProgress("Logging in", 0.4f);

    // 连接到内置服务端
    NetworkClientConfig clientConfig;
    clientConfig.username = m_settings.username.get();
    auto clientResult = m_networkClient->connectLocal(m_integratedServer->getClientEndpoint(), clientConfig);
    if (clientResult.failed()) {
        spdlog::error("[Session] Failed to connect to integrated server: {}", clientResult.error().toString());
        m_integratedServer->stop();
        m_integratedServer.reset();
        m_networkClient.reset();
        m_commandManager.reset();
        return clientResult.error();
    }

    m_useIntegratedServer = true;
    spdlog::info("[Session] Connected to integrated server");

    m_stateMachine.reportLoadingProgress("Initializing world", 0.5f);

    // 初始化世界
    auto worldResult = m_world.initialize(config.seed);
    if (worldResult.failed()) {
        spdlog::error("[Session] Failed to initialize world: {}", worldResult.error().toString());
        m_integratedServer->stop();
        m_integratedServer.reset();
        m_networkClient.reset();
        m_commandManager.reset();
        return worldResult.error();
    }

    m_stateMachine.reportLoadingProgress("Building meshes", 0.6f);

    // 初始化网格构建系统
    MeshSchedulerConfig schedulerConfig;
    schedulerConfig.maxDispatchedTaskCount = std::max(32, m_settings.renderDistance.get() * 12);
    schedulerConfig.reprioritizeIntervalFrames = 6;
    schedulerConfig.cameraMoveThreshold = 2.0f;
    schedulerConfig.cameraDirectionDotThreshold = 0.96f;
    schedulerConfig.behindCancelDotThreshold = -0.35f;
    schedulerConfig.behindCancelDistanceChunks = static_cast<f32>(std::max(6, m_settings.renderDistance.get() / 2));
    m_world.initializeMeshSystem(-1, schedulerConfig);

    // 设置区块卸载回调
    m_world.setChunkUnloadCallback([this](const ChunkId& chunkId) {
        if (m_renderer && m_renderer->isChunkRendererInitialized()) {
            m_renderer->chunkRenderer().removeChunk(chunkId);
        }
    });

    // 设置本地音效播放回调（方块动画 tick 产生的环境音效）
    m_world.setPlayLocalSoundCallback([this](const ResourceLocation& soundEventId,
                                          sound::SoundCategory category,
                                          const Vector3& position,
                                          f32 volume,
                                          f32 pitch) {
        if (m_audioService) {
            auto sound = std::make_unique<sound::SoundInstance>(sound::SoundInstance::createLocated(
                soundEventId, category, position.x, position.y, position.z, volume, pitch));
            m_audioService->play(std::move(sound));
        }
    });

    // 设置实体渲染回调
    m_renderer->setEntityRenderCallback([this](VkCommandBuffer cmd, f64 partialTick) {
        const auto& frustum = m_renderer->frustum();
        const auto& frameContext = m_renderer->frameContext();

        // 设置相机信息给 EntityRendererManager（用于名称标签渲染）
        if (frameContext.camera) {
            m_renderer->entityRendererManager().setCameraInfo(
                frameContext.camera->position(), frameContext.viewMatrix, frustum);
        }

        // 设置本地玩家访问器（每帧更新以应对登录/登出切换）
        // 第三人称玩家 GPU 管线路径需要从本地 Player 读取 use-item 状态驱动弩动画
        m_renderer->entityRendererManager().setLocalPlayerAccessor(
            m_localIdentity.entityId(), [this]() -> ::mc::Player* { return m_player.get(); });

        m_world.entityManager().forEachEntity([&](client::ClientEntity& entity) {
            m_renderer->entityRendererManager().renderWithPipeline(cmd, entity, partialTick, frustum);
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

        // 从玩家获取挥动动画状态
        const f32 partialTickF32 = static_cast<f32>(partialTick);
        const bool isSwinging = m_player->isSwingInProgress();
        const Hand swingingHand = m_player->swingingHand();

        if (isSwinging) {
            const f32 prevSwing = m_player->prevSwingProgress();
            const f32 currSwing = m_player->swingProgress();
            const f32 interpolatedSwing = prevSwing + (currSwing - prevSwing) * partialTickF32;

            if (swingingHand == Hand::MainHand) {
                renderContext.mainHandSwingProgress = interpolatedSwing;
                renderContext.offHandSwingProgress = 0.0f;
            } else {
                renderContext.mainHandSwingProgress = 0.0f;
                renderContext.offHandSwingProgress = interpolatedSwing;
            }
        }

        // 从玩家获取物品使用状态
        const bool isUsingItem = m_player->isUsingItem();
        if (isUsingItem) {
            const Hand activeHand = m_player->getActiveHand();
            const i32 useCount = m_player->getItemInUseCount();

            if (activeHand == Hand::MainHand) {
                renderContext.isMainHandActive = true;
                renderContext.mainHandUseCount = useCount;
            } else {
                renderContext.isOffHandActive = true;
                renderContext.offHandUseCount = useCount;
            }
        }

        m_renderer->firstPersonRenderer().render(cmd, cameraSet, renderContext);
    });

    m_stateMachine.reportLoadingProgress("Creating player", 0.7f);

    // 创建物理引擎
    m_physicsEngine = std::make_unique<PhysicsEngine>(m_world);

    // 创建玩家
    m_player = std::make_unique<Player>(static_cast<EntityId>(1), m_settings.username.get());
    m_player->setPosition(8.0, 50.0, 8.0);
    m_player->setPhysicsEngine(m_physicsEngine.get());
    m_player->setGameMode(config.defaultGameMode);
    if (config.defaultGameMode == GameMode::Creative) {
        m_player->setCreativeModeInventory();
        m_player->abilities().flying = true;
    }
    spdlog::info("[Session] Player created at (8, 50, 8)");

    m_stateMachine.reportLoadingProgress("Preparing renderer", 0.8f);

    // 更新渲染器设置
    m_renderer->setRenderDistanceChunks(m_settings.renderDistance.get());

    m_stateMachine.reportLoadingProgress("Ready", 1.0f);

    return Result<void>::ok();
}

void ClientApplication::destroyGameSession()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Initialization, "DestroyGameSession");

    spdlog::info("[Session] Destroying game session");

    // 1. 清理 HUD 引用
    if (m_kageroEngine) {
        auto* hudWidget = static_cast<ui::minecraft::widgets::HudWidget*>(m_kageroEngine->getLayer(m_hudLayerId));
        if (hudWidget) {
            hudWidget->setPlayer(nullptr);
        }
    }

    // 2. 清理皮肤管理器
    if (m_skinManager) {
        m_skinManager->shutdown();
        m_skinManager.reset();
    }

    // 3. 清理玩家
    m_player.reset();

    // 4. 清理物理引擎
    m_physicsEngine.reset();

    // 5. 清理世界
    m_world.destroy();

    // 6. 清理渲染器的区块缓冲
    if (m_renderer && m_renderer->isChunkRendererInitialized()) {
        m_renderer->chunkRenderer().clearChunks();
    }

    // 7. 清理网络客户端
    if (m_networkClient) {
        m_networkClient->disconnect("Session destroyed");
        m_networkClient.reset();
    }

    // 8. 清理命令管理器
    m_commandManager.reset();

    // 9. 停止内置服务端
    if (m_integratedServer) {
        spdlog::info("[Session] Stopping integrated server");
        m_integratedServer->stop();
        m_integratedServer.reset();
    }

    // 10. 清理已知玩家名
    m_knownPlayerNames.clear();

    // 11. 重置时间状态
    m_renderGameTime = 0;
    m_renderDayTime = 0;
    m_renderTickAccumulator = 0.0f;
    m_hasServerTimeSync = false;

    // 12. 重置玩家状态
    m_wasPlayerInWater = false;
    m_wasPlayerInLava = false;

    spdlog::info("[Session] Game session destroyed");
}

bool ClientApplication::leaveWorldToMainMenu()
{
    if (!m_stateMachine.canReturnToMainMenu()) {
        spdlog::warn("[Session] Cannot leave world in current state: {}",
            ClientAppStateMachine::stateToString(m_stateMachine.state()));
        return false;
    }

    spdlog::info("[Session] Leaving world to main menu");

    // 开始离开流程
    if (!m_stateMachine.startLeavingWorld()) {
        return false;
    }

    // 世界与玩家存档由 IntegratedServer::stop() 统一负责：
    // - stop() 在 clearAll() 之前调用 savePlayerRuntimeState() 回写在线玩家运行时状态
    // - 随后 stopCore() → shutdownManagers() → saveAllWorldData() 落盘区块、level.dat、玩家数据
    // 因此此处无需再显式触发保存。

    // 销毁游戏会话
    destroyGameSession();

    // 完成离开，返回主菜单
    if (!m_stateMachine.finishLeavingWorld()) {
        spdlog::error("[Session] Failed to finish leaving world");
        return false;
    }

    // 显示主菜单
    showMainMenu();

    return true;
}

void ClientApplication::showMainMenu()
{
    if (!m_stateMachine.isInMainMenu()) {
        spdlog::warn("[Session] Cannot show main menu in current state: {}",
            ClientAppStateMachine::stateToString(m_stateMachine.state()));
        return;
    }

    spdlog::info("[Session] Showing main menu");

    // 获取屏幕栈
    auto* screenStack = getScreenStackWidget(this);
    if (!screenStack) {
        spdlog::error("[Session] ScreenStackWidget not available");
        return;
    }

    // 清除所有屏幕
    screenStack->clear();

    // 创建主菜单屏幕
    auto mainMenuScreen = std::make_unique<ui::minecraft::MainMenuScreen>();

    // 设置回调
    mainMenuScreen->setOnSinglePlayer([this]() { showWorldSelection(); });

    mainMenuScreen->setOnMultiPlayer([this]() {
        // TODO: 多人游戏支持
        spdlog::info("[Session] Multiplayer not yet implemented");
    });

    mainMenuScreen->setOnOptions([this]() {
        auto* screenStack = getScreenStackWidget(this);
        if (!screenStack) {
            return;
        }
        auto optionsScreen = std::make_unique<ui::minecraft::OptionsScreen>();
        optionsScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));
        optionsScreen->setOnClose([this, screenStack]() { screenStack->pop(); });
        screenStack->push(std::move(optionsScreen));
    });

    mainMenuScreen->setOnQuit([this]() {
        spdlog::info("[Session] Quit requested from main menu");
        stop();
    });

    // 设置屏幕大小
    mainMenuScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

    // 添加到屏幕栈（push 会自动调用 onOpen）
    screenStack->push(std::move(mainMenuScreen));

    // 确保鼠标未被捕获
    if (m_mouseCaptured) {
        toggleMouseCapture();
    }
}

void ClientApplication::showPauseMenu()
{
    if (!m_stateMachine.canPause()) {
        spdlog::warn("[Session] Cannot show pause menu in current state: {}",
            ClientAppStateMachine::stateToString(m_stateMachine.state()));
        return;
    }

    // 暂停游戏
    if (!m_stateMachine.pause()) {
        return;
    }

    spdlog::info("[Session] Showing pause menu");

    // 获取屏幕栈
    auto* screenStack = getScreenStackWidget(this);
    if (!screenStack) {
        spdlog::error("[Session] ScreenStackWidget not available");
        return;
    }

    // 创建暂停菜单屏幕
    auto pauseScreen = std::make_unique<ui::minecraft::PauseScreen>();

    // 设置回调
    pauseScreen->setOnResume([this, screenStack]() {
        // 恢复游戏
        if (m_stateMachine.canResumeFromPause()) {
            m_stateMachine.resume();
            spdlog::info("[Session] Resumed from pause");
        }
        // 关闭暂停菜单
        screenStack->pop();
        // 捕获鼠标
        if (!m_mouseCaptured) {
            toggleMouseCapture();
        }
    });

    pauseScreen->setOnOptions([this]() {
        auto* screenStack = getScreenStackWidget(this);
        if (!screenStack) {
            return;
        }
        auto optionsScreen = std::make_unique<ui::minecraft::OptionsScreen>();
        optionsScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));
        optionsScreen->setOnClose([this, screenStack]() { screenStack->pop(); });
        screenStack->push(std::move(optionsScreen));
    });

    pauseScreen->setOnSaveAndQuit([this]() {
        spdlog::info("[Session] Save and quit to title");
        // 离开世界返回主菜单
        leaveWorldToMainMenu();
    });

    // 设置屏幕大小
    pauseScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

    // 添加到屏幕栈（push 会自动调用 onOpen）
    screenStack->push(std::move(pauseScreen));

    // 释放鼠标
    if (m_mouseCaptured) {
        toggleMouseCapture();
    }
}

void ClientApplication::showWorldSelection()
{
    spdlog::info("[Session] Showing world selection");

    // 获取屏幕栈
    auto* screenStack = getScreenStackWidget(this);
    if (!screenStack) {
        spdlog::error("[Session] ScreenStackWidget not available");
        return;
    }

    // 创建存档选择屏幕
    auto worldSelectionScreen = std::make_unique<ui::minecraft::WorldSelectionScreen>();

    // 设置屏幕栈引用（用于弹出确认对话框）
    worldSelectionScreen->setScreenStack(screenStack);

    // 设置回调
    worldSelectionScreen->setOnSelectWorld([this, screenStack](const world::storage::WorldListEntry& entry) {
        spdlog::info("[Session] Selected world: {}", entry.displayName);

        // 构建启动配置
        WorldLaunchConfig config;
        config.levelId = entry.levelId;
        config.displayName = entry.displayName;
        config.seed = static_cast<i64>(entry.seed);
        config.worldType = entry.worldType;
        config.defaultGameMode = entry.gameMode;
        config.difficulty = entry.difficulty;
        config.hardcore = entry.hardcore;
        config.allowCommands = entry.allowCommands;
        config.viewDistance = m_settings.renderDistance.get();
        config.isNewWorld = false;

        // 启动世界
        auto result = startIntegratedWorld(config);
        if (result.failed()) {
            spdlog::error("[Session] Failed to start world: {}", result.error().toString());
            // 弹出错误对话框，告知用户启动失败的具体原因
            auto messageScreen = std::make_unique<ui::minecraft::MessageScreen>(
                "Failed to Start World", result.error().message(), "OK", [screenStack]() {
                    if (screenStack != nullptr) {
                        screenStack->pop();
                    }
                });
            messageScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));
            screenStack->push(std::move(messageScreen));
        }
    });

    worldSelectionScreen->setOnCreateWorld([this]() { showCreateWorld(); });

    worldSelectionScreen->setOnBack([this, screenStack]() {
        // 返回主菜单
        screenStack->pop();
    });

    // 设置屏幕大小
    worldSelectionScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

    // 添加到屏幕栈（push 会自动调用 onOpen）
    screenStack->push(std::move(worldSelectionScreen));
}

void ClientApplication::showCreateWorld()
{
    spdlog::info("[Session] Showing create world");

    // 获取屏幕栈
    auto* screenStack = getScreenStackWidget(this);
    if (!screenStack) {
        spdlog::error("[Session] ScreenStackWidget not available");
        return;
    }

    // 创建世界创建屏幕
    auto createWorldScreen = std::make_unique<ui::minecraft::CreateWorldScreen>();

    // 设置回调
    createWorldScreen->setOnCreate([this, screenStack](const world::storage::CreateWorldRequest& request) {
        spdlog::info("[Session] Creating world: {}", request.displayName);

        // 关闭创建界面
        screenStack->pop();

        world::storage::GlobalStorageManager storageManager;
        auto levelIdResult = world::storage::WorldNameSanitizer::findAvailableLevelId(
            storageManager.savesDirectory(), request.displayName);

        if (!levelIdResult.success()) {
            spdlog::error("[Session] Failed to generate levelId: {}", levelIdResult.error().toString());
            return;
        }

        std::string levelId = levelIdResult.value();
        spdlog::info("[Session] Generated levelId: '{}' from displayName: '{}'", levelId, request.displayName);

        // 构建启动配置
        WorldLaunchConfig config;
        config.levelId = levelId;
        config.displayName = request.displayName;
        config.seed = static_cast<i64>(request.seed);
        config.worldType = request.worldType;
        config.defaultGameMode = request.gameMode;
        config.difficulty = request.difficulty;
        config.hardcore = request.hardcore;
        config.allowCommands = request.allowCommands;
        config.viewDistance = request.viewDistance > 0 ? request.viewDistance : m_settings.renderDistance.get();
        config.isNewWorld = true;

        // 启动世界
        auto result = startIntegratedWorld(config);
        if (result.failed()) {
            spdlog::error("[Session] Failed to create world: {}", result.error().toString());
            // 弹出错误对话框，告知用户创建失败的具体原因
            auto messageScreen = std::make_unique<ui::minecraft::MessageScreen>(
                "Failed to Create World", result.error().message(), "OK", [screenStack]() {
                    if (screenStack != nullptr) {
                        screenStack->pop();
                    }
                });
            messageScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));
            screenStack->push(std::move(messageScreen));
        }
    });

    createWorldScreen->setOnCancel([this, screenStack]() {
        // 返回存档选择
        screenStack->pop();
    });

    // 设置屏幕大小
    createWorldScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

    // 添加到屏幕栈（push 会自动调用 onOpen）
    screenStack->push(std::move(createWorldScreen));
}

void ClientApplication::showLoadingScreen()
{
    spdlog::info("[Session] Showing loading screen");

    // 获取屏幕栈
    auto* screenStack = getScreenStackWidget(this);
    if (!screenStack) {
        spdlog::error("[Session] ScreenStackWidget not available");
        return;
    }

    // 创建加载屏幕
    auto loadingScreen = std::make_unique<ui::minecraft::LoadingScreen>();

    // 保存原始指针用于更新进度
    m_loadingScreen = loadingScreen.get();

    // 设置屏幕大小
    loadingScreen->setBounds(ui::kagero::Rect(0, 0, m_guiScaleState.width, m_guiScaleState.height));

    // 添加到屏幕栈（push 会自动调用 onOpen）
    screenStack->push(std::move(loadingScreen));
}

void ClientApplication::hideLoadingScreen()
{
    if (m_loadingScreen) {
        spdlog::info("[Session] Hiding loading screen");
        m_loadingScreen = nullptr;

        // 获取屏幕栈
        auto* screenStack = getScreenStackWidget(this);
        if (screenStack) {
            screenStack->pop();
        }
    }
}

void ClientApplication::updateLoadingProgress(const std::string& stage, f32 progress)
{
    if (m_loadingScreen) {
        m_loadingScreen->setStage(stage);
        m_loadingScreen->setProgress(progress);
    }
}

} // namespace mc::client
