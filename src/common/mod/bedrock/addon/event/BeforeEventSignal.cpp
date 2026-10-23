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

#include "common/mod/bedrock/addon/event/BeforeEventSignal.hpp"
#include "common/core/Types.hpp"

#include <algorithm>
#include <any>
#include <cstddef>
#include <mutex>
#include <typeindex>
#include <utility>

namespace mc::mod::bedrock::addon {

ScriptEventHandler BeforeEventSignal::subscribe(std::type_index eventType, Callback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    u64 id = m_nextId++;
    m_handlers[eventType].push_back({id, std::move(callback)});

    return ScriptEventHandler{id, eventType};
}

bool BeforeEventSignal::unsubscribe(const ScriptEventHandler& handle)
{
    if (!handle.valid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(handle.eventType);
    if (it == m_handlers.end()) {
        return false;
    }

    auto& subs = it->second;
    auto subIt = std::find_if(subs.begin(), subs.end(), [&](const Subscription& sub) {
        return sub.id == handle.id;
    });
    if (subIt != subs.end()) {
        subs.erase(subIt);

        return true;
    }

    return false;
}

bool BeforeEventSignal::fire(std::type_index eventType, std::any& eventData)
{
    std::vector<Subscription> subs;
    {
        std::lock_guard lock(m_mutex);
        const auto it = m_handlers.find(eventType);
        if (it == m_handlers.end()) {
            return false;
        }

        subs = it->second;
    }

    bool cancelled = false;
    for (const auto& sub : subs) {
        cancelled = sub.callback(eventData) || cancelled;
    }

    return cancelled;
}

void BeforeEventSignal::clearHandlers(std::type_index eventType)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.erase(eventType);
}

void BeforeEventSignal::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.clear();
}

size_t BeforeEventSignal::handlerCount(std::type_index eventType) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_handlers.find(eventType);

    return it != m_handlers.end() ? it->second.size() : 0;
}

} // namespace mc::mod::bedrock::addon
