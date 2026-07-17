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

#include "common/core/Types.hpp"
#include "common/world/WorldConfig.hpp"
#include <functional>
#include <string>

namespace mc::client {

/**
 * @brief 客户端应用状态
 *
 * 定义客户端应用的生命周期状态。状态转换是单向的，不允许跳过中间状态。
 */
enum class ClientAppState : u8 {
    /// 初始化中（应用启动到外壳初始化完成）
    Initializing,

    /// 主菜单（默认启动状态）
    MainMenu,

    /// 正在加载世界（显示加载界面）
    LoadingWorld,

    /// 游戏中
    InGame,

    /// 暂停菜单（游戏中按 ESC 打开）
    Paused,

    /// 正在离开世界（保存中，显示保存界面）
    LeavingWorld,

    /// 正在关闭（应用退出）
    ShuttingDown,
};

/**
 * @brief 世界启动配置
 *
 * 包含启动一个世界所需的所有参数。
 */
struct WorldLaunchConfig {
    /// 世界目录名（如 "New World"）
    std::string levelId;

    /// 显示名称
    std::string displayName;

    /// 世界种子
    i64 seed = 0;

    /// 世界类型（使用 mc::WorldType）
    mc::WorldType worldType = mc::WorldType::Default;

    /// 世界预设资源位置（数据驱动装配查 WorldPresetRegistry，如 "minecraft:default"）
    resource::ResourceLocation worldPresetId{"minecraft", "default"};

    /// 默认游戏模式
    GameMode defaultGameMode = GameMode::Survival;

    /// 难度
    Difficulty difficulty = Difficulty::Normal;

    /// 是否启用作弊
    bool allowCommands = false;

    /// 是否为 hardcore 模式
    bool hardcore = false;

    /// 视距
    i32 viewDistance = 12;

    /// 是否为新创建的世界（需要写入初始 level.dat）
    bool isNewWorld = false;
};

/**
 * @brief 客户端应用状态机
 *
 * 集中管理客户端应用的状态转换和生命周期事件。
 * 提供状态查询、状态转换验证和状态变更通知。
 *
 * 状态转换图：
 * ```
 * Initializing -> MainMenu -> LoadingWorld -> InGame <-> Paused
 *                  ^                              |
 *                  |                              v
 *                  +-------- LeavingWorld <-------+
 *                               |
 *                               v
 *                          ShuttingDown
 * ```
 *
 * 使用示例：
 * @code
 * ClientAppStateMachine stateMachine;
 *
 * // 查询状态
 * if (stateMachine.isInGame()) {
 *     // 处理游戏输入
 * }
 *
 * // 状态转换
 * if (stateMachine.canStartWorld()) {
 *     stateMachine.transitionToLoadingWorld(config);
 * }
 *
 * // 监听状态变更
 * stateMachine.setOnStateChanged([](ClientAppState from, ClientAppState to) {
 *     spdlog::info("State changed: {} -> {}", stateToString(from), stateToString(to));
 * });
 * @endcode
 */
class ClientAppStateMachine {
public:
    /**
     * @brief 状态变更回调类型
     */
    using StateChangeCallback = std::function<void(ClientAppState from, ClientAppState to)>;

    /**
     * @brief 世界加载进度回调类型
     */
    using LoadingProgressCallback = std::function<void(const std::string& stage, f32 progress)>;

    ClientAppStateMachine() = default;
    ~ClientAppStateMachine() = default;

    // 禁止拷贝
    ClientAppStateMachine(const ClientAppStateMachine&) = delete;
    ClientAppStateMachine& operator=(const ClientAppStateMachine&) = delete;

    // 允许移动
    ClientAppStateMachine(ClientAppStateMachine&&) noexcept = default;
    ClientAppStateMachine& operator=(ClientAppStateMachine&&) noexcept = default;

    // ========== 状态查询 ==========

    [[nodiscard]] ClientAppState state() const noexcept { return m_state; }

    [[nodiscard]] bool isInitializing() const noexcept { return m_state == ClientAppState::Initializing; }

    [[nodiscard]] bool isInMainMenu() const noexcept { return m_state == ClientAppState::MainMenu; }

    [[nodiscard]] bool isLoadingWorld() const noexcept { return m_state == ClientAppState::LoadingWorld; }

