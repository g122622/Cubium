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

#pragma once

#include "client/application/ClientAppStateMachine.hpp"
#include "client/application/features/MemoryTraceThread.hpp"
#include "client/dimension/ClientDimensionManager.hpp"
#include "client/input/InputManager.hpp"
#include "client/network/ClientNetwork.hpp"
#include "client/network/ClientPlayVisitor.hpp"
#include "client/renderer/Camera.hpp"
#include "client/renderer/map/MapRenderer.hpp"
#include "client/renderer/trident/core/TridentEngine.hpp"
#include "client/renderer/trident/gui/GuiSpriteAtlas.hpp"
#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "client/settings/ClientSettings.hpp"
#include "client/skin/ClientSkinManager.hpp"
#include "client/ui/GuiScale.hpp"
#include "client/ui/TridentCanvas.hpp"
#include "client/ui/kagero/KageroEngine.hpp"
#include "client/window/Window.hpp"
#include "client/world/ClientMapDataCache.hpp"
#include "client/world/ClientWorld.hpp"
#include "client/world/player/ClientPlayerPredictor.hpp"
#include "client/world/player/LocalPlayerIdentity.hpp"
#include "client/world/player/PlayerIdentityRegistry.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/network/protocol/GameActions.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "server/application/IntegratedServer.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

namespace mc::client::command {
class ClientCommandManager;
}

namespace mc::client::sound {
class AudioService;
}

namespace mc::world {
struct DimensionRenderSettings;
}

namespace mc::client::ui::minecraft {
class LoadingScreen;
}

namespace mc::client::ui::minecraft::widgets {
class ChatWidget;
class ScreenStackWidget;
} // namespace mc::client::ui::minecraft::widgets

namespace mc::client {

/**
 * @brief 客户端启动参数
 *
 * 仅包含运行时行为参数和配置文件路径。
 * 所有配置值（窗口大小、用户名、服务器地址等）统一从配置文件读取，
 * 不通过命令行覆盖。
 */
struct ClientLaunchParams {
    // 配置文件路径（可选，未指定时使用默认路径）
    std::optional<std::string> configPath;

    // 运行时行为参数
    bool skipIntegratedServer = false;         // 跳过内置服务器
    bool benchmarkExitAfterInitialize = false; // benchmark 模式：initialize 完成后立即退出

    // Quick-play 选项（跳过主菜单直接进入世界）
    std::optional<std::string> quickPlayLevelId; // 直接加载指定世界
    bool quickPlayNew = false;                   // 直接创建新世界
};

/**
 * @brief 客户端应用
 */
class ClientApplication {
public:
    ClientApplication();
    ~ClientApplication();

    // 禁止拷贝
    ClientApplication(const ClientApplication&) = delete;
    ClientApplication& operator=(const ClientApplication&) = delete;

    /**
     * @brief 初始化客户端
     *
     * @param params 启动参数（可选覆盖）
     */
    [[nodiscard]] Result<void> initialize(const ClientLaunchParams& params);

    /**
     * @brief 运行客户端主循环
     */
    [[nodiscard]] Result<void> run();

    /**
     * @brief 停止客户端
     */
    void stop();

    /**
     * @brief 检查客户端是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

    /**
     * @brief 获取窗口
     */
    [[nodiscard]] Window& window() noexcept { return m_window; }
    [[nodiscard]] const Window& window() const noexcept { return m_window; }

    /**
     * @brief 获取输入管理器
     */
    [[nodiscard]] InputManager& input() noexcept { return m_input; }
    [[nodiscard]] const InputManager& input() const noexcept { return m_input; }

    /**
     * @brief 获取设置
     */
    [[nodiscard]] ClientSettings& settings() noexcept { return m_settings; }
    [[nodiscard]] const ClientSettings& settings() const noexcept { return m_settings; }

    /**
     * @brief 获取渲染器
     */
    [[nodiscard]] renderer::trident::TridentEngine& renderer() noexcept { return *m_renderer; }
    [[nodiscard]] const renderer::trident::TridentEngine& renderer() const noexcept { return *m_renderer; }

    /**
     * @brief 获取相机
     */
    [[nodiscard]] Camera& camera() noexcept { return m_camera; }
    [[nodiscard]] const Camera& camera() const noexcept { return m_camera; }

