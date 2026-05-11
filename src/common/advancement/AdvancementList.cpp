#include "AdvancementList.hpp"

namespace mc::advancement {

bool AdvancementList::add(Advancement::Ptr advancement) {
    if (!advancement) {
        return false;
    }

    const auto& id = advancement->getId();
    if (m_advancements.find(id) != m_advancements.end()) {
        return false;  // 已存在
    }

    m_advancements[id] = advancement;

    // 尝试建立父子关系
    bool parentSet = trySetParent(advancement);

    if (!parentSet && advancement->getParent().has_value()) {
        // 父成就尚未加载，加入等待列表
        m_waitingForParent[advancement->getParent().value()].push_back(advancement);
    } else if (advancement->isRoot()) {
        m_roots.push_back(advancement);
    } else {
        m_nonRoots.push_back(advancement);
    }

    // 检查是否有其他成就在等待这个成就
    auto waitingIt = m_waitingForParent.find(id);
    if (waitingIt != m_waitingForParent.end()) {
        for (auto& child : waitingIt->second) {
            advancement->addChild(child);
            m_nonRoots.push_back(child);
        }
        m_waitingForParent.erase(waitingIt);
    }

    notifyAdvancementAdded(advancement);
    return true;
}

bool AdvancementList::remove(const ResourceLocation& id) {
    auto it = m_advancements.find(id);
    if (it == m_advancements.end()) {
        return false;
    }

    auto advancement = it->second;
    m_advancements.erase(it);

    // 从根节点列表移除
    auto rootIt = std::find(m_roots.begin(), m_roots.end(), advancement);
    if (rootIt != m_roots.end()) {
        m_roots.erase(rootIt);
    }

    // 从非根节点列表移除
    auto nonRootIt = std::find(m_nonRoots.begin(), m_nonRoots.end(), advancement);
    if (nonRootIt != m_nonRoots.end()) {
        m_nonRoots.erase(nonRootIt);
    }

    // 从父成就的子列表移除（通过重建关系）
    // 注意：当前实现中，子成就需要单独移除

    notifyAdvancementRemoved(advancement);
    return true;
}

void AdvancementList::clear() {
    m_advancements.clear();
    m_roots.clear();
    m_nonRoots.clear();
    m_waitingForParent.clear();
}

Advancement::Ptr AdvancementList::get(const ResourceLocation& id) const {
    auto it = m_advancements.find(id);
    return it != m_advancements.end() ? it->second : nullptr;
}

bool AdvancementList::contains(const ResourceLocation& id) const {
    return m_advancements.find(id) != m_advancements.end();
}

void AdvancementList::forEach(std::function<bool(Advancement::Ptr)> callback) const {
    for (const auto& [_, advancement] : m_advancements) {
        if (!callback(advancement)) {
            break;
        }
    }
}

void AdvancementList::forEachRoot(std::function<bool(Advancement::Ptr)> callback) const {
    for (const auto& root : m_roots) {
        if (!callback(root)) {
            break;
        }
    }
}

void AdvancementList::addListener(IListener* listener) {
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end()) {
        m_listeners.push_back(listener);
    }
}

void AdvancementList::removeListener(IListener* listener) {
    auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
    }
}

void AdvancementList::rebuildRelations() {
    // 清空当前分类
    m_roots.clear();
    m_nonRoots.clear();
    m_waitingForParent.clear();

    // 重新建立父子关系
    for (auto& [_, advancement] : m_advancements) {
        if (advancement->getParent().has_value()) {
            auto parent = get(advancement->getParent().value());
            if (parent) {
                parent->addChild(advancement);
                m_nonRoots.push_back(advancement);
            } else {
                m_waitingForParent[advancement->getParent().value()].push_back(advancement);
            }
        } else {
            m_roots.push_back(advancement);
        }
    }
}

bool AdvancementList::trySetParent(Advancement::Ptr advancement) {
    if (!advancement->getParent().has_value()) {
        return true;  // 根成就
    }

    auto parent = get(advancement->getParent().value());
    if (parent) {
        parent->addChild(advancement);
        return true;
    }
    return false;
}

void AdvancementList::notifyAdvancementAdded(Advancement::Ptr advancement) {
    for (auto* listener : m_listeners) {
        listener->onAdvancementAdded(advancement);
    }
}

void AdvancementList::notifyAdvancementRemoved(Advancement::Ptr advancement) {
    for (auto* listener : m_listeners) {
        listener->onAdvancementRemoved(advancement);
    }
}

void AdvancementList::notifyAdvancementUpdated(Advancement::Ptr advancement) {
    for (auto* listener : m_listeners) {
        listener->onAdvancementUpdated(advancement);
    }
}

} // namespace mc::advancement
