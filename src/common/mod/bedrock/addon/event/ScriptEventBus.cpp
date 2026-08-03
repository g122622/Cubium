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

#include "common/mod/bedrock/addon/event/ScriptEventBus.hpp"
#include "common/mod/bedrock/addon/event/AfterEventSignal.hpp"
#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"

#include <any>
#include <memory>
#include <typeindex>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ScriptEventBus::ScriptEventBus()
    : m_beforeEvents(std::make_unique<BeforeEventSignal>())
    , m_afterEvents(std::make_unique<AfterEventSignal>())
{}

ScriptEventBus::~ScriptEventBus()
{
    shutdown();
}

ScriptEventBus::ScriptEventBus(ScriptEventBus&& other) noexcept
    : m_beforeEvents(std::move(other.m_beforeEvents))
    , m_afterEvents(std::move(other.m_afterEvents))
    , m_initialized(other.m_initialized)
{
    other.m_initialized = false;
}

ScriptEventBus& ScriptEventBus::operator=(ScriptEventBus&& other) noexcept
{
    if (this != &other) {
        shutdown();
        m_beforeEvents = std::move(other.m_beforeEvents);
        m_afterEvents = std::move(other.m_afterEvents);
        m_initialized = other.m_initialized;
        other.m_initialized = false;
    }
    return *this;
}

void ScriptEventBus::initialize()
{
    if (m_initialized) {
        return;
    }
    spdlog::info("[BedrockAddon] ScriptEventBus initialized");
    m_initialized = true;
}

void ScriptEventBus::shutdown()
{
    if (!m_initialized) {
        return;
    }
    m_beforeEvents->clear();
    m_afterEvents->clear();
    m_initialized = false;
    spdlog::info("[BedrockAddon] ScriptEventBus shut down");
}

void ScriptEventBus::tick()
{
    if (!m_initialized) {
        return;
    }

    // 处理afterEvent队列（延迟批量处理）
    m_afterEvents->preFlush();
    m_afterEvents->flush();
    m_afterEvents->postFlush();
}

void ScriptEventBus::enqueueAfterEvent(std::type_index eventType, std::any eventData)
{
    if (!m_initialized) {
        return;
    }
    m_afterEvents->enqueue(eventType, std::move(eventData));
}

bool ScriptEventBus::dispatchBeforeEvent(std::type_index eventType, std::any& eventData)
{
    if (!m_initialized) {
        return false;
    }
    return m_beforeEvents->fire(eventType, eventData);
}

bool ScriptEventBus::hasPendingAfterEvents() const
{
    return m_afterEvents->pendingCount() > 0;
}

} // namespace mc::mod::bedrock::addon