    /**
     * @brief 获取世界
     */
    [[nodiscard]] ClientWorld& world() noexcept { return m_world; }
    [[nodiscard]] const ClientWorld& world() const noexcept { return m_world; }

    /**
     * @brief 获取客户端统一计算池（ClientCompute）
     *
     * 进程级计算池，承接 chunkmesh 构建、皮肤异步加载等客户端计算任务。
     * 由 ClientApplication 持有，生命周期长于单个游戏会话；mesh 系统与皮肤管理器
     * 均通过注入引用/指针消费此池。
     */
    [[nodiscard]] util::UniversalWorkerPool& clientComputeWorkerPool() noexcept { return m_clientComputeWorkerPool; }
    [[nodiscard]] const util::UniversalWorkerPool& clientComputeWorkerPool() const noexcept
    {
        return m_clientComputeWorkerPool;
    }

    /**
     * @brief 获取皮肤管理器
     */
    [[nodiscard]] skin::ClientSkinManager& skinManager() noexcept { return *m_skinManager; }
    [[nodiscard]] const skin::ClientSkinManager& skinManager() const noexcept { return *m_skinManager; }

    /**
     * @brief 获取状态机
     */
    [[nodiscard]] ClientAppStateMachine& stateMachine() noexcept { return m_stateMachine; }
    [[nodiscard]] const ClientAppStateMachine& stateMachine() const noexcept { return m_stateMachine; }

    /**
     * @brief 获取维度管理器
     */
    [[nodiscard]] ClientDimensionManager& dimensionManager() noexcept { return m_dimensionManager; }
    [[nodiscard]] const ClientDimensionManager& dimensionManager() const noexcept { return m_dimensionManager; }

    // ========== 游戏会话管理 ==========

    /**
     * @brief 启动集成世界
     *
     * 从主菜单启动一个世界（新建或加载）。
     * 状态机必须处于 MainMenu 状态。
     *
     * @param config 世界启动配置
     * @return 是否成功开始加载
     */
    [[nodiscard]] Result<void> startIntegratedWorld(const WorldLaunchConfig& config);

    /**
     * @brief 销毁游戏会话
     *
     * 释放游戏相关资源：网络客户端、内置服务端、世界、玩家等。
     * 状态机必须处于 LeavingWorld 状态。
     */
    void destroyGameSession();

    /**
     * @brief 离开世界返回主菜单
     *
     * 从游戏中退出到主菜单。
     * 会触发保存、断开连接、销毁会话等操作。
     *
     * @return 是否成功开始离开流程
     */
    bool leaveWorldToMainMenu();

    /**
     * @brief 显示主菜单
     *
     * 切换到主菜单界面。仅在主菜单状态下可用。
     */
    void showMainMenu();

    /**
     * @brief 显示暂停菜单
     *
     * 在游戏中显示暂停菜单。
     * 状态机必须处于 InGame 状态。
     */
    void showPauseMenu();

    /**
     * @brief 显示存档选择界面
     *
     * 显示世界列表，允许玩家选择或创建世界。
     */
    void showWorldSelection();

    /**
     * @brief 显示创建世界界面
     *
     * 显示创建新世界的表单。
     */
    void showCreateWorld();

    /**
     * @brief 显示加载界面
     *
     * 显示世界加载进度界面。
     */
    void showLoadingScreen();

    /**
     * @brief 隐藏加载界面
     *
     * 隐藏加载进度界面。
     */
    void hideLoadingScreen();

    /**
     * @brief 更新加载进度
     *
     * @param stage 当前加载阶段描述
     * @param progress 进度（0.0 - 1.0）
     */
    void updateLoadingProgress(const std::string& stage, f32 progress);

    // 友元声明，用于回调
    friend void onWindowResize(i32 width, i32 height, void* userData);

    // 友元声明，用于屏幕操作辅助函数
    friend ui::minecraft::widgets::ScreenStackWidget* getScreenStackWidget(ClientApplication* app);

    // 友元声明，用于新网络层 visitor 直接操作私有成员（替代旧 setupNetworkCallbacks lambda 捕获 this）
    friend class ::mc::client::net::ClientPlayVisitor;

private:
    void mainLoop();
    void handleEvents();
    void update(f32 deltaTime);
    void render();
    void shutdown();
    void releaseRendererDependentResources();

    // ========== 状态机回调 ==========
    void onStateChanged(ClientAppState from, ClientAppState to);
    void setupStateMachineCallbacks();

