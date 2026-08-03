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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HAVING BEEN CLAIMED FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/mod/bedrock/addon/lifecycle/ScriptManager.hpp"
#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptLogger.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptTickListener.hpp"
#include "common/mod/bedrock/addon/lifecycle/ScriptWatchdog.hpp"
#include "common/mod/bedrock/addon/modules/MinecraftModuleFactory.hpp"
#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPackList.hpp"
#include "common/mod/bedrock/addon/plugin/ScriptPluginManager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptManager::ScriptManager()
    : m_engine(createScriptEngine())
    , m_pluginManager(std::make_unique<ScriptPluginManager>())
    , m_eventBus(std::make_unique<ScriptEventBus>())
    , m_watchdog(std::make_unique<ScriptWatchdog>(ScriptWatchdog::Config{}))
    , m_logger(std::make_unique<ScriptLogger>())
    , m_scheduler(std::make_unique<ScriptScheduler>())
    , m_packList(std::make_unique<BehaviorPackList>())
{}

ScriptManager::~ScriptManager()
{
    shutdown();
}

Result<void> ScriptManager::initialize()
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    spdlog::info("[BedrockAddon] Initializing script manager...");

    // 注册内置模块工厂
    _registerBuiltinModules();

    // 初始化引擎
    if (!m_engine->initialize()) {
        return Error(ErrorCode::InitializationFailed, "Failed to initialize script engine");
    }

    // 初始化事件总线
    m_eventBus->initialize();

    m_initialized = true;
    spdlog::info("[BedrockAddon] Script manager initialized");
    return Result<void>::ok();
}

void ScriptManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[BedrockAddon] Shutting down script manager...");

    // 停止所有插件
    m_pluginManager->stopAllPlugins();
    m_pluginManager->unloadAllPlugins();

    // 关闭事件总线
    m_eventBus->shutdown();

    // 清除调度器
    m_scheduler->clearAll();

    // 关闭引擎
    m_engine->shutdown();

    // 清空行为包列表
    m_packList->clear();

    m_initialized = false;
    spdlog::info("[BedrockAddon] Script manager shut down");
}

Result<void> ScriptManager::loadPacks(const std::string& globalPackDir, const std::string& worldPackDir)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Script manager not initialized");
    }

    spdlog::info("[BedrockAddon] Loading behavior packs...");

    // 扫描全局行为包目录
    if (!globalPackDir.empty()) {
        auto result = m_packList->scanDirectory(globalPackDir);
        if (result.failed()) {
            spdlog::warn("[BedrockAddon] Failed to scan global pack directory '{}': {}",
                globalPackDir,
                result.error().message());
        }
    }

    // 扫描世界级行为包目录
    if (!worldPackDir.empty()) {
        auto result = m_packList->scanDirectory(worldPackDir);
        if (result.failed()) {
            spdlog::warn(
                "[BedrockAddon] Failed to scan world pack directory '{}': {}", worldPackDir, result.error().message());
        }
    }

    if (m_packList->empty()) {
        spdlog::info("[BedrockAddon] No behavior packs found");
        return Result<void>::ok();
    }

    spdlog::info("[BedrockAddon] Found {} behavior pack(s)", m_packList->size());

    // 加载插件
    auto result = m_pluginManager->loadPlugins(*m_engine, *m_packList);
    if (result.failed()) {
        spdlog::error("[BedrockAddon] Failed to load plugins: {}", result.error().message());
        return result;
    }

    return Result<void>::ok();
}

void ScriptManager::startPlugins()
{
    if (!m_initialized) {
        return;
    }

    m_pluginManager->startAllPlugins();
}

void ScriptManager::stopPlugins()
{
    if (!m_initialized) {
        return;
    }

    m_pluginManager->stopAllPlugins();
}

void ScriptManager::tickPlugins()
{
    if (!m_initialized) {
        return;
    }

    m_pluginManager->tickPlugins();
}

void ScriptManager::executePendingJobs()
{
    if (!m_initialized) {
        return;
    }

    m_engine->runtime().executePendingJobs();
}

void ScriptManager::reload()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[BedrockAddon] Reloading scripts...");

    // 停止并卸载所有插件
    m_pluginManager->stopAllPlugins();
    m_pluginManager->unloadAllPlugins();

    // 清除调度器
    m_scheduler->clearAll();

    // 清空行为包列表
    m_packList->clear();

    // 重新加载（需要重新调用loadPacks）
    spdlog::info("[BedrockAddon] Scripts reloaded (call loadPacks() to reload behavior packs)");
}

bool ScriptManager::isInitialized() const
{
    return m_initialized;
}

IScriptEngine& ScriptManager::engine()
{
    return *m_engine;
}

const IScriptEngine& ScriptManager::engine() const
{
    return *m_engine;
}

ScriptPluginManager& ScriptManager::pluginManager()
{
    return *m_pluginManager;
}

const ScriptPluginManager& ScriptManager::pluginManager() const
{
    return *m_pluginManager;
}

ScriptEventBus& ScriptManager::eventBus()
{
    return *m_eventBus;
}

const ScriptEventBus& ScriptManager::eventBus() const
{
    return *m_eventBus;
}

ScriptWatchdog& ScriptManager::watchdog()
{
    return *m_watchdog;
}

const ScriptWatchdog& ScriptManager::watchdog() const
{
    return *m_watchdog;
}

ScriptLogger& ScriptManager::logger()
{
    return *m_logger;
}

const ScriptLogger& ScriptManager::logger() const
{
    return *m_logger;
}

ScriptScheduler& ScriptManager::scheduler()
{
    return *m_scheduler;
}

const ScriptScheduler& ScriptManager::scheduler() const
{
    return *m_scheduler;
}

BehaviorPackList* ScriptManager::packList()
{
    return m_packList.get();
}

const BehaviorPackList* ScriptManager::packList() const
{
    return m_packList.get();
}

void ScriptManager::_registerBuiltinModules()
{
    // 注册 @minecraft/server 模块
    auto minecraftFactory = std::make_unique<MinecraftModuleFactory>();
    minecraftFactory->setScheduler(m_scheduler.get());
    minecraftFactory->setEventBus(m_eventBus.get());
    if (!m_eventSignals.empty()) {
        minecraftFactory->setEventSignals(m_eventSignals);
    }
    m_engine->addModuleFactory(std::move(minecraftFactory));
    spdlog::info("[BedrockAddon] Registered @minecraft/server module factory");
}

void ScriptManager::setEventSignals(const std::vector<EventSignalInfo>& signals)
{
    m_eventSignals = signals;
}

} // namespace mc::mod::bedrock::addon
