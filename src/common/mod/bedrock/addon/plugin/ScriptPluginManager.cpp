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

#include "common/mod/bedrock/addon/plugin/ScriptPluginManager.hpp"
#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/core/ScriptException.hpp"
#include "common/mod/bedrock/addon/pack/AddonManifest.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"
#include "common/mod/bedrock/addon/plugin/PluginExecutionGroup.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPlugin.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginSource.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptPluginManager::ScriptPluginManager() = default;

ScriptPluginManager::~ScriptPluginManager()
{
    unloadAllPlugins();
}

Result<void> ScriptPluginManager::loadPlugins(IScriptEngine& engine, BehaviorPackList& packList)
{
    auto dependencies = packList.resolveDependencies();
    if (dependencies.failed()) return dependencies;
    const auto enabled = packList.getEnabledPacks();
    std::unordered_map<std::string, BehaviorPack*> scripts;
    for (auto* pack : enabled) {
        if (pack->manifest().hasScriptModule()) scripts.emplace(pack->uuid(), pack);
    }
    std::unordered_map<std::string, u8> state;
    std::vector<BehaviorPack*> ordered;
    std::function<Result<void>(BehaviorPack*)> visit = [&](BehaviorPack* pack) -> Result<void> {
        auto& status = state[pack->uuid()];
        if (status == 2) return Result<void>::ok();
        if (status == 1) return Error(ErrorCode::InvalidData, "Cyclic script pack dependency at: " + pack->uuid());
        status = 1;
        for (const auto& dependency : pack->manifest().dependencies) {
            const auto found = scripts.find(dependency.uuid);
            if (found == scripts.end()) continue;
            if (_determineExecutionGroup(found->second->manifest()) > _determineExecutionGroup(pack->manifest())) {
                return Error(ErrorCode::InvalidData, "Script pack depends on a later execution group: " + pack->uuid());
            }
            auto result = visit(found->second);
            if (result.failed()) return result;
        }
        status = 2;
        ordered.push_back(pack);
        return Result<void>::ok();
    };
    for (auto* pack : enabled) {
        if (pack->manifest().hasScriptModule()) {
            auto result = visit(pack);
            if (result.failed()) return result;
        }
    }
    std::vector<std::string> loaded;
    for (const auto group :
        {PluginExecutionGroup::PrePackLoad, PluginExecutionGroup::ServerStart, PluginExecutionGroup::ClientLevel}) {
        for (auto* pack : ordered) {
            if (m_plugins.contains(pack->uuid()) || _determineExecutionGroup(pack->manifest()) != group) continue;
            auto result = loadPlugin(engine, *pack);
            if (result.failed()) {
                for (auto it = loaded.rbegin(); it != loaded.rend(); ++it)
                    unloadPlugin(*it);
                return Error(
                    result.error().code(), "Failed to load pack '" + pack->name() + "': " + result.error().message());
            }
            loaded.push_back(pack->uuid());
        }
    }
    return Result<void>::ok();
}

Result<void> ScriptPluginManager::loadPlugin(IScriptEngine& engine, BehaviorPack& pack)
{
    // 检查是否已存在
    if (m_plugins.contains(pack.uuid())) {
        return Error(ErrorCode::AlreadyExists, "Plugin already loaded: " + pack.uuid());
    }

    // 创建脚本源
    auto source = std::make_unique<ScriptPluginSource>(pack);
    ScriptPluginSource* sourcePtr = source.get();
    m_sources[pack.uuid()] = std::move(source);

    // 确定执行分组
    auto execGroup = _determineExecutionGroup(pack.manifest());

    // 从manifest获取版本字符串
    std::string version = pack.manifest().header.version.toString();

    // 创建插件实例
    auto plugin = std::make_unique<ScriptPlugin>(pack.uuid(), pack.name(), version, execGroup);

    // 从manifest配置权限
    ScriptPackPermissions permissions(pack.manifest().capabilities);
    plugin->configuration().setPermissions(permissions);

    // 加载插件
    auto result = plugin->load(engine, *sourcePtr, *this);
    if (result.failed()) {
        // 清理源
        m_sources.erase(pack.uuid());
        return result;
    }

    spdlog::info("[BedrockAddon] Plugin '{}' loaded successfully", pack.name());
    m_plugins[pack.uuid()] = std::move(plugin);
    return Result<void>::ok();
}