    // 初始化辅助函数
    void initializeCoreRegistries();
    [[nodiscard]] Result<void> initializeWindowAndInput();
    [[nodiscard]] Result<void> initializeRenderer();

    // 外壳初始化（不包含游戏会话）
    [[nodiscard]] Result<void> initializeShell(const ClientLaunchParams& params);

    // 游戏会话初始化（启动世界时调用）
    [[nodiscard]] Result<void> initializeGameSession(const WorldLaunchConfig& config);

    void initializeUi();
    void setupInputBindings();
    void setupCamera();
    void setupNetworkCallbacks();
    void setupSettingCallbacks();
    void toggleMouseCapture();
    void handleBlockMiningInput(f32 deltaTime);
    void handleBlockPlacementInput(f32 deltaTime);
    void cancelBreakingBlock();
    void beginBreakingBlock(const BlockPos& currentTargetPos, Direction currentTargetFace, bool attackJustPressed);
    void completeBreakingBlock(bool instantBreak);
    void openInventoryScreen();
    void openCreativeScreen();
    /**
     * @brief 打开地图查看屏（纯客户端本地开屏）
     *
     * 玩家右键手持已填充地图时调用。1.21.11 原版 MapScreen 是客户端本地开，
     * 不走服务端 OpenScreen 下推（地图屏无容器菜单）。
     */
    void openMapScreen(i32 mapId);
    void closeInventoryScreenIfModeMismatch();
    [[nodiscard]] bool isCreativeModeActive() const;
    void sendBlockInteraction(network::BlockInteractionAction action, const BlockPos& pos, Direction face);
    void sendBlockPlacement(const BlockPos& pos, Direction face, const Vector3& hitPos);
    /// 右键空挥（raycast 未命中方块/实体）时发送 UseItem，触发服务端物品在空气中的使用。
    void sendUseItem();

    // 玩家位置同步
    void sendPlayerPosition();

    // 主动 ping（sb:37），对齐 Java PingDebugMonitor.tick，由 update 在调试屏可见时节流发送
    void sendPingRequest();

    // 难度切换（sb:3）/锁定（sb:28），由选项菜单回调触发，仅集成服有效
    void sendChangeDifficulty(Difficulty difficulty);
    void sendLockDifficulty();

    // 聊天命令处理
    void handleChatCommand(const std::string& input);

    // 加载设置
    [[nodiscard]] Result<void> loadSettings(const std::string& path);

    // 应用设置到系统
    void applySettings();

    /**
     * @brief 收集玩家补全候选项
     */
    [[nodiscard]] std::vector<std::string> collectPlayerCompletionCandidates() const;

    /**
     * @brief 收集实体补全候选项
     */
    [[nodiscard]] std::vector<std::string> collectEntityCompletionCandidates() const;

    /**
     * @brief 处理服务端广播的世界事件
     *
     * 对应 MC Java 的 LevelEventHandler.levelEvent()，根据事件ID播放音效和粒子效果。
     *
     * @param eventId 事件ID（参见 WorldEvents 命名空间）
     * @param x 事件位置 X
     * @param y 事件位置 Y
     * @param z 事件位置 Z
     * @param data 事件数据（含义因事件类型而异）
     */
    void _handleWorldEvent(i32 eventId, i32 x, i32 y, i32 z, i32 data);

    // 初始化资源系统
    [[nodiscard]] Result<void> initializeResources();

    // 初始化方块资产（computeBlockAppearances + BlockModelCache）。
    // 必须在 initializeAtlasManager 之后调用——方块外观的纹理区域来自 AtlasManager 的 blocks atlas。
    [[nodiscard]] Result<void> initializeBlockAssets();

    // 初始化音频系统
    [[nodiscard]] Result<void> initializeAudio();

    // 更新玩家音频
    void updatePlayerAudio();

    // 更新世界音频
    void updateWorldAudio();

    // 更新射线检测结果
    void updateRaycastResult();

    // 更新调试屏幕和准星目标信息
    void updateTargetInfoUi();

    // 更新音频暂停状态
    void updateAudioPauseState();

    // 关闭音频系统
    void shutdownAudio();

    // 更新客户端渲染时间与天气
    void updateTimeAndWeather(f32 deltaTime);

    // 更新云高度（根据当前维度）
    void updateCloudHeight();

