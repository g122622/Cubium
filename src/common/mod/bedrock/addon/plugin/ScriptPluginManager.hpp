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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackConfiguration.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

class BehaviorPack;
class BehaviorPackList;

/**
 * @brief 脚本插件管理器
 *
 * 负责发现、加载、卸载脚本插件。
 * 管理插件的生命周期，协调依赖解析和加载顺序。
 * 同时实现IDependencyLoader（加载模块源码）和IScriptPrinter（日志输出）。
 */
class ScriptPluginManager : public IDependencyLoader, public IScriptPrinter {
public:
    ScriptPluginManager();
    ~ScriptPluginManager() override;

    // 禁止拷贝
    ScriptPluginManager(const ScriptPluginManager&) = delete;
    ScriptPluginManager& operator=(const ScriptPluginManager&) = delete;

    /**
     * @brief 从行为包列表发现并加载插件
     *
     * 扫描BehaviorPackList中包含脚本模块的行为包，
     * 解析依赖关系，按优先级和依赖顺序加载插件。
     *
     * @param engine 脚本引擎
     * @param packList 行为包列表
     * @return 加载结果
     */
    Result<void> loadPlugins(IScriptEngine& engine, BehaviorPackList& packList);

    /**
     * @brief 加载单个行为包作为插件
     * @param engine 脚本引擎
     * @param pack 行为包
     * @return 加载结果
     */
    Result<void> loadPlugin(IScriptEngine& engine, BehaviorPack& pack);

    /**
     * @brief 卸载指定UUID的插件
     * @param uuid 插件UUID
     */
    void unloadPlugin(const std::string& uuid);

    /**
     * @brief 卸载所有插件
     */
    void unloadAllPlugins();

    /**
     * @brief 启动所有已加载的插件
     *
     * 按执行分组顺序启动：PrePackLoad → ServerStart → ClientLevel
     */
    void startAllPlugins();

    /**
     * @brief 停止所有运行中的插件
     */
    void stopAllPlugins();

    /**
     * @brief 对所有运行中的插件执行tick
     */
    void tickPlugins();

    /**
     * @brief 获取指定UUID的插件
     * @param uuid 插件UUID
     * @return 插件指针，未找到返回nullptr
     */
    [[nodiscard]] ScriptPlugin* getPlugin(const std::string& uuid);

    /**
     * @brief 获取所有运行中的插件
     */
    [[nodiscard]] std::vector<ScriptPlugin*> getRunningPlugins();

    /**
     * @brief 获取所有插件
     */
    [[nodiscard]] std::vector<const ScriptPlugin*> getAllPlugins() const;

    /**
     * @brief 获取插件数量
     */
    [[nodiscard]] size_t pluginCount() const;

    /**
     * @brief 获取处于错误状态的插件数量
     */
    [[nodiscard]] size_t errorCount() const;

    // ===== IDependencyLoader =====

    /**
     * @brief 从已注册的包源中加载模块脚本
     *
     * 遍历所有插件源查找指定模块。
     */
    [[nodiscard]] std::optional<ScriptData> loadScript(const std::string& moduleName) override;

    // ===== IScriptPrinter =====

    void onInfo(const std::string& message) override;
    void onWarn(const std::string& message) override;
    void onError(const std::string& message) override;
    void onException(const ScriptException& exception) override;

private:
    /**
     * @brief 根据manifest确定执行分组
     */
    [[nodiscard]] PluginExecutionGroup _determineExecutionGroup(const AddonManifest& manifest) const;

    std::unordered_map<std::string, std::unique_ptr<ScriptPlugin>> m_plugins;
    std::unordered_map<std::string, std::unique_ptr<ScriptPluginSource>> m_sources;
};

} // namespace mc::mod::bedrock::addon
