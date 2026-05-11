#pragma once

#include "../CriterionTrigger.hpp"
#include "conditions/ItemPredicate.hpp"
#include <memory>

namespace mc::advancement {

/**
 * @brief 消耗物品触发器
 *
 * 当玩家消耗物品（如吃食物、喝药水）时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.ConsumeItemTrigger
 */
class ConsumeItemTrigger : public AbstractCriterionTrigger<ConsumeItemTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:consume_item";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(ItemPredicate item);

        [[nodiscard]] bool test(const class ItemStack& item) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ItemPredicate m_item;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item);

    static std::shared_ptr<Instance> item(const ItemPredicate& item);
};

/**
 * @brief 物品耐久变化触发器
 *
 * 当物品耐久度变化时触发。
 */
class ItemDurabilityTrigger : public AbstractCriterionTrigger<ItemDurabilityTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:item_durability_changed";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(ItemPredicate item, IntBounds durability, IntBounds delta);

        [[nodiscard]] bool test(const class ItemStack& item, i32 oldDurability) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ItemPredicate m_item;
        IntBounds m_durability;
        IntBounds m_delta;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 oldDurability);
};

/**
 * @brief 附魔物品触发器
 *
 * 当玩家附魔物品时触发。
 */
class EnchantedItemTrigger : public AbstractCriterionTrigger<EnchantedItemTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enchanted_item";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(ItemPredicate item, IntBounds levels);

        [[nodiscard]] bool test(const class ItemStack& item, i32 levels) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ItemPredicate m_item;
        IntBounds m_levels;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 levels);
};

/**
 * @brief 填充桶触发器
 *
 * 当玩家用桶装液体时触发。
 */
class FilledBucketTrigger : public AbstractCriterionTrigger<FilledBucketTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:filled_bucket";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        explicit Instance(ItemPredicate item);

        [[nodiscard]] bool test(const class ItemStack& item) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        ItemPredicate m_item;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item);
};

} // namespace mc::advancement
