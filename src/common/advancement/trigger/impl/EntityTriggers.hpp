#pragma once

#include "../CriterionTrigger.hpp"
#include "conditions/EntityPredicate.hpp"
#include <memory>

namespace mc::advancement {

/**
 * @brief 驯服动物触发器
 *
 * 当玩家驯服动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TameAnimalTrigger
 */
class TameAnimalTrigger : public AbstractCriterionTrigger<TameAnimalTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(EntityPredicate entity);

        [[nodiscard]] bool test(const class Entity& entity) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        EntityPredicate m_entity;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 动物繁殖触发器
 *
 * 当玩家繁殖动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.BredAnimalsTrigger
 */
class BredAnimalsTrigger : public AbstractCriterionTrigger<BredAnimalsTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner);

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

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(
        class ServerPlayer& player,
        const class Entity& child,
        const class Entity& parent,
        const class Entity& partner
    );
};

/**
 * @brief 召唤实体触发器
 *
 * 当玩家召唤实体时触发（如铁傀儡、雪傀儡）。
 */
class SummonedEntityTrigger : public AbstractCriterionTrigger<SummonedEntityTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:summoned_entity";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(EntityPredicate entity);

        [[nodiscard]] bool test(const class Entity& entity) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        EntityPredicate m_entity;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 治愈僵尸村民触发器
 *
 * 当玩家治愈僵尸村民时触发。
 */
class CuredZombieVillagerTrigger : public AbstractCriterionTrigger<CuredZombieVillagerTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:cured_zombie_villager";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(EntityPredicate zombie, EntityPredicate villager);

        [[nodiscard]] bool test(const class Entity& zombie, const class Entity& villager) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        EntityPredicate m_zombie;
        EntityPredicate m_villager;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class Entity& zombie, const class Entity& villager);
};

/**
 * @brief 村民交易触发器
 *
 * 当玩家与村民交易时触发。
 */
class VillagerTradeTrigger : public AbstractCriterionTrigger<VillagerTradeTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:villager_trade";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(EntityPredicate villager, ItemPredicate item);

        [[nodiscard]] bool test(
            const class Entity& villager,
            const class ItemStack& item
        ) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        EntityPredicate m_villager;
        ItemPredicate m_item;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class Entity& villager, const class ItemStack& item);
};

} // namespace mc::advancement
