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
 */

#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

ItemComponentRegistry& ItemComponentRegistry::instance()
{
    static ItemComponentRegistry registry;
    return registry;
}

void ItemComponentRegistry::registerComponent(const std::string& itemTypeId, ItemCustomComponent component)
{
    if (component.name.find(':') == std::string::npos) {
        spdlog::warn(
            "ItemComponentRegistry: component name '{}' missing namespace prefix, recommended format 'namespace:name'", component.name);
    }

    std::unique_lock lock(m_mutex);
    m_components[itemTypeId].push_back(std::move(component));
    _updateCallbackFlags(itemTypeId);

    spdlog::info("ItemComponentRegistry: registered item component '{}' to '{}'", m_components[itemTypeId].back().name, itemTypeId);
}

size_t ItemComponentRegistry::unregisterComponent(const std::string& itemTypeId, const std::string& componentName)
{
    std::unique_lock lock(m_mutex);

    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return 0;
    }

    auto& components = it->second;
    auto originalSize = components.size();
    components.erase(std::remove_if(components.begin(),
                         components.end(),
                         [&componentName](const ItemCustomComponent& c) { return c.name == componentName; }),
        components.end());

    auto removed = originalSize - components.size();
    if (components.empty()) {
        m_components.erase(it);
        m_callbackFlags.erase(itemTypeId);
    } else {
        _updateCallbackFlags(itemTypeId);
    }

    return removed;
}

void ItemComponentRegistry::unregisterAll(const std::string& itemTypeId)
{
    std::unique_lock lock(m_mutex);
    m_components.erase(itemTypeId);
    m_callbackFlags.erase(itemTypeId);
}

// ===== 查询方法 =====

bool ItemComponentRegistry::hasUseCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasUse;
}

bool ItemComponentRegistry::hasUseOnCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasUseOn;
}

bool ItemComponentRegistry::hasHitEntityCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasHitEntity;
}

bool ItemComponentRegistry::hasMineBlockCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasMineBlock;
}

bool ItemComponentRegistry::hasBeforeDurabilityDamageCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasBeforeDurabilityDamage;
}

bool ItemComponentRegistry::hasCompleteUseCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasCompleteUse;
}

bool ItemComponentRegistry::hasConsumeCallback(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(itemTypeId);
    return it != m_callbackFlags.end() && it->second.hasConsume;
}

// ===== 派发方法 =====

bool ItemComponentRegistry::dispatchUse(const std::string& itemTypeId, ItemComponentUseEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onUse) {
            component.onUse(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchUseOn(const std::string& itemTypeId, ItemComponentUseOnEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onUseOn) {
            component.onUseOn(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchHitEntity(const std::string& itemTypeId, ItemComponentHitEntityEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onHitEntity) {
            component.onHitEntity(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchMineBlock(const std::string& itemTypeId, ItemComponentMineBlockEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onMineBlock) {
            component.onMineBlock(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchBeforeDurabilityDamage(
    const std::string& itemTypeId, ItemComponentBeforeDurabilityDamageEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onBeforeDurabilityDamage) {
            component.onBeforeDurabilityDamage(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchCompleteUse(const std::string& itemTypeId, ItemComponentCompleteUseEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onCompleteUse) {
            component.onCompleteUse(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool ItemComponentRegistry::dispatchConsume(const std::string& itemTypeId, ItemComponentConsumeEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onConsume) {
            component.onConsume(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

void ItemComponentRegistry::clear() noexcept
{
    std::unique_lock lock(m_mutex);
    m_components.clear();
    m_callbackFlags.clear();
}

size_t ItemComponentRegistry::registeredItemTypeCount() const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_components.size();
}

size_t ItemComponentRegistry::componentCount(const std::string& itemTypeId) const noexcept
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    return it != m_components.end() ? it->second.size() : 0;
}

void ItemComponentRegistry::_updateCallbackFlags(const std::string& itemTypeId)
{
    auto it = m_components.find(itemTypeId);
    if (it == m_components.end()) {
        m_callbackFlags.erase(itemTypeId);
        return;
    }

    CallbackFlags flags{};
    for (const auto& component : it->second) {
        if (component.onUse) flags.hasUse = 1;
        if (component.onUseOn) flags.hasUseOn = 1;
        if (component.onHitEntity) flags.hasHitEntity = 1;
        if (component.onMineBlock) flags.hasMineBlock = 1;
        if (component.onBeforeDurabilityDamage) flags.hasBeforeDurabilityDamage = 1;
        if (component.onCompleteUse) flags.hasCompleteUse = 1;
        if (component.onConsume) flags.hasConsume = 1;
    }
    m_callbackFlags[itemTypeId] = flags;
}

} // namespace mc::mod::bedrock::addon