    // 雨天按密度在相机附近生成雨滴粒子（对齐原版 WeatherEffectRenderer.tickRainParticles）
    void _tickRainParticles();

    // 获取维度渲染设置
    [[nodiscard]] world::DimensionRenderSettings getDimensionRenderSettings(DimensionId dimensionId) const;

    // 更新每帧 UI 状态（ScreenStackWidget 参数、KageroEngine tick 等）
    void updateUiFrameState(f32 deltaTime, f32 partialTick);

    // 处理覆盖层输入（聊天框、屏幕栈）
    [[nodiscard]] bool handleUiOverlayInput();

    // 处理游戏输入（热键、鼠标视角、移动）
    void handleGameplayInput();

    // 处理游戏热键（聊天、背包、飞行切换等）
    [[nodiscard]] bool handleGameplayShortcutInput(ui::minecraft::widgets::ChatWidget* chatWidget);

    // 处理鼠标视角、移动和快捷栏选择
    void handleMouseAndMovementInput();

    // 重新加载资源
    void reloadResources();

    /**
     * @brief 根据当前窗口尺寸和 GUI 缩放设置刷新逻辑 UI 尺寸
     */
    void applyGuiScale();

    ClientSettings m_settings;
    // 当前会话生效的设置文件路径（加载/自动保存/退出保存统一使用）
    std::filesystem::path m_settingsPath;
    // 游戏目录管理器（统一管理资源包、数据包、存档等路径）
    GameDirectory m_gameDirectory;
    Window m_window;
    InputManager m_input;
    std::unique_ptr<renderer::trident::TridentEngine> m_renderer;

    // 状态机
    ClientAppStateMachine m_stateMachine;

    // 启动参数（保存用于 quick-play）
    ClientLaunchParams m_launchParams;

    // 资源系统
    PackRepository m_resourcePackList;
    std::unique_ptr<ResourceManager> m_resourceManager;
    BlockModelCache m_modelCache;

    // GUI精灵图集（双图集架构：icons和widgets分离）
    std::unique_ptr<renderer::trident::gui::GuiSpriteAtlas> m_iconsAtlas;           // 心形、饥饿、盔甲、经验条等
    std::unique_ptr<renderer::trident::gui::GuiSpriteAtlas> m_widgetsAtlas;         // 快捷栏、按钮等
    std::unique_ptr<renderer::trident::gui::GuiTextureManager> m_guiTextureManager; // GUI容器纹理管理器

    // 地图渲染：MapRenderer 把 MapData 转 RGBA 纹理并逐像素绘制；ClientMapDataCache
    // 存储服务端下发的 MapData（客户端 ClientWorld 无 MapDataManager，只能由网络包还原）。
    std::unique_ptr<MapRenderer> m_mapRenderer;
    std::unique_ptr<ClientMapDataCache> m_mapDataCache;

    // 相机
    Camera m_camera;
    CameraController m_cameraController;
    bool m_mouseCaptured = false;

    // 世界
    ClientWorld m_world;

    // 维度管理器
    ClientDimensionManager m_dimensionManager;

    // 物理系统
    std::unique_ptr<PhysicsEngine> m_physicsEngine;

    // 音频系统
    // 注意：主线程不再直接持有 SoundEngine/SoundHandler（避免跨线程触碰 OpenAL）。
    // 所有音频逻辑通过 AudioService 投递命令，由独立音频线程执行。
    std::unique_ptr<sound::AudioService> m_audioService;

    // 玩家实体
    std::unique_ptr<Player> m_player;

    // 本地玩家身份（playerId ↔ entityId 映射）
    LocalPlayerIdentity m_localIdentity;

    // 玩家身份注册表（UUID ↔ entityId ↔ playerId ↔ username 多向映射，
    // 消除 static_cast<EntityInstanceId>(playerId) 反模式，供渲染层与皮肤层按 entityId 查 UUID）
    PlayerIdentityRegistry m_identityRegistry;

    // 客户端玩家预测器
    std::unique_ptr<ClientPlayerPredictor> m_predictor;

    // 调试屏幕可见性
    bool m_debugScreenVisible = true;

    // Kagero UI引擎
    std::unique_ptr<ui::kagero::KageroEngine> m_kageroEngine;
    std::unique_ptr<ui::TridentCanvas> m_canvas;
    ui::GuiScaleState m_guiScaleState{1, 0, 0};

