#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/resource/ResourcePackList.hpp"
#include "common/util/math/random/Random.hpp"
#include "client/settings/ClientSettings.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/SoundHandler.hpp"
#include "client/sound/handler/BiomeAmbientHandler.hpp"
#include "client/sound/handler/UnderwaterAmbientHandler.hpp"
#include "../window/Window.hpp"
#include "../input/InputManager.hpp"
#include "../renderer/Camera.hpp"
#include "../resource/ResourceManager.hpp"
#include "../resource/BlockModelCache.hpp"
#include "../renderer/trident/core/TridentEngine.hpp"
#include "../ui/GuiScale.hpp"
#include "../renderer/trident/gui/GuiSpriteAtlas.hpp"
#include "../renderer/trident/gui/GuiTextureManager.hpp"
#include "../world/ClientWorld.hpp"
#include "../network/NetworkClient.hpp"
#include "../ui/kagero/KageroEngine.hpp"
#include "../ui/TridentCanvas.hpp"
#include "server/application/IntegratedServer.hpp"

#include <string>
#include <memory>
#include <atomic>
#include <filesystem>
#include <unordered_map>

namespace mc::client::command {
class ClientCommandManager;
}

namespace mc::client {

/**
 * @brief 客户端启动参数
 *
 * 用于命令行覆盖设置文件中的配置。
 */
struct ClientLaunchParams {
    // 窗口配置覆盖（可选）
    Optional<i32> windowWidth;
    Optional<i32> windowHeight;
    Optional<bool> fullscreen;

    // 服务器配置覆盖（可选）
    Optional<String> serverAddress;
    Optional<u16> serverPort;
    Optional<String> username;

    // 其他启动参数
    Optional<String> settingsPath;  // 自定义设置文件路径
    bool skipIntegratedServer = false;  // 跳过内置服务器
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
    [[nodiscard]] Result<void> initialize(const ClientLaunchParams& params = {});

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
     * @brief 获取声音引擎
     */
    [[nodiscard]] sound::SoundEngine& soundEngine() noexcept { return *m_soundEngine; }
    [[nodiscard]] const sound::SoundEngine& soundEngine() const noexcept { return *m_soundEngine; }

    /**
     * @brief 获取声音处理器
     */
    [[nodiscard]] sound::SoundHandler& soundHandler() noexcept { return *m_soundHandler; }
    [[nodiscard]] const sound::SoundHandler& soundHandler() const noexcept { return *m_soundHandler; }

    // 友元声明，用于回调
    friend void onWindowResize(i32 width, i32 height, void* userData);

private:
    void mainLoop();
    void handleEvents();
    void update(f32 deltaTime);
    void render();
    void shutdown();

    // 初始化辅助函数
    void setupInputBindings();
    void setupCamera();
    void setupNetworkCallbacks();
    void setupSettingCallbacks();
    void toggleMouseCapture();
    void handleBlockInteractionInput(f32 deltaTime);
    void handleBlockPlacementInput(f32 deltaTime);
    void sendBlockInteraction(network::BlockInteractionAction action,
                              const BlockPos& pos,
                              Direction face);
    void sendBlockPlacement(const BlockPos& pos, Direction face, const Vector3& hitPos);

    // 玩家位置同步
    void sendPlayerPosition();

    // 聊天命令处理
    void handleChatCommand(const String& input);

    // 加载设置
    [[nodiscard]] Result<void> loadSettings(const String& path);

    // 应用设置到系统
    void applySettings();

    /**
     * @brief 收集玩家补全候选项
     */
    [[nodiscard]] std::vector<String> collectPlayerCompletionCandidates() const;

    /**
     * @brief 收集实体补全候选项
     */
    [[nodiscard]] std::vector<String> collectEntityCompletionCandidates() const;

    // 初始化资源系统
    [[nodiscard]] Result<void> initializeResources();

    // 重新加载资源
    void reloadResources();

    /**
     * @brief 根据当前窗口尺寸和 GUI 缩放设置刷新逻辑 UI 尺寸
     */
    void applyGuiScale();

    ClientSettings m_settings;
    // 当前会话生效的设置文件路径（加载/自动保存/退出保存统一使用）
    std::filesystem::path m_settingsPath;
    Window m_window;
    InputManager m_input;
    std::unique_ptr<renderer::trident::TridentEngine> m_renderer;

    // 资源系统
    ResourcePackList m_resourcePackList;
    std::unique_ptr<ResourceManager> m_resourceManager;
    BlockModelCache m_modelCache;

    // GUI精灵图集（双图集架构：icons和widgets分离）
    std::unique_ptr<renderer::trident::gui::GuiSpriteAtlas> m_iconsAtlas;   // 心形、饥饿、盔甲、经验条等
    std::unique_ptr<renderer::trident::gui::GuiSpriteAtlas> m_widgetsAtlas; // 快捷栏、按钮等
    std::unique_ptr<renderer::trident::gui::GuiTextureManager> m_guiTextureManager; // GUI容器纹理管理器

    // 相机
    Camera m_camera;
    CameraController m_cameraController;
    bool m_mouseCaptured = false;

    // 世界
    ClientWorld m_world;

    // 物理系统
    std::unique_ptr<PhysicsEngine> m_physicsEngine;

    // 声音系统
    std::unique_ptr<sound::SoundHandler> m_soundHandler;
    std::unique_ptr<sound::SoundEngine> m_soundEngine;
    std::unique_ptr<sound::BiomeAmbientHandler> m_biomeAmbientHandler;
    std::unique_ptr<sound::UnderwaterAmbientHandler> m_underwaterAmbientHandler;

    // 玩家实体
    std::unique_ptr<Player> m_player;

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
    size_t m_chatLayerId = 0;
    size_t m_screenStackLayerId = 0;
    size_t m_debugScreenLayerId = 0;

    // 射线检测结果
    BlockRaycastResult m_raycastResult;
    bool m_breakingBlockActive = false;
    BlockPos m_breakingBlockPos{};
    Direction m_breakingBlockFace = Direction::None;
    f32 m_breakingBlockProgress = 0.0f;

    // 方块放置冷却
    f32 m_placeCooldown = 0.0f;
    static constexpr f32 PLACE_COOLDOWN_TIME = 0.05f;  // 50ms 放置冷却

    // 内置服务端
    std::unique_ptr<server::IntegratedServer> m_integratedServer;
    std::unique_ptr<NetworkClient> m_networkClient;
    std::unique_ptr<command::ClientCommandManager> m_commandManager;
    bool m_useIntegratedServer = true;

    std::unordered_map<PlayerId, String> m_knownPlayerNames;

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
    static constexpr f32 POSITION_SEND_INTERVAL = 1.0f / 20.0f;  // 20 TPS

    // 渲染时间（服务端时间不可用时使用本地回退）
    i64 m_renderGameTime = 0;
    i64 m_renderDayTime = 0;
    f32 m_renderTickAccumulator = 0.0f;
    bool m_hasServerTimeSync = false;

    // 玩家水中状态跟踪（用于音效触发）
    bool m_wasPlayerInWater = false;
    bool m_wasPlayerInLava = false;

    // 视野晃动状态
    f32 m_bobAngle = 0.0f;           // 晃动角度累计
    f32 m_bobPhase = 0.0f;           // 晃动相位

    // 随机数生成器（用于音调变化等）
    math::Random m_random;
};

} // namespace mc::client
