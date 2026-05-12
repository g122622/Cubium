#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/EntityPredicate.hpp"
#include <memory>

namespace mc::advancement {

// 前向声明 Instance 类
class TameAnimalTriggerInstance;
class BredAnimalsTriggerInstance;
class SummonedEntityTriggerInstance;
class CuredZombieVillagerTriggerInstance;
class VillagerTradeTriggerInstance;

/**
 * @brief 驯服动物触发器
 *
 * 当玩家驯服动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TameAnimalTrigger
 */
class TameAnimalTrigger : public AbstractCriterionTrigger<TameAnimalTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<TameAnimalTriggerInstance>> fromJson(const nlohmann::json& json);

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

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<BredAnimalsTriggerInstance>> fromJson(const nlohmann::json& json);

    void trigger(
        class ServerPlayer& player,
        const class Entity& child,
        const class Entity& parent,
        const class Entity& partner
    );
};

/**
 * @brief 动物繁殖触发器实例
 */
class BredAnimalsTriggerInstance : public CriterionInstance<BredAnimalsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    BredAnimalsTriggerInstance() = default;
    BredAnimalsTriggerInstance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner);

    [[nodiscard]] bool test(
        const class Entity& child,
        const class Entity& parent,
        const class Entity& partner
    ) const;

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

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<SummonedEntityTriggerInstance>> fromJson(const nlohmann::json& json);

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

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<CuredZombieVillagerTriggerInstance>> fromJson(const nlohmann::json& json);

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

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<VillagerTradeTriggerInstance>> fromJson(const nlohmann::json& json);

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

    [[nodiscard]] bool test(
        const class Entity& villager,
        const ItemStack& item
    ) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_villager;
    ItemPredicate m_item;
};

} // namespace mc::advancement
