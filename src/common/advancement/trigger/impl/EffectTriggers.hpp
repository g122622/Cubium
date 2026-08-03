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

#pragma once

#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/MobEffectsPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// Forward declarations
class RecipeUnlockedTriggerInstance;
class EffectsChangedTriggerInstance;
class BrewedPotionTriggerInstance;

/**
 * @brief 配方解锁触发器
 *
 * 当玩家解锁配方时触发。
 */
class RecipeUnlockedTrigger : public AbstractCriterionTrigger<RecipeUnlockedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:recipe_unlocked";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    // trigger() 方法需要在 server 层通过包含 TriggerInstantiation.hpp 来实现
};

/**
 * @brief 配方解锁触发器实例
 */
class RecipeUnlockedTriggerInstance : public CriterionInstance<RecipeUnlockedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:recipe_unlocked";
    RecipeUnlockedTriggerInstance() = default;
    explicit RecipeUnlockedTriggerInstance(ResourceLocation recipe);

    [[nodiscard]] bool test(const ResourceLocation& recipe) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ResourceLocation m_recipe;
};

/**
 * @brief 效果变化触发器
 *
 * 当玩家获得/失去效果时触发。
 */
class EffectsChangedTrigger : public AbstractCriterionTrigger<EffectsChangedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:effects_changed";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    // trigger() 方法需要在 server 层通过包含 TriggerInstantiation.hpp 来实现
};

/**
 * @brief 效果变化触发器实例
 */
class EffectsChangedTriggerInstance : public CriterionInstance<EffectsChangedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:effects_changed";
    EffectsChangedTriggerInstance() = default;

    /**
     * @brief 构造带效果谓词的实例
     * @param effects 效果谓词
     */
    explicit EffectsChangedTriggerInstance(MobEffectsPredicate effects);

    [[nodiscard]] bool test(const class Entity& entity) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    // ========== Getters ==========

    [[nodiscard]] const MobEffectsPredicate& getEffects() const noexcept { return m_effects; }

private:
    MobEffectsPredicate m_effects; ///< 效果谓词
};

/**
 * @brief 酿造药水触发器
 *
 * 当玩家酿造药水时触发。
 */
class BrewedPotionTrigger : public AbstractCriterionTrigger<BrewedPotionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:brewed_potion";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    // trigger() 方法需要在 server 层通过包含 TriggerInstantiation.hpp 来实现
};

/**
 * @brief 酿造药水触发器实例
 */
class BrewedPotionTriggerInstance : public CriterionInstance<BrewedPotionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:brewed_potion";
    BrewedPotionTriggerInstance() = default;
    explicit BrewedPotionTriggerInstance(ResourceLocation potion);

    [[nodiscard]] bool test(const ResourceLocation& potion) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ResourceLocation m_potion;
};

} // namespace mc::advancement
