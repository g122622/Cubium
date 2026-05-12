#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/ItemPredicate.hpp"
#include <memory>

namespace mc::advancement {

// 前向声明 Instance 类
class ConsumeItemTriggerInstance;
class ItemDurabilityTriggerInstance;
class EnchantedItemTriggerInstance;
class FilledBucketTriggerInstance;

/**
 * @brief 消耗物品触发器
 *
 * 当玩家消耗物品（如吃食物、喝药水）时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.ConsumeItemTrigger
 */
class ConsumeItemTrigger : public AbstractCriterionTrigger<ConsumeItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:consume_item";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ConsumeItemTriggerInstance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item);

    static std::shared_ptr<ConsumeItemTriggerInstance> item(const ItemPredicate& item);
};

/**
 * @brief 消耗物品触发器实例
 */
class ConsumeItemTriggerInstance : public CriterionInstance<ConsumeItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:consume_item";

    ConsumeItemTriggerInstance() = default;
    explicit ConsumeItemTriggerInstance(ItemPredicate item);

    [[nodiscard]] bool test(const class ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
};

/**
 * @brief 物品耐久变化触发器
 *
 * 当物品耐久度变化时触发。
 */
class ItemDurabilityTrigger : public AbstractCriterionTrigger<ItemDurabilityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:item_durability_changed";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ItemDurabilityTriggerInstance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 oldDurability);
};

/**
 * @brief 物品耐久变化触发器实例
 */
class ItemDurabilityTriggerInstance : public CriterionInstance<ItemDurabilityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:item_durability_changed";

    ItemDurabilityTriggerInstance() = default;
    ItemDurabilityTriggerInstance(ItemPredicate item, IntBounds durability, IntBounds delta);

    [[nodiscard]] bool test(const class ItemStack& item, i32 oldDurability) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    IntBounds m_durability;
    IntBounds m_delta;
};

/**
 * @brief 附魔物品触发器
 *
 * 当玩家附魔物品时触发。
 */
class EnchantedItemTrigger : public AbstractCriterionTrigger<EnchantedItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enchanted_item";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<EnchantedItemTriggerInstance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 levels);
};

/**
 * @brief 附魔物品触发器实例
 */
class EnchantedItemTriggerInstance : public CriterionInstance<EnchantedItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enchanted_item";

    EnchantedItemTriggerInstance() = default;
    EnchantedItemTriggerInstance(ItemPredicate item, IntBounds levels);

    [[nodiscard]] bool test(const class ItemStack& item, i32 levels) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    IntBounds m_levels;
};

/**
 * @brief 填充桶触发器
 *
 * 当玩家用桶装液体时触发。
 */
class FilledBucketTrigger : public AbstractCriterionTrigger<FilledBucketTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:filled_bucket";

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<FilledBucketTriggerInstance>> fromJson(const nlohmann::json& json);

    void trigger(class ServerPlayer& player, const class ItemStack& item);
};

/**
 * @brief 填充桶触发器实例
 */
class FilledBucketTriggerInstance : public CriterionInstance<FilledBucketTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:filled_bucket";

    FilledBucketTriggerInstance() = default;
    explicit FilledBucketTriggerInstance(ItemPredicate item);

    [[nodiscard]] bool test(const class ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
};

} // namespace mc::advancement
