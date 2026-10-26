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

/**
 * @file GenericTriggers.hpp
 * @brief 与具体玩法逻辑无关的触发器实现（条件按 vanilla Codec 字段逐一解码）
 *
 * 这里的每个实例都只承载一组简单的谓词/标量条件，触发判断本身由对应玩法在合适时机
 * 调用。条件 JSON 的字段布局严格对齐 vanilla 1.21.11 的 Codec——缺省字段取默认值，
 * 未知字段被忽略，与原版解码行为一致。
 */

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class ItemStack;
} // namespace mc

namespace mc::advancement {

class ItemPredicate;

/**
 * @brief 「只带玩家上下文谓词」的触发器实例
 *
 * 对齐 vanilla 中字段只有 `player: ContextAwarePredicate` 的那一类触发器
 * （tick / impossible / slept_in_bed / hero_of_the_village / voluntary_exile /
 * kill_mob_near_sculk_catalyst……）。条件 JSON 里若出现未声明的字段，vanilla 会忽略，
 * 这里同样忽略。
 */
class PlayerContextTriggerInstance : public CriterionInstance<PlayerContextTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_context";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 玩家上下文是否匹配（无 player 条件时恒真）
    [[nodiscard]] bool test(const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
};

/**
 * @brief 建造信标触发器实例
 *
 * 条件：`{"level": MinMaxBounds.Ints, "player": ContextAwarePredicate}`。
 */
class ConstructBeaconTriggerInstance : public CriterionInstance<ConstructBeaconTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:construct_beacon";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 信标层数是否落在条件范围内，且玩家上下文匹配
    [[nodiscard]] bool test(i32 level, const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
    IntBounds m_level;
};

class ConstructBeaconTrigger : public AbstractCriterionTrigger<ConstructBeaconTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:construct_beacon";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 射击弩触发器实例
 *
 * 条件：`{"item": ItemPredicate, "player": ContextAwarePredicate}`。
 */
class ShotCrossbowTriggerInstance : public CriterionInstance<ShotCrossbowTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:shot_crossbow";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 射出的弩是否匹配物品谓词，且玩家上下文匹配
    [[nodiscard]] bool test(const mc::ItemStack& crossbow, const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
    std::optional<ItemPredicate> m_item;
};

class ShotCrossbowTrigger : public AbstractCriterionTrigger<ShotCrossbowTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:shot_crossbow";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 长矛（三叉戟突刺）击杀数触发器实例
 *
 * 条件：`{"count": MinMaxBounds.Ints, "player": ContextAwarePredicate}`。
 */
class SpearMobsTriggerInstance : public CriterionInstance<SpearMobsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:spear_mobs";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 累计击杀数是否落在范围内，且玩家上下文匹配
    [[nodiscard]] bool test(i32 count, const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
    IntBounds m_count;
};

class SpearMobsTrigger : public AbstractCriterionTrigger<SpearMobsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:spear_mobs";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 命中标靶触发器实例
 *
 * 条件：`{"projectile": ContextAwarePredicate, "signal_strength": MinMaxBounds.Ints,
 * "player": ContextAwarePredicate}`。
 */
class TargetHitTriggerInstance : public CriterionInstance<TargetHitTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:target_hit";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 信号强度是否落在范围内，且投射物/玩家上下文匹配
    [[nodiscard]] bool test(i32 signalStrength, const CriterionContext& player, const CriterionContext& projectile) const;

    /// 投射物上下文（供事件侧判定 projectile 子条件）
    [[nodiscard]] const ContextAwarePredicate& projectile() const noexcept
    {
        return m_projectile;
    }

private:
    ContextAwarePredicate m_player;
    ContextAwarePredicate m_projectile;
    IntBounds m_signalStrength;
};

class TargetHitTrigger : public AbstractCriterionTrigger<TargetHitTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:target_hit";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 悦灵送物品到方块触发器实例
 *
 * 条件：`{"item": ItemPredicate, "location": ContextAwarePredicate, "player": ContextAwarePredicate}`。
 */
class AllayDropItemOnBlockTriggerInstance : public CriterionInstance<AllayDropItemOnBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:allay_drop_item_on_block";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 落点上下文与物品是否同时满足条件，且玩家上下文匹配
    [[nodiscard]] bool test(const mc::ItemStack& dropped, const CriterionContext& location, const CriterionContext& player) const;

private:
    ContextAwarePredicate m_player;
    ContextAwarePredicate m_location;
    std::optional<ItemPredicate> m_item;
};

class AllayDropItemOnBlockTrigger : public AbstractCriterionTrigger<AllayDropItemOnBlockTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:allay_drop_item_on_block";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 合成器合成触发器实例
 *
 * 条件：`{"recipe_id": ResourceLocation, "player": ContextAwarePredicate}`。
 * 与 `minecraft:recipe_crafted` 的区别：后者由玩家工作台合成触发，前者由合成器方块触发。
 */
class CrafterRecipeCraftedTriggerInstance : public CriterionInstance<CrafterRecipeCraftedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:crafter_recipe_crafted";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    /// 配方 ID 是否匹配，且玩家上下文匹配
    [[nodiscard]] bool test(const ResourceLocation& recipeId, const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
    std::optional<ResourceLocation> m_recipeId;
};

class CrafterRecipeCraftedTrigger : public AbstractCriterionTrigger<CrafterRecipeCraftedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:crafter_recipe_crafted";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 生物击杀与幽匿催发体相邻触发器实例
 *
 * 条件：`{"player": ContextAwarePredicate}`。触发时机由事件侧判定（击杀点 8 格内存在
 * 幽匿催发体），实例只负责玩家上下文比对。
 */
class KillMobNearSculkCatalystTriggerInstance
    : public CriterionInstance<KillMobNearSculkCatalystTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:kill_mob_near_sculk_catalyst";

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    [[nodiscard]] bool test(const CriterionContext& context) const;

private:
    ContextAwarePredicate m_player;
};

class KillMobNearSculkCatalystTrigger : public AbstractCriterionTrigger<KillMobNearSculkCatalystTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:kill_mob_near_sculk_catalyst";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

} // namespace mc::advancement