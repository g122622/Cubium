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

#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include <spdlog/spdlog.h>

namespace mc::mod::bedrock::addon {

BlockComponentRegistry& BlockComponentRegistry::instance()
{
    static BlockComponentRegistry registry;
    return registry;
}

void BlockComponentRegistry::registerComponent(const std::string& blockTypeId, BlockCustomComponent component)
{
    // 验证组件名称包含命名空间前缀
    if (component.name.find(':') == std::string::npos) {
        spdlog::warn(
            "BlockComponentRegistry: component name '{}' missing namespace prefix, recommended format 'namespace:name'", component.name);
    }

    std::unique_lock lock(m_mutex);
    m_components[blockTypeId].push_back(std::move(component));
    _updateCallbackFlags(blockTypeId);

    spdlog::info(
        "BlockComponentRegistry: registered block component '{}' to '{}'", m_components[blockTypeId].back().name, blockTypeId);
}

size_t BlockComponentRegistry::unregisterComponent(const std::string& blockTypeId, const std::string& componentName)
{
    std::unique_lock lock(m_mutex);

    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return 0;
    }

    auto& components = it->second;
    auto originalSize = components.size();
    components.erase(std::remove_if(components.begin(),
                         components.end(),
                         [&componentName](const BlockCustomComponent& c) { return c.name == componentName; }),
        components.end());

    auto removed = originalSize - components.size();
    if (components.empty()) {
        m_components.erase(it);
        m_callbackFlags.erase(blockTypeId);
    } else {
        _updateCallbackFlags(blockTypeId);
    }

    return removed;
}

void BlockComponentRegistry::unregisterAll(const std::string& blockTypeId)
{
    std::unique_lock lock(m_mutex);
    m_components.erase(blockTypeId);
    m_callbackFlags.erase(blockTypeId);
}

// ===== 查询方法 =====

bool BlockComponentRegistry::hasStepOnCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasStepOn;
}

bool BlockComponentRegistry::hasStepOffCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasStepOff;
}

bool BlockComponentRegistry::hasPlaceCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasPlace;
}

bool BlockComponentRegistry::hasBreakCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasBreak;
}

bool BlockComponentRegistry::hasPlayerBreakCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasPlayerBreak;
}

bool BlockComponentRegistry::hasPlayerInteractCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasPlayerInteract;
}

bool BlockComponentRegistry::hasPlayerPlaceBeforeCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasPlayerPlaceBefore;
}

bool BlockComponentRegistry::hasEntityFallOnCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasEntityFallOn;
}

bool BlockComponentRegistry::hasRandomTickCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasRandomTick;
}

bool BlockComponentRegistry::hasTickCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasTick;
}

bool BlockComponentRegistry::hasRedstoneUpdateCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasRedstoneUpdate;
}

bool BlockComponentRegistry::hasEntityCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasEntity;
}

bool BlockComponentRegistry::hasBlockStateChangeCallback(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_callbackFlags.find(blockTypeId);
    return it != m_callbackFlags.end() && it->second.hasBlockStateChange;
}

// ===== 派发方法 =====

bool BlockComponentRegistry::dispatchStepOn(const std::string& blockTypeId, BlockComponentStepOnEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onStepOn) {
            component.onStepOn(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchStepOff(const std::string& blockTypeId, BlockComponentStepOffEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onStepOff) {
            component.onStepOff(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchPlace(const std::string& blockTypeId, BlockComponentOnPlaceEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onPlace) {
            component.onPlace(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchBreak(const std::string& blockTypeId, BlockComponentBreakEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onBreak) {
            component.onBreak(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchPlayerBreak(const std::string& blockTypeId, BlockComponentPlayerBreakEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onPlayerBreak) {
            component.onPlayerBreak(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchPlayerInteract(
    const std::string& blockTypeId, BlockComponentPlayerInteractEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onPlayerInteract) {
            component.onPlayerInteract(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchPlayerPlaceBefore(
    const std::string& blockTypeId, BlockComponentPlayerPlaceBeforeEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.beforeOnPlayerPlace) {
            component.beforeOnPlayerPlace(event, component.parameters);
            dispatched = true;
            // beforeEvent：所有回调都执行，但cancel状态传播
        }
    }
    return dispatched && event.cancel;
}

bool BlockComponentRegistry::dispatchEntityFallOn(
    const std::string& blockTypeId, BlockComponentEntityFallOnEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onEntityFallOn) {
            component.onEntityFallOn(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchRandomTick(const std::string& blockTypeId, BlockComponentRandomTickEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onRandomTick) {
            component.onRandomTick(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchTick(const std::string& blockTypeId, BlockComponentTickEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onTick) {
            component.onTick(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchRedstoneUpdate(
    const std::string& blockTypeId, BlockComponentRedstoneUpdateEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onRedstoneUpdate) {
            component.onRedstoneUpdate(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchEntity(const std::string& blockTypeId, BlockComponentEntityEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onEntity) {
            component.onEntity(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

bool BlockComponentRegistry::dispatchBlockStateChange(
    const std::string& blockTypeId, BlockComponentBlockStateChangeEvent& event)
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        return false;
    }

    bool dispatched = false;
    for (auto& component : it->second) {
        if (component.onBlockStateChange) {
            component.onBlockStateChange(event, component.parameters);
            dispatched = true;
        }
    }
    return dispatched;
}

void BlockComponentRegistry::clear()
{
    std::unique_lock lock(m_mutex);
    m_components.clear();
    m_callbackFlags.clear();
}

size_t BlockComponentRegistry::registeredBlockTypeCount() const
{
    std::shared_lock lock(m_mutex);
    return m_components.size();
}

size_t BlockComponentRegistry::componentCount(const std::string& blockTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(blockTypeId);
    return it != m_components.end() ? it->second.size() : 0;
}

void BlockComponentRegistry::_updateCallbackFlags(const std::string& blockTypeId)
{
    auto it = m_components.find(blockTypeId);
    if (it == m_components.end()) {
        m_callbackFlags.erase(blockTypeId);
        return;
    }

    CallbackFlags flags{};
    for (const auto& component : it->second) {
        if (component.onStepOn) flags.hasStepOn = 1;
        if (component.onStepOff) flags.hasStepOff = 1;
        if (component.onPlace) flags.hasPlace = 1;
        if (component.onBreak) flags.hasBreak = 1;
        if (component.onPlayerBreak) flags.hasPlayerBreak = 1;
        if (component.onPlayerInteract) flags.hasPlayerInteract = 1;
        if (component.beforeOnPlayerPlace) flags.hasPlayerPlaceBefore = 1;
        if (component.onEntityFallOn) flags.hasEntityFallOn = 1;
        if (component.onRandomTick) flags.hasRandomTick = 1;
        if (component.onTick) flags.hasTick = 1;
        if (component.onRedstoneUpdate) flags.hasRedstoneUpdate = 1;
        if (component.onEntity) flags.hasEntity = 1;
        if (component.onBlockStateChange) flags.hasBlockStateChange = 1;
    }
    m_callbackFlags[blockTypeId] = flags;
}

} // namespace mc::mod::bedrock::addon
