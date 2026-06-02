#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace mc::mod::bedrock::addon {

ItemComponentRegistry& ItemComponentRegistry::instance()
{
    static ItemComponentRegistry registry;
    return registry;
}

void ItemComponentRegistry::registerComponent(const std::string& itemTypeId, ItemCustomComponent component)
{
    if (component.name.find(':') == std::string::npos) {
        spdlog::warn("ItemComponentRegistry: 组件名称'{}'缺少命名空间前缀，建议使用'namespace:name'格式",
                      component.name);
    }

    std::unique_lock lock(m_mutex);
    m_components[itemTypeId].push_back(std::move(component));
    updateCallbackFlags(itemTypeId);

    spdlog::info("ItemComponentRegistry: 已注册物品组件'{}'到'{}'",
                 m_components[itemTypeId].back().name, itemTypeId);
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
    components.erase(
        std::remove_if(components.begin(), components.end(),
            [&componentName](const ItemCustomComponent& c) { return c.name == componentName; }),
        components.end());

    auto removed = originalSize - components.size();
    if (components.empty()) {
        m_components.erase(it);
        m_callbackFlags.erase(itemTypeId);
    } else {
        updateCallbackFlags(itemTypeId);
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

bool ItemComponentRegistry::dispatchBeforeDurabilityDamage(const std::string& itemTypeId, ItemComponentBeforeDurabilityDamageEvent& event)
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

void ItemComponentRegistry::clear()
{
    std::unique_lock lock(m_mutex);
    m_components.clear();
    m_callbackFlags.clear();
}

size_t ItemComponentRegistry::registeredItemTypeCount() const
{
    std::shared_lock lock(m_mutex);
    return m_components.size();
}

size_t ItemComponentRegistry::componentCount(const std::string& itemTypeId) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_components.find(itemTypeId);
    return it != m_components.end() ? it->second.size() : 0;
}

void ItemComponentRegistry::updateCallbackFlags(const std::string& itemTypeId)
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