    [[nodiscard]] bool isInGame() const noexcept { return m_state == ClientAppState::InGame; }

    [[nodiscard]] bool isPaused() const noexcept { return m_state == ClientAppState::Paused; }

    [[nodiscard]] bool isLeavingWorld() const noexcept { return m_state == ClientAppState::LeavingWorld; }

    [[nodiscard]] bool isShuttingDown() const noexcept { return m_state == ClientAppState::ShuttingDown; }

    /**
     * @brief 是否有活跃的游戏会话
     *
     * 包含 InGame、Paused、LeavingWorld 状态，用于判断是否需要清理游戏资源。
     */
    [[nodiscard]] bool hasActiveGameSession() const noexcept
    {
        return m_state == ClientAppState::InGame || m_state == ClientAppState::Paused ||
            m_state == ClientAppState::LeavingWorld;
    }

    /**
     * @brief 是否可以捕获鼠标
     * 只有 InGame 状态下才能捕获鼠标。
     */
    [[nodiscard]] bool canCaptureMouse() const noexcept { return m_state == ClientAppState::InGame; }

    /**
     * @brief 是否可以暂停
     * 只有 InGame 状态下才能打开暂停菜单。
     */
    [[nodiscard]] bool canPause() const noexcept { return m_state == ClientAppState::InGame; }

    [[nodiscard]] bool canResumeFromPause() const noexcept { return m_state == ClientAppState::Paused; }

    [[nodiscard]] bool canStartWorld() const noexcept { return m_state == ClientAppState::MainMenu; }

    [[nodiscard]] bool canReturnToMainMenu() const noexcept
    {
        return m_state == ClientAppState::Paused || m_state == ClientAppState::LeavingWorld;
    }

    [[nodiscard]] bool canShutdown() const noexcept
    {
        return m_state == ClientAppState::MainMenu || m_state == ClientAppState::LeavingWorld;
    }

    /**
     * @brief 是否需要渲染游戏世界
     * 包含 InGame、Paused、LoadingWorld 状态。
     */
    [[nodiscard]] bool needsWorldRendering() const noexcept
    {
        return m_state == ClientAppState::InGame || m_state == ClientAppState::Paused ||
            m_state == ClientAppState::LoadingWorld;
    }

    /**
     * @brief 是否需要渲染 HUD
     * 包含 InGame、Paused 状态。
     */
    [[nodiscard]] bool needsHudRendering() const noexcept
    {
        return m_state == ClientAppState::InGame || m_state == ClientAppState::Paused;
    }

    [[nodiscard]] bool needsGameTick() const noexcept { return m_state == ClientAppState::InGame; }

    // ========== 状态转换 ==========

    /**
     * @brief 完成初始化，进入主菜单
     *
     * 只能从 Initializing 状态调用。
     *
     * @return 是否成功转换
     */
    bool finishInitializing()
    {
        if (m_state != ClientAppState::Initializing) {
            return false;
        }
        _transitionTo(ClientAppState::MainMenu);
        return true;
    }

    /**
     * @brief 开始加载世界
     *
     * 只能从 MainMenu 状态调用。
     *
     * @param config 世界启动配置
     * @return 是否成功转换
     */
    bool startLoadingWorld(const WorldLaunchConfig& config)
    {
        if (m_state != ClientAppState::MainMenu) {
            return false;
        }
        m_worldConfig = config;
        _transitionTo(ClientAppState::LoadingWorld);
        return true;
    }

    /**
     * @brief 完成世界加载，进入游戏
     *
     * 只能从 LoadingWorld 状态调用。
     *
     * @return 是否成功转换
     */
    bool finishLoadingWorld()
    {
        if (m_state != ClientAppState::LoadingWorld) {
            return false;
        }
        _transitionTo(ClientAppState::InGame);
        return true;
    }

    /**
     * @brief 暂停游戏
     *
     * 只能从 InGame 状态调用。
     *
     * @return 是否成功转换
     */
    bool pause()
    {
        if (m_state != ClientAppState::InGame) {
            return false;
        }
        _transitionTo(ClientAppState::Paused);
        return true;
    }

