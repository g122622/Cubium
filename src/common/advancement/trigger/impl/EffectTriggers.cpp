#include "EffectTriggers.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== TickTrigger ==========

Result<void> TickTrigger::Instance::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return {};
}

nlohmann::json TickTrigger::Instance::conditionsToJson() const {
    return nullptr;
}

Result<std::shared_ptr<TickTrigger::Instance>> TickTrigger::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return std::make_shared<Instance>();
}

void TickTrigger::trigger(ServerPlayer& player) {
    for (const auto& listener : getListeners(*player.getAdvancements())) {
        if (listener.getInstance().test()) {
            listener.grantCriterion(*player.getAdvancements());
        }
    }
}

// ========== RecipeUnlockedTrigger ==========

RecipeUnlockedTrigger::Instance::Instance(ResourceLocation recipe)
    : m_recipe(std::move(recipe)) {
}

bool RecipeUnlockedTrigger::Instance::test(const ResourceLocation& recipe) const {
    return m_recipe == recipe;
}

Result<void> RecipeUnlockedTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("recipe")) {
        m_recipe = ResourceLocation(json["recipe"].get<std::string>());
    }

    return {};
}

nlohmann::json RecipeUnlockedTrigger::Instance::conditionsToJson() const {
    return {{"recipe", m_recipe.toString()}};
}

Result<std::shared_ptr<RecipeUnlockedTrigger::Instance>> RecipeUnlockedTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void RecipeUnlockedTrigger::trigger(ServerPlayer& player, const ResourceLocation& recipe) {
    // [阶段2+3：事件系统集成] 由 RecipeUnlockedEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(recipe);
}

// ========== EffectsChangedTrigger ==========

EffectsChangedTrigger::Instance::Instance(MobEffectsPredicate effects)
    : m_effects(std::move(effects)) {
}

bool EffectsChangedTrigger::Instance::test(const Entity& entity) const {
    return m_effects.test(entity);
}

Result<void> EffectsChangedTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("effects")) {
        auto result = MobEffectsPredicate::fromJson(json["effects"]);
        if (result.failed()) {
            return result.error();
        }
        m_effects = result.value();
    }

    return {};
}

nlohmann::json EffectsChangedTrigger::Instance::conditionsToJson() const {
    if (!m_effects.isAny()) {
        return {{"effects", m_effects.toJson()}};
    }
    return nullptr;
}

Result<std::shared_ptr<EffectsChangedTrigger::Instance>> EffectsChangedTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EffectsChangedTrigger::trigger(ServerPlayer& player) {
    // [阶段2+3：事件系统集成] 由 EffectChangedEvent 触发
    MC_UNUSED(player);
}

// ========== BrewedPotionTrigger ==========

BrewedPotionTrigger::Instance::Instance(ResourceLocation potion)
    : m_potion(std::move(potion)) {
}

bool BrewedPotionTrigger::Instance::test(const ResourceLocation& potion) const {
    return m_potion == potion;
}

Result<void> BrewedPotionTrigger::Instance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("potion")) {
        m_potion = ResourceLocation(json["potion"].get<std::string>());
    }

    return {};
}

nlohmann::json BrewedPotionTrigger::Instance::conditionsToJson() const {
    return {{"potion", m_potion.toString()}};
}

Result<std::shared_ptr<BrewedPotionTrigger::Instance>> BrewedPotionTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<Instance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void BrewedPotionTrigger::trigger(ServerPlayer& player, const ResourceLocation& potion) {
    // [阶段2+3：事件系统集成] 由 BrewedPotionEvent 触发
    MC_UNUSED(player);
    MC_UNUSED(potion);
}

} // namespace mc::advancement
