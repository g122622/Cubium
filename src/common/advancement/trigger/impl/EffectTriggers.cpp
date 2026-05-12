#include "EffectTriggers.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// ========== RecipeUnlockedTrigger ==========

RecipeUnlockedTriggerInstance::RecipeUnlockedTriggerInstance(ResourceLocation recipe)
    : m_recipe(std::move(recipe)) {
}

bool RecipeUnlockedTriggerInstance::test(const ResourceLocation& recipe) const {
    // 如果没有指定配方ID（path为空），则匹配任何配方（any() 行为）
    // 默认构造的 ResourceLocation 有 namespace="minecraft", path=""
    if (m_recipe.path().empty()) {
        return true;
    }
    return m_recipe == recipe;
}

Result<void> RecipeUnlockedTriggerInstance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("recipe")) {
        m_recipe = ResourceLocation(json["recipe"].get<std::string>());
    }

    return {};
}

nlohmann::json RecipeUnlockedTriggerInstance::conditionsToJson() const {
    return {{"recipe", m_recipe.toString()}};
}

Result<std::shared_ptr<ICriterionInstance>> RecipeUnlockedTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<RecipeUnlockedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: RecipeUnlockedTrigger::trigger() is implemented in server layer via include of TriggerInstantiation.hpp

// ========== EffectsChangedTrigger ==========

bool EffectsChangedTriggerInstance::test(const Entity& entity) const {
    MC_UNUSED(entity);
    // [TODO] 需要 MobEffectsPredicate 实现
    return true;
}

Result<void> EffectsChangedTriggerInstance::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    // [TODO] 需要 MobEffectsPredicate 实现
    return {};
}

nlohmann::json EffectsChangedTriggerInstance::conditionsToJson() const {
    // [TODO] 需要 MobEffectsPredicate 实现
    return nullptr;
}

Result<std::shared_ptr<ICriterionInstance>> EffectsChangedTrigger::fromJson(const nlohmann::json& json) {
    MC_UNUSED(json);
    return std::make_shared<EffectsChangedTriggerInstance>();
}

// Note: EffectsChangedTrigger::trigger() is implemented via TriggerInstantiation.hpp in server layer

// ========== BrewedPotionTrigger ==========

BrewedPotionTriggerInstance::BrewedPotionTriggerInstance(ResourceLocation potion)
    : m_potion(std::move(potion)) {
}

bool BrewedPotionTriggerInstance::test(const ResourceLocation& potion) const {
    return m_potion == potion;
}

Result<void> BrewedPotionTriggerInstance::fromJson(const nlohmann::json& json) {
    if (json.is_null()) {
        return {};
    }

    if (json.contains("potion")) {
        m_potion = ResourceLocation(json["potion"].get<std::string>());
    }

    return {};
}

nlohmann::json BrewedPotionTriggerInstance::conditionsToJson() const {
    return {{"potion", m_potion.toString()}};
}

Result<std::shared_ptr<ICriterionInstance>> BrewedPotionTrigger::fromJson(const nlohmann::json& json) {
    auto instance = std::make_shared<BrewedPotionTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: BrewedPotionTrigger::trigger() is implemented via TriggerInstantiation.hpp in server layer

} // namespace mc::advancement
