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

#include "EffectTriggers.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/advancement/trigger/conditions/MobEffectsPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== RecipeUnlockedTrigger ==========

RecipeUnlockedTriggerInstance::RecipeUnlockedTriggerInstance(ResourceLocation recipe)
    : m_recipe(std::move(recipe))
{}

bool RecipeUnlockedTriggerInstance::test(const ResourceLocation& recipe) const
{
    // 如果没有指定配方ID（path为空），则匹配任何配方（any() 行为）
    // 默认构造的 ResourceLocation 有 namespace="minecraft", path=""
    if (m_recipe.path().empty()) {
        return true;
    }
    return m_recipe == recipe;
}

Result<void> RecipeUnlockedTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("recipe")) {
        m_recipe = ResourceLocation(json["recipe"].get<std::string>());
    }

    return {};
}

nlohmann::json RecipeUnlockedTriggerInstance::conditionsToJson() const
{
    return {{"recipe", m_recipe.toString()}};
}

Result<std::shared_ptr<ICriterionInstance>> RecipeUnlockedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<RecipeUnlockedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: RecipeUnlockedTrigger::trigger() is implemented in server layer via include of TriggerInstantiation.hpp

// ========== RecipeCraftedTrigger ==========

bool RecipeCraftedTriggerInstance::test(
    const ResourceLocation& recipeId, const std::vector<ItemStack>& ingredients) const
{
    // 配方 ID 必须一致；空 ID 表示"任意配方"（对齐 vanilla 条件中 recipe_id 必填，这里放宽以兼容）。
    if (!m_recipeId.path().empty() && m_recipeId != recipeId) {
        return false;
    }

    // 原料按多重集合匹配：每个谓词须能消耗掉一个尚未匹配的原料（对齐 vanilla matches）。
    std::vector<const ItemStack*> remaining;
    remaining.reserve(ingredients.size());
    for (const auto& stack : ingredients) {
        if (!stack.isEmpty()) {
            remaining.push_back(&stack);
        }
    }
    for (const auto& predicate : m_ingredients) {
        bool matched = false;
        for (auto it = remaining.begin(); it != remaining.end(); ++it) {
            if (predicate.test(**it)) {
                remaining.erase(it);
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

Result<void> RecipeCraftedTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }
    if (json.contains("recipe_id")) {
        m_recipeId = ResourceLocation(json["recipe_id"].get<std::string>());
    }
    if (json.contains("ingredients") && json["ingredients"].is_array()) {
        for (const auto& entry : json["ingredients"]) {
            auto predicate = advancement::ItemPredicate::fromJson(entry);
            if (predicate.failed()) {
                return predicate.error();
            }
            m_ingredients.push_back(std::move(predicate.value()));
        }
    }
    return {};
}

nlohmann::json RecipeCraftedTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;
    if (!m_recipeId.path().empty()) {
        json["recipe_id"] = m_recipeId.toString();
    }
    if (!m_ingredients.empty()) {
        nlohmann::json ingredients = nlohmann::json::array();
        for (const auto& predicate : m_ingredients) {
            ingredients.push_back(predicate.toJson());
        }
        json["ingredients"] = std::move(ingredients);
    }
    return json;
}

void RecipeCraftedTrigger::trigger(
    ServerPlayer& player, const ResourceLocation& recipeId, const std::vector<ItemStack>& ingredients)
{
    // 实际触发逻辑在服务端（TriggerInstantiation.hpp 的模板实现），此接口供 ServerPlayer 调用。
    MC_UNUSED(player);
    MC_UNUSED(recipeId);
    MC_UNUSED(ingredients);
}

Result<std::shared_ptr<ICriterionInstance>> RecipeCraftedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<RecipeCraftedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// ========== EffectsChangedTrigger ==========

EffectsChangedTriggerInstance::EffectsChangedTriggerInstance(MobEffectsPredicate effects)
    : m_effects(std::move(effects))
{}

bool EffectsChangedTriggerInstance::test(const Entity& entity) const
{
    return m_effects.test(entity);
}

Result<void> EffectsChangedTriggerInstance::fromJson(const nlohmann::json& json)
{
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

nlohmann::json EffectsChangedTriggerInstance::conditionsToJson() const
{
    if (m_effects.isAny()) {
        return nullptr;
    }

    nlohmann::json json;
    json["effects"] = m_effects.toJson();
    return json;
}

Result<std::shared_ptr<ICriterionInstance>> EffectsChangedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EffectsChangedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: EffectsChangedTrigger::trigger() is implemented via TriggerInstantiation.hpp in server layer

// ========== BrewedPotionTrigger ==========

BrewedPotionTriggerInstance::BrewedPotionTriggerInstance(ResourceLocation potion)
    : m_potion(std::move(potion))
{}

bool BrewedPotionTriggerInstance::test(const ResourceLocation& potion) const
{
    return m_potion == potion;
}

Result<void> BrewedPotionTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("potion")) {
        m_potion = ResourceLocation(json["potion"].get<std::string>());
    }

    return {};
}

nlohmann::json BrewedPotionTriggerInstance::conditionsToJson() const
{
    return {{"potion", m_potion.toString()}};
}

Result<std::shared_ptr<ICriterionInstance>> BrewedPotionTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<BrewedPotionTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: BrewedPotionTrigger::trigger() is implemented via TriggerInstantiation.hpp in server layer

// ========== LootTableTrigger ==========

LootTableTriggerInstance::LootTableTriggerInstance(const ResourceLocation& lootTable)
    : m_lootTable(lootTable)
{}

Result<void> LootTableTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        // vanilla 中 loot_table 为必填字段；缺失时保留空 ID，永不匹配任何战利品表
        return {};
    }
    if (!json.contains("loot_table") || !json["loot_table"].is_string()) {
        return Error::invalidArgument("LootTableTrigger 条件缺少必填字段 loot_table");
    }
    m_lootTable = ResourceLocation(json["loot_table"].get<std::string>());
    return {};
}

nlohmann::json LootTableTriggerInstance::conditionsToJson() const
{
    return {{"loot_table", m_lootTable.toString()}};
}

Result<std::shared_ptr<ICriterionInstance>> LootTableTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<LootTableTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

// Note: LootTableTrigger::trigger() is implemented in PacketHandlers via
// AbstractCriterionTrigger<LootTableTriggerInstance>::trigger (see TriggerInstantiation.hpp)

} // namespace mc::advancement
