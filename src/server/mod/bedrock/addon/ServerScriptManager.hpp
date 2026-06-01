#pragma once

#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"

#include <memory>
#include <string>

namespace mc::server {

/**
 * @brief 服务端脚本管理器
 *
 * 继承自ScriptManager，添加服务端特定的集成逻辑。
 * 与MinecraftServer生命周期绑定，提供世界级行为包目录。
 */
class ServerScriptManager {
public:
    /**
     * @brief 构造函数
     * @param globalBehaviorPackDir 全局行为包目录路径
     */
    explicit ServerScriptManager(const std::string& globalBehaviorPackDir = "behavior_packs");

    ~ServerScriptManager();

    // 禁止拷贝
    ServerScriptManager(const ServerScriptManager&) = delete;
    ServerScriptManager& operator=(const ServerScriptManager&) = delete;

    /**
     * @brief 初始化脚本系统
     *
     * 创建引擎、注册模块工厂、初始化事件总线。
     * 在MinecraftServer::initializeCoreManagers()中调用。
     *
     * @return 初始化结果
     */
    Result<void> initialize();

    /**
     * @brief 加载行为包和插件
     *
     * 扫描全局和世界级行为包目录，发现并加载脚本插件。
     * 在服务器启动完成后调用。
     *
     * @param worldBehaviorPackDir 世界级行为包目录路径
     * @return 加载结果
     */
    Result<void> loadPlugins(const std::string& worldBehaviorPackDir = "");

    /**
     * @brief 启动所有插件
     */
    void startPlugins();

    /**
     * @brief 每tick调用
     *
     * 在MinecraftServer::tick()中调用，驱动脚本执行。
     * 处理beforeEvents、pending jobs、afterEvents。
     */
    void tick();

    /**
     * @brief 关闭脚本系统
     *
     * 在MinecraftServer::shutdown()中调用。
     */
    void shutdown();

    /**
     * @brief 重新加载所有插件
     */
    void reload();

    /**
     * @brief 检查脚本系统是否已初始化
     */
    [[nodiscard]] bool isInitialized() const;

    /**
     * @brief 获取底层脚本管理器
     */
    [[nodiscard]] mc::mod::bedrock::addon::ScriptManager& scriptManager();
    [[nodiscard]] const mc::mod::bedrock::addon::ScriptManager& scriptManager() const;

    /**
     * @brief 设置全局行为包目录
     */
    void setGlobalBehaviorPackDir(const std::string& dir);

private:
    std::unique_ptr<mc::mod::bedrock::addon::ScriptManager> m_scriptManager;
    std::string m_globalBehaviorPackDir;
    bool m_initialized = false;
};

} // namespace mc::server
