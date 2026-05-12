#include "CriterionTriggers.hpp"
#include "impl/ImpossibleTrigger.hpp"
#include "impl/InventoryChangedTrigger.hpp"
#include "impl/TickTrigger.hpp"
#include "impl/EffectTriggers.hpp"
#include <spdlog/spdlog.h>

namespace mc::advancement {

CriterionTriggers& CriterionTriggers::instance() {
    static CriterionTriggers instance;
    return instance;
}

void CriterionTriggers::registerTrigger(std::unique_ptr<ICriterionTriggerBase> trigger) {
    if (!trigger) {
        return;
    }

    ResourceLocation id = trigger->getId();
    if (m_triggers.find(id) != m_triggers.end()) {
        spdlog::warn("Trigger already registered, replacing: {}", id.toString());
    }

    m_triggers[id] = std::move(trigger);
}

ICriterionTriggerBase* CriterionTriggers::getTrigger(const ResourceLocation& id) {
    auto it = m_triggers.find(id);
    return it != m_triggers.end() ? it->second.get() : nullptr;
}

bool CriterionTriggers::hasTrigger(const ResourceLocation& id) const {
    return m_triggers.find(id) != m_triggers.end();
}

std::vector<ResourceLocation> CriterionTriggers::getAllTriggerIds() const {
    std::vector<ResourceLocation> ids;
    ids.reserve(m_triggers.size());
    for (const auto& [id, _] : m_triggers) {
        ids.push_back(id);
    }
    return ids;
}

void CriterionTriggers::clear() {
    m_triggers.clear();
}

void CriterionTriggers::registerBuiltinTriggers() {
    // 注册基础触发器
    registerTrigger(std::make_unique<ImpossibleTrigger>());
    registerTrigger(std::make_unique<InventoryChangedTrigger>());
    registerTrigger(std::make_unique<TickTrigger>());
    registerTrigger(std::make_unique<RecipeUnlockedTrigger>());
    registerTrigger(std::make_unique<EffectsChangedTrigger>());
    registerTrigger(std::make_unique<BrewedPotionTrigger>());

    // [TODO 阶段3+4：触发器完善] 注册更多触发器
    // registerTrigger(std::make_unique<LocationTrigger>());
    // registerTrigger(std::make_unique<PlayerKilledEntityTrigger>());
    // 等等...

    spdlog::info("Registered {} builtin triggers", m_triggers.size());
}

} // namespace mc::advancement
