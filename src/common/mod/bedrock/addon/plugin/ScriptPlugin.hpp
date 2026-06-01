#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"

#include <memory>
#include <string>

namespace mc::mod::bedrock::addon {

class BehaviorPack;
class ScriptPluginSource;

/**
 * @brief 脚本插件实例
 *
 * 代表一个已加载的行为包脚本模块。
 * 每个ScriptPlugin拥有独立的IScriptContext用于隔离执行环境。
 *
 * 生命周期状态机：
 *   Unloaded → Loading → Loaded → Running → Unloading → Unloaded
 *                        ↘ Error
 */
class ScriptPlugin {
public:
    /**
     * @brief 插件状态
     */
    enum class State : u8 {
        /// 未加载
        Unloaded = 0,
        /// 正在加载
        Loading = 1,
        /// 已加载但未运行
        Loaded = 2,
        /// 正在运行
        Running = 3,
        /// 加载或运行出错
        Error = 4,
        /// 正在卸载
        Unloading = 5,
    };

    /**
     * @brief 构造函数
     * @param uuid 行为包UUID
     * @param name 插件名称
     * @param version 插件版本字符串
     * @param executionGroup 执行分组
     */
    ScriptPlugin(std::string uuid,
        std::string name,
        std::string version,
        PluginExecutionGroup executionGroup = PluginExecutionGroup::ServerStart);

    ~ScriptPlugin();

    // 禁止拷贝
    ScriptPlugin(const ScriptPlugin&) = delete;
    ScriptPlugin& operator=(const ScriptPlugin&) = delete;

    // 允许移动
    ScriptPlugin(ScriptPlugin&&) noexcept;
    ScriptPlugin& operator=(ScriptPlugin&&) noexcept;

    /**
     * @brief 加载插件
     *
     * 创建脚本上下文、注册模块绑定、执行入口脚本。
     * 状态从Unloaded → Loading → Loaded，失败则进入Error。
     *
     * @param engine 脚本引擎
     * @param source 脚本源加载器
     * @param printer 日志输出
     * @return 加载结果
     */
    Result<void> load(IScriptEngine& engine, IDependencyLoader& source, IScriptPrinter& printer);

    /**
     * @brief 启动插件
     *
     * 将插件从Loaded状态转入Running状态。
     * 在此阶段可以注册事件监听器等。
     *
     * @return 启动结果
     */
    Result<void> start();

    /**
     * @brief 停止插件
     *
     * 将插件从Running状态转入Loaded状态。
     * 移除事件监听器，停止调度任务。
     */
    void stop();

    /**
     * @brief 卸载插件
     *
     * 释放脚本上下文和所有关联资源。
     * 状态从Loaded/Running/Error → Unloading → Unloaded。
     */
    void unload();

    /**
     * @brief 执行一个tick
     *
     * 在Running状态下由ScriptTickListener每tick调用。
     * 处理JS引擎的待处理任务（Promise、setTimeout等）。
     */
    void tick();

    // ===== 访问器 =====

    [[nodiscard]] const std::string& uuid() const;
    [[nodiscard]] const std::string& name() const;
    [[nodiscard]] const std::string& version() const;
    [[nodiscard]] State state() const;
    [[nodiscard]] PluginExecutionGroup executionGroup() const;
    [[nodiscard]] const std::string& errorMessage() const;
    [[nodiscard]] IScriptContext* context();
    [[nodiscard]] const IScriptContext* context() const;
    [[nodiscard]] ScriptPackConfiguration& configuration();
    [[nodiscard]] const ScriptPackConfiguration& configuration() const;

    /**
     * @brief 获取状态名称
     */
    [[nodiscard]] static const char* stateName(State state);

private:
    std::string m_uuid;
    std::string m_name;
    std::string m_version;
    PluginExecutionGroup m_executionGroup;
    State m_state = State::Unloaded;

    std::unique_ptr<IScriptContext> m_context;
    ScriptPackConfiguration m_configuration;
    std::string m_errorMessage;
};

} // namespace mc::mod::bedrock::addon