    // Kagero 层 ID
    size_t m_crosshairLayerId = 0;
    size_t m_hudLayerId = 0;
    size_t m_targetInfoLayerId = 0;
    size_t m_titleLayerId = 0;
    size_t m_chatLayerId = 0;
    size_t m_screenStackLayerId = 0;
    size_t m_debugScreenLayerId = 0;

    // 当前加载屏幕（用于显示加载进度）
    ui::minecraft::LoadingScreen* m_loadingScreen = nullptr;

    // 射线检测结果
    BlockRaycastResult m_raycastResult;
    bool m_breakingBlockActive = false;
    BlockPos m_breakingBlockPos{};
    Direction m_breakingBlockFace = Direction::None;
    f32 m_breakingBlockProgress = 0.0f;

    // 方块放置冷却
    f32 m_placeCooldown = 0.0f;
    static constexpr f32 PLACE_COOLDOWN_TIME = 0.05f; // 50ms 放置冷却

    // 内置服务端
    std::unique_ptr<server::IntegratedServer> m_integratedServer;
    // 新网络层：ClientNetwork 持 Connection 驱动握手状态机 + 出站统一 send；
    // ClientPlayVisitor 处理 Play 阶段入站包。
    std::unique_ptr<net::ClientNetwork> m_network;
    std::unique_ptr<net::ClientPlayVisitor> m_playVisitor;
    std::unique_ptr<command::ClientCommandManager> m_commandManager;
    std::unique_ptr<skin::ClientSkinManager> m_skinManager;
    bool m_useIntegratedServer = true;

    std::unordered_map<PlayerId, std::string> m_knownPlayerNames;

    // 服务端品牌（Configuration 阶段 CustomPayload{minecraft:brand} 下发），供调试/UI 展示。
    std::string m_serverBrand;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    f64 m_lastFrameTime = 0.0;
    u64 m_frameCount = 0;

    // 位置同步（内部使用 f32，发送时转换为 f64）
    f32 m_lastSentX = 0.0f;
    f32 m_lastSentY = 0.0f;
    f32 m_lastSentZ = 0.0f;
    f32 m_lastSentYaw = 0.0f;
    f32 m_lastSentPitch = 0.0f;
    f32 m_positionSendAccumulator = 0.0f;
    f32 m_playerPhysicsAccumulator = 0.0f;
    f32 m_pingSendAccumulator = 0.0f; ///< 主动 ping(sb:37) 发送累加器，对齐 Java PingDebugMonitor 20 TPS
    static constexpr f32 PLAYER_PHYSICS_INTERVAL = constants::TICK_DURATION;
    static constexpr f32 POSITION_SEND_INTERVAL = constants::TICK_DURATION; // 20 TPS
    static constexpr f32 PING_SEND_INTERVAL = constants::TICK_DURATION;     // 20 TPS，对齐 Java PingDebugMonitor.tick

    // 渲染时间（服务端时间不可用时使用本地回退）
    i64 m_renderGameTime = 0;
    i64 m_renderDayTime = 0;
    f32 m_renderTickAccumulator = 0.0f;
    bool m_hasServerTimeSync = false;

    // 玩家水中状态跟踪（用于音效触发）
    bool m_wasPlayerInWater = false;
    bool m_wasPlayerInLava = false;
    bool m_wasInBubbleColumn = false;

    // 视野晃动状态
    glm::mat4 buildViewBobbingTransform(f32 partialTick) const;

    // 随机数生成器（用于音调变化等）
    math::Random m_random;

    // 心境音效采样位置随机数生成器（主线程侧，用于计算采样位置光照）
    math::Random m_moodRng;

    // 内存追踪线程（独立线程采样，避免阻塞主循环）
    MemoryTraceThread m_memoryTraceThread;

    // 客户端统一计算池（ClientCompute）。进程级，承接 chunkmesh/皮肤等客户端计算任务。
    // 由 ClientApplication 持有：initializeShell 阶段 start()，shutdown 阶段（销毁 mesh 系统/皮肤
    // 管理器之后）shutdown()。生命周期间长于 ClientWorld（会话级），故不放在 ClientWorld。
    util::UniversalWorkerPool m_clientComputeWorkerPool{-1, "ClientCompute", 300};
};

} // namespace mc::client