    /**
     * @brief 从暂停恢复游戏
     *
     * 只能从 Paused 状态调用。
     *
     * @return 是否成功转换
     */
    bool resume()
    {
        if (m_state != ClientAppState::Paused) {
            return false;
        }
        _transitionTo(ClientAppState::InGame);
        return true;
    }

    /**
     * @brief 开始离开世界（保存中）
     *
     * 只能从 InGame 或 Paused 状态调用。
     *
     * @return 是否成功转换
     */
    bool startLeavingWorld()
    {
        if (m_state != ClientAppState::InGame && m_state != ClientAppState::Paused) {
            return false;
        }
        _transitionTo(ClientAppState::LeavingWorld);
        return true;
    }

    /**
     * @brief 完成离开世界，返回主菜单
     *
     * 只能从 LeavingWorld 状态调用。
     *
     * @return 是否成功转换
     */
    bool finishLeavingWorld()
    {
        if (m_state != ClientAppState::LeavingWorld) {
            return false;
        }
        m_worldConfig.reset();
        _transitionTo(ClientAppState::MainMenu);
        return true;
    }

    /**
     * @brief 开始关闭应用
     *
     * 可以从 MainMenu 或 LeavingWorld 状态调用。
     *
     * @return 是否成功转换
     */
    bool startShutdown()
    {
        if (!canShutdown()) {
            return false;
        }
        _transitionTo(ClientAppState::ShuttingDown);
        return true;
    }

    /**
     * @brief 强制设置状态（用于异常恢复）
     *
     * 跳过状态验证，直接设置状态。
     * 仅用于断开连接等异常情况下的状态恢复。
     *
     * @param newState 新状态
     */
    void forceState(ClientAppState newState)
    {
        if (m_state != newState) {
            ClientAppState oldState = m_state;
            m_state = newState;
            _notifyStateChange(oldState, newState);
        }
    }

    // ========== 回调设置 ==========

    /**
     * @brief 设置状态变更回调
     */
    void setOnStateChanged(StateChangeCallback callback) { m_onStateChanged = std::move(callback); }

    /**
     * @brief 设置加载进度回调
     */
    void setLoadingProgressCallback(LoadingProgressCallback callback) { m_onLoadingProgress = std::move(callback); }

    // ========== 世界配置访问 ==========

    /**
     * @brief 获取当前世界启动配置
     *
     * 仅在 LoadingWorld、InGame、Paused、LeavingWorld 状态有效。
     */
    [[nodiscard]] const std::optional<WorldLaunchConfig>& worldConfig() const noexcept { return m_worldConfig; }

    /**
     * @brief 报告加载进度（仅 LoadingWorld 状态有效）
     *
     * @param stage 当前加载阶段名称
     * @param progress 进度（0.0 - 1.0）
     */
    void reportLoadingProgress(const std::string& stage, f32 progress)
    {
        if (m_state == ClientAppState::LoadingWorld && m_onLoadingProgress) {
            m_onLoadingProgress(stage, progress);
        }
    }

    // ========== 工具方法 ==========

    /**
     * @brief 将状态转换为字符串
     */
    [[nodiscard]] static const char* stateToString(ClientAppState state) noexcept
    {
        switch (state) {
            case ClientAppState::Initializing:
                return "Initializing";
            case ClientAppState::MainMenu:
                return "MainMenu";
            case ClientAppState::LoadingWorld:
                return "LoadingWorld";
            case ClientAppState::InGame:
                return "InGame";
            case ClientAppState::Paused:
                return "Paused";
            case ClientAppState::LeavingWorld:
                return "LeavingWorld";
            case ClientAppState::ShuttingDown:
                return "ShuttingDown";
            default:
                return "Unknown";
        }
    }

private:
    ClientAppState m_state = ClientAppState::Initializing;
    std::optional<WorldLaunchConfig> m_worldConfig;

    StateChangeCallback m_onStateChanged;
    LoadingProgressCallback m_onLoadingProgress;

    void _transitionTo(ClientAppState newState)
    {
        ClientAppState oldState = m_state;
        m_state = newState;
        _notifyStateChange(oldState, newState);
    }

    void _notifyStateChange(ClientAppState from, ClientAppState to)
    {
        if (m_onStateChanged) {
            m_onStateChanged(from, to);
        }
    }
};

} // namespace mc::client
