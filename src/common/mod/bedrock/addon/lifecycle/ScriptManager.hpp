#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/engine/QuickJSEngine.hpp"
#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptLogger.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptWatchdog.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginManager.hpp"

#include <memory>
#include <string>

namespace mc::mod::bedrock::addon {

class BehaviorPackList;
class ScriptTickListener;

/**
 * @brief 脚本管理器
 *
 * 顶层脚本系统协调器。拥有IScriptEngine、ScriptPluginManager、
 * ScriptEventBus、ScriptWatchdog、ScriptLogger等组件。
 *
 * 生命周期：
 * 1. initialize() — 创建引擎、注册模块工厂
 * 2. loadPacks() — 扫描行为包目录、加载插件
 * 3. startPlugins() — 启动所有插件
 * 4. tick() — 每tick由ScriptTickListener驱动
 * 5. shutdown() — 停止插件、关闭引擎
 */
class ScriptManager {
public:
    ScriptManager();
    ~ScriptManager();

    // 禁止拷贝
    ScriptManager(const ScriptManager&) = delete;
    ScriptManager& operator=(const ScriptManager&) = delete;

    /**
     * @brief 初始化脚本系统
     *
     * 创建QuickJS引擎、注册模块工厂、初始化事件总线。
     *
     * @return 初始化结果
     */
    Result<void> initialize();

    /**
     * @brief 关闭脚本系统
     *
     * 停止所有插件、关闭引擎、清理资源。
     */
    void shutdown();

    /**
     * @brief 从行为包目录加载插件
     * @param globalPackDir 全局行为包目录
     * @param worldPackDir 世界级行为包目录（可选）
     * @return 加载结果
     */
    Result<void> loadPacks(const std::string& globalPackDir, const std::string& worldPackDir = "");

    /**
     * @brief 启动所有已加载的插件
     */
    void startPlugins();

    /**
     * @brief 停止所有运行中的插件
     */
    void stopPlugins();

    /**
     * @brief 驱动所有插件的tick
     */
    void tickPlugins();

    /**
     * @brief 执行JS引擎的pending jobs
     */
    void executePendingJobs();

    /**
     * @brief 重新加载所有插件
     */
    void reload();

    /**
     * @brief 检查脚本系统是否已初始化
     */
    [[nodiscard]] bool isInitialized() const;

    // ===== 访问器 =====

    [[nodiscard]] IScriptEngine& engine();
    [[nodiscard]] const IScriptEngine& engine() const;
    [[nodiscard]] ScriptPluginManager& pluginManager();
    [[nodiscard]] const ScriptPluginManager& pluginManager() const;
    [[nodiscard]] ScriptEventBus& eventBus();
    [[nodiscard]] const ScriptEventBus& eventBus() const;
    [[nodiscard]] ScriptWatchdog& watchdog();
    [[nodiscard]] const ScriptWatchdog& watchdog() const;
    [[nodiscard]] ScriptLogger& logger();
    [[nodiscard]] const ScriptLogger& logger() const;
    [[nodiscard]] BehaviorPackList* packList();
    [[nodiscard]] const BehaviorPackList* packList() const;

private:
    /**
     * @brief 注册内置模块工厂
     */
    void registerBuiltinModules();

    std::unique_ptr<QuickJSEngine> m_engine;
    std::unique_ptr<ScriptPluginManager> m_pluginManager;
    std::unique_ptr<ScriptEventBus> m_eventBus;
    std::unique_ptr<ScriptWatchdog> m_watchdog;
    std::unique_ptr<ScriptLogger> m_logger;
    std::unique_ptr<BehaviorPackList> m_packList;
    std::unique_ptr<ScriptTickListener> m_tickListener;

    bool m_initialized = false;
};

} // namespace mc::mod::bedrock::addon
