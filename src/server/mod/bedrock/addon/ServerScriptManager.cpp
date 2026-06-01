#include "server/mod/bedrock/addon/ServerScriptManager.hpp"

#include <spdlog/spdlog.h>

namespace mc::server {

ServerScriptManager::ServerScriptManager(const std::string& globalBehaviorPackDir)
    : m_scriptManager(std::make_unique<mc::mod::bedrock::addon::ScriptManager>())
    , m_globalBehaviorPackDir(globalBehaviorPackDir)
{}

ServerScriptManager::~ServerScriptManager()
{
    shutdown();
}

Result<void> ServerScriptManager::initialize()
{
    if (m_initialized) {
        return Result<void>::ok();
    }

    spdlog::info("[Server] Initializing script system...");

    auto result = m_scriptManager->initialize();
    if (result.failed()) {
        return result;
    }

    m_initialized = true;
    spdlog::info("[Server] Script system initialized");
    return Result<void>::ok();
}

Result<void> ServerScriptManager::loadPlugins(const std::string& worldBehaviorPackDir)
{
    if (!m_initialized) {
        return Error(ErrorCode::NotInitialized, "Script system not initialized");
    }

    spdlog::info("[Server] Loading behavior packs...");

    auto result = m_scriptManager->loadPacks(m_globalBehaviorPackDir, worldBehaviorPackDir);
    if (result.failed()) {
        spdlog::error("[Server] Failed to load behavior packs: {}", result.error().message());
        return result;
    }

    // 自动启动所有已加载的插件
    m_scriptManager->startPlugins();

    spdlog::info("[Server] Behavior packs loaded and plugins started");
    return Result<void>::ok();
}

void ServerScriptManager::startPlugins()
{
    if (!m_initialized) {
        return;
    }

    m_scriptManager->startPlugins();
}

void ServerScriptManager::tick()
{
    if (!m_initialized) {
        return;
    }

    auto& tickListener = *m_scriptManager; // 通过ScriptManager间接调用
    // tick流程：
    // 1. 开始tick（看门狗计时）
    m_scriptManager->watchdog().beginTick();

    // 2. 执行JS pending jobs和插件tick
    m_scriptManager->tickPlugins();
    m_scriptManager->executePendingJobs();

    // 3. 刷新afterEvent队列
    m_scriptManager->eventBus().tick();

    // 4. 结束tick（看门狗检查）
    m_scriptManager->watchdog().endTick();
    m_scriptManager->watchdog().tick(*m_scriptManager);
}

void ServerScriptManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    spdlog::info("[Server] Shutting down script system...");
    m_scriptManager->shutdown();
    m_initialized = false;
    spdlog::info("[Server] Script system shut down");
}

void ServerScriptManager::reload()
{
    if (!m_initialized) {
        return;
    }

    m_scriptManager->reload();
}

bool ServerScriptManager::isInitialized() const
{
    return m_initialized;
}

mc::mod::bedrock::addon::ScriptManager& ServerScriptManager::scriptManager()
{
    return *m_scriptManager;
}

const mc::mod::bedrock::addon::ScriptManager& ServerScriptManager::scriptManager() const
{
    return *m_scriptManager;
}

void ServerScriptManager::setGlobalBehaviorPackDir(const std::string& dir)
{
    m_globalBehaviorPackDir = dir;
}

} // namespace mc::server
