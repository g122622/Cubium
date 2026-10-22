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
class RecipeCraftedTriggerInstance;
class EffectsChangedTriggerInstance;
class BrewedPotionTriggerInstance;
class LootTableTriggerInstance;

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
 * @brief 配方制作触发器（对齐 vanilla RecipeCraftedTrigger）
 *
 * 玩家在合成台/工作台完成一次配方时触发，条件可限定 recipe_id 与 ingredients。
 */
class RecipeCraftedTrigger : public AbstractCriterionTrigger<RecipeCraftedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:recipe_crafted";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（本次合成的配方 id 与消耗的原料列表）
    void trigger(class ServerPlayer& player,
        const ResourceLocation& recipeId,
        const class std::vector<class ItemStack>& ingredients);
};

/**
 * @brief 配方制作触发器实例
 */
class RecipeCraftedTriggerInstance : public CriterionInstance<RecipeCraftedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:recipe_crafted";

    RecipeCraftedTriggerInstance() = default;

    /// 配方 ID 与原料共同匹配（原料按多重集合匹配，逐个消耗）
    [[nodiscard]] bool test(const ResourceLocation& recipeId, const std::vector<class ItemStack>& ingredients) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ResourceLocation m_recipeId;
    std::vector<advancement::ItemPredicate> m_ingredients;
};

/**
 * @brief 容器战利品生成触发器（对齐 vanilla LootTableTrigger）
 *
 * 玩家首次打开带战利品表的容器（箱子/木桶/发射器/运输矿车/可疑的沙子等）时，
 * 在真正填充物品之前触发，条件以 loot_table 指定战利品表资源位置。
 *
 * vanilla 对应条目 minecraft:player_generates_container_loot，数据包进度
 * story/loot_* 系列依赖它。
 */
class LootTableTrigger : public AbstractCriterionTrigger<LootTableTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_generates_container_loot";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（本次生成的战利品表资源位置）
    void trigger(class ServerPlayer& player, const ResourceLocation& lootTableId);
};

/**
 * @brief 容器战利品生成触发器实例
 */
class LootTableTriggerInstance : public CriterionInstance<LootTableTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_generates_container_loot";

    LootTableTriggerInstance() = default;
    explicit LootTableTriggerInstance(const ResourceLocation& lootTable);

    /// 战利品表资源位置一致即匹配（vanilla TriggerInstance.matches）
    [[nodiscard]] bool test(const ResourceLocation& lootTableId) const { return m_lootTable == lootTableId; }

    [[nodiscard]] const ResourceLocation& lootTable() const noexcept { return m_lootTable; }

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ResourceLocation m_lootTable;
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
