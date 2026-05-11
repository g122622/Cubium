#include "AdvancementManager.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

AdvancementManager& AdvancementManager::instance() {
    static AdvancementManager instance;
    return instance;
}

AdvancementManager::AdvancementManager() {
    m_list.addListener(this);
}

bool AdvancementManager::registerAdvancement(Advancement::Ptr advancement) {
    return m_list.add(std::move(advancement));
}

bool AdvancementManager::removeAdvancement(const ResourceLocation& id) {
    return m_list.remove(id);
}

void AdvancementManager::clear() {
    m_list.clear();
}

Advancement::Ptr AdvancementManager::get(const ResourceLocation& id) const {
    return m_list.get(id);
}

bool AdvancementManager::contains(const ResourceLocation& id) const {
    return m_list.contains(id);
}

void AdvancementManager::reload() {
    m_list.clear();
    // 实际加载由 AdvancementLoader 完成
}

void AdvancementManager::onAdvancementAdded(Advancement::Ptr advancement) {
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就添加
}

void AdvancementManager::onAdvancementRemoved(Advancement::Ptr advancement) {
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就移除
}

void AdvancementManager::onAdvancementUpdated(Advancement::Ptr advancement) {
    MC_UNUSED(advancement);
    // 子类可以重写此方法来响应成就更新
}

} // namespace mc::advancement