void ScriptPluginManager::unloadPlugin(const std::string& uuid)
{
    auto it = m_plugins.find(uuid);
    if (it == m_plugins.end()) {
        return;
    }

    it->second->unload();
    m_plugins.erase(it);
    m_sources.erase(uuid);
    spdlog::info("[BedrockAddon] Plugin unloaded: {}", uuid);
}

void ScriptPluginManager::unloadAllPlugins()
{
    // 按执行分组的逆序卸载
    for (auto group :
        {PluginExecutionGroup::ClientLevel, PluginExecutionGroup::ServerStart, PluginExecutionGroup::PrePackLoad}) {
        for (auto it = m_plugins.begin(); it != m_plugins.end();) {
            if (it->second->executionGroup() == group) {
                it->second->unload();
                it = m_plugins.erase(it);
            } else {
                ++it;
            }
        }
    }

    m_sources.clear();
    spdlog::info("[BedrockAddon] All plugins unloaded");
}

void ScriptPluginManager::startAllPlugins()
{
    // 按执行分组顺序启动
    for (auto group :
        {PluginExecutionGroup::PrePackLoad, PluginExecutionGroup::ServerStart, PluginExecutionGroup::ClientLevel}) {
        for (auto& [uuid, plugin] : m_plugins) {
            if (plugin->executionGroup() == group && plugin->state() == ScriptPlugin::State::Loaded) {
                auto result = plugin->start();
                if (result.failed()) {
                    spdlog::error(
                        "[BedrockAddon] Failed to start plugin '{}': {}", plugin->name(), result.error().message());
                }
            }
        }
    }
}

void ScriptPluginManager::stopAllPlugins()
{
    for (auto& [uuid, plugin] : m_plugins) {
        plugin->stop();
    }
}

void ScriptPluginManager::tickPlugins()
{
    for (auto& [uuid, plugin] : m_plugins) {
        plugin->tick();
    }
}

ScriptPlugin* ScriptPluginManager::getPlugin(const std::string& uuid)
{
    auto it = m_plugins.find(uuid);
    return it != m_plugins.end() ? it->second.get() : nullptr;
}

std::vector<ScriptPlugin*> ScriptPluginManager::getRunningPlugins()
{
    std::vector<ScriptPlugin*> running;
    for (auto& [uuid, plugin] : m_plugins) {
        if (plugin->state() == ScriptPlugin::State::Running) {
            running.push_back(plugin.get());
        }
    }
    return running;
}

std::vector<const ScriptPlugin*> ScriptPluginManager::getAllPlugins() const
{
    std::vector<const ScriptPlugin*> plugins;
    plugins.reserve(m_plugins.size());
    for (const auto& [uuid, plugin] : m_plugins) {
        plugins.push_back(plugin.get());
    }
    return plugins;
}

size_t ScriptPluginManager::pluginCount() const
{
    return m_plugins.size();
}

size_t ScriptPluginManager::errorCount() const
{
    size_t count = 0;
    for (const auto& [uuid, plugin] : m_plugins) {
        if (plugin->state() == ScriptPlugin::State::Error) {
            ++count;
        }
    }
    return count;
}

std::optional<ScriptData> ScriptPluginManager::loadScript(const std::string& moduleName)
{
    // 遍历所有插件源查找模块
    for (auto& [uuid, source] : m_sources) {
        auto data = source->loadScript(moduleName);
        if (data.has_value()) {
            return data;
        }
    }
    return std::nullopt;
}

void ScriptPluginManager::onInfo(const std::string& message)
{
    spdlog::info("[Script] {}", message);
}

void ScriptPluginManager::onWarn(const std::string& message)
{
    spdlog::warn("[Script] {}", message);
}

void ScriptPluginManager::onError(const std::string& message)
{
    spdlog::error("[Script] {}", message);
}

void ScriptPluginManager::onException(const ScriptException& exception)
{
    spdlog::error("[Script] {}: {} at {}:{}",
        ScriptException::errorTypeName(exception.type()),
        exception.message(),
        exception.filename(),
        exception.line());
}

PluginExecutionGroup ScriptPluginManager::_determineExecutionGroup(const AddonManifest& manifest) const
{
    // 检查是否有早期执行能力
    if (manifest.hasCapability("earlyExec")) {
        return PluginExecutionGroup::PrePackLoad;
    }

    // 默认在服务器启动时执行
    return PluginExecutionGroup::ServerStart;
}

} // namespace mc::mod::bedrock::addon
