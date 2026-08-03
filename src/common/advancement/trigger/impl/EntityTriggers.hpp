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

#include "../CriterionTrigger.hpp"
#include "../conditions/EntityPredicate.hpp"
#include "../conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class ItemStack;

namespace advancement {

// 前向声明 Instance 类
class TameAnimalTriggerInstance;
class BredAnimalsTriggerInstance;
class SummonedEntityTriggerInstance;
class CuredZombieVillagerTriggerInstance;
class VillagerTradeTriggerInstance;
class PlayerInteractedWithEntityTriggerInstance;

/**
 * @brief 驯服动物触发器
 *
 * 当玩家驯服动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TameAnimalTrigger
 */
class TameAnimalTrigger : public AbstractCriterionTrigger<TameAnimalTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 驯服动物触发器实例
 */
class TameAnimalTriggerInstance : public CriterionInstance<TameAnimalTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    TameAnimalTriggerInstance() = default;
    explicit TameAnimalTriggerInstance(EntityPredicate entity);

    [[nodiscard]] bool test(const class Entity& entity) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
};

/**
 * @brief 动物繁殖触发器
 *
 * 当玩家繁殖动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.BredAnimalsTrigger
 */
class BredAnimalsTrigger : public AbstractCriterionTrigger<BredAnimalsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(
        class ServerPlayer& player, const class Entity& child, const class Entity& parent, const class Entity& partner);
};

/**
 * @brief 动物繁殖触发器实例
 */
class BredAnimalsTriggerInstance : public CriterionInstance<BredAnimalsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    BredAnimalsTriggerInstance() = default;
    BredAnimalsTriggerInstance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner);

    [[nodiscard]] bool test(const class Entity& child, const class Entity& parent, const class Entity& partner) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_child;
    EntityPredicate m_parent;
    EntityPredicate m_partner;
};

/**
 * @brief 召唤实体触发器
 *
 * 当玩家召唤实体时触发（如铁傀儡、雪傀儡）。
 */
class SummonedEntityTrigger : public AbstractCriterionTrigger<SummonedEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:summoned_entity";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 召唤实体触发器实例
 */
class SummonedEntityTriggerInstance : public CriterionInstance<SummonedEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:summoned_entity";

    SummonedEntityTriggerInstance() = default;
    explicit SummonedEntityTriggerInstance(EntityPredicate entity);

    [[nodiscard]] bool test(const class Entity& entity) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
};

/**
 * @brief 治愈僵尸村民触发器
 *
 * 当玩家治愈僵尸村民时触发。
 */
class CuredZombieVillagerTrigger : public AbstractCriterionTrigger<CuredZombieVillagerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:cured_zombie_villager";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& zombie, const class Entity& villager);
};

/**
 * @brief 治愈僵尸村民触发器实例
 */
class CuredZombieVillagerTriggerInstance : public CriterionInstance<CuredZombieVillagerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:cured_zombie_villager";

    CuredZombieVillagerTriggerInstance() = default;
    CuredZombieVillagerTriggerInstance(EntityPredicate zombie, EntityPredicate villager);

    [[nodiscard]] bool test(const class Entity& zombie, const class Entity& villager) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_zombie;
    EntityPredicate m_villager;
};

/**
 * @brief 村民交易触发器
 *
 * 当玩家与村民交易时触发。
 */
class VillagerTradeTrigger : public AbstractCriterionTrigger<VillagerTradeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:villager_trade";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& villager, const class ItemStack& item);
};

/**
 * @brief 村民交易触发器实例
 */
class VillagerTradeTriggerInstance : public CriterionInstance<VillagerTradeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:villager_trade";

    VillagerTradeTriggerInstance() = default;
    VillagerTradeTriggerInstance(EntityPredicate villager, ItemPredicate item);

    [[nodiscard]] bool test(const class Entity& villager, const ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_villager;
    ItemPredicate m_item;
};

/**
 * @brief 玩家与实体交互触发器
 *
 * 当玩家与实体交互时触发（如右键点击实体）。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.PlayerEntityInteractionTrigger
 */
class PlayerInteractedWithEntityTrigger : public AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_interacted_with_entity";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item, const class Entity& entity);
};

/**
 * @brief 玩家与实体交互触发器实例
 */
class PlayerInteractedWithEntityTriggerInstance : public CriterionInstance<PlayerInteractedWithEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_interacted_with_entity";

    PlayerInteractedWithEntityTriggerInstance() = default;
    PlayerInteractedWithEntityTriggerInstance(ItemPredicate item, EntityPredicate entity);

    [[nodiscard]] bool test(const class ItemStack& item, const class Entity& entity) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    EntityPredicate m_entity;
};

} // namespace advancement
} // namespace mc
