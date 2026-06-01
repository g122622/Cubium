#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"

#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptEventBus::ScriptEventBus()
    : m_beforeEvents(std::make_unique<BeforeEventSignal>())
    , m_afterEvents(std::make_unique<AfterEventSignal>())
{}

ScriptEventBus::~ScriptEventBus() {
    shutdown();
}

void ScriptEventBus::initialize() {
    if (m_initialized) {
        return;
    }
    spdlog::info("[BedrockAddon] ScriptEventBus initialized");
    m_initialized = true;
}

void ScriptEventBus::shutdown() {
    if (!m_initialized) {
        return;
    }
    m_beforeEvents->clear();
    m_afterEvents->clear();
    m_initialized = false;
    spdlog::info("[BedrockAddon] ScriptEventBus shut down");
}

void ScriptEventBus::tick() {
    if (!m_initialized) {
        return;
    }

    // 处理afterEvent队列（延迟批量处理）
    m_afterEvents->preFlush();
    m_afterEvents->flush();
    m_afterEvents->postFlush();
}

void ScriptEventBus::enqueueAfterEvent(std::type_index eventType, std::any eventData) {
    if (!m_initialized) {
        return;
    }
    m_afterEvents->enqueue(eventType, std::move(eventData));
}

bool ScriptEventBus::dispatchBeforeEvent(std::type_index eventType, std::any& eventData) {
    if (!m_initialized) {
        return false;
    }
    return m_beforeEvents->fire(eventType, eventData);
}

bool ScriptEventBus::hasPendingAfterEvents() const {
    return m_afterEvents->pendingCount() > 0;
}

} // namespace mc::mod::bedrock::addon
