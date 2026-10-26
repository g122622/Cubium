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

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// 前向声明 Instance 类
class ConsumeItemTriggerInstance;
class ItemDurabilityTriggerInstance;
class EnchantedItemTriggerInstance;
class FilledBucketTriggerInstance;
class ThrownItemPickedUpByEntityTriggerInstance;
class ThrownItemPickedUpByPlayerTriggerInstance;

/**
 * @brief 消耗物品触发器
 *
 * 当玩家消耗物品（如吃食物、喝药水）时触发。
 */
class ConsumeItemTrigger : public AbstractCriterionTrigger<ConsumeItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:consume_item";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item);

    static std::shared_ptr<ConsumeItemTriggerInstance> item(const ItemPredicate& item);
};

/**
 * @brief 消耗物品触发器实例
 */
class ConsumeItemTriggerInstance : public CriterionInstance<ConsumeItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:consume_item";

    ConsumeItemTriggerInstance() noexcept = default;
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

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 oldDurability);
};

/**
 * @brief 物品耐久变化触发器实例
 */
class ItemDurabilityTriggerInstance : public CriterionInstance<ItemDurabilityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:item_durability_changed";

    ItemDurabilityTriggerInstance() noexcept = default;
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

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item, i32 levels);
};

/**
 * @brief 附魔物品触发器实例
 */
class EnchantedItemTriggerInstance : public CriterionInstance<EnchantedItemTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:enchanted_item";

    EnchantedItemTriggerInstance() noexcept = default;
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

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item);
};

/**
 * @brief 填充桶触发器实例
 */
class FilledBucketTriggerInstance : public CriterionInstance<FilledBucketTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:filled_bucket";

    FilledBucketTriggerInstance() noexcept = default;
    explicit FilledBucketTriggerInstance(ItemPredicate item);

    [[nodiscard]] bool test(const class ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
};

/**
 * @brief 投掷物被实体拾取触发器（对齐 vanilla ThrownItemPickedUpByEntityTrigger）
 *
 * 玩家投掷的物品（owner 为玩家）被某个实体拾取时触发。
 * 条件：item（被拾取的物品）+ entity（拾取者，可选，实体谓词）。
 */
class ThrownItemPickedUpByEntityTrigger : public AbstractCriterionTrigger<ThrownItemPickedUpByEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:thrown_item_picked_up_by_entity";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;
};

/**
 * @brief 投掷物被实体拾取触发器实例（对齐 vanilla TriggerInstance）
 */
class ThrownItemPickedUpByEntityTriggerInstance : public CriterionInstance<ThrownItemPickedUpByEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:thrown_item_picked_up_by_entity";

    ThrownItemPickedUpByEntityTriggerInstance() noexcept = default;
    explicit ThrownItemPickedUpByEntityTriggerInstance(ItemPredicate item);

    /**
     * @brief 检查被拾取物品是否满足条件
     *
     * 仅校验 item 条件；拾取者条件由 test(item, picker) 校验。
     */
    [[nodiscard]] bool test(const ItemStack& item) const;

    /**
     * @brief 检查物品与拾取者是否同时满足条件
     *
     * 对齐 vanilla PickedUpItemTrigger.TriggerInstance#matches：item 条件不满足即失败；
     * entity 条件存在时还要求拾取者匹配该实体谓词，picker 为空视为不匹配。
     */
    [[nodiscard]] bool test(const ItemStack& item, const class Entity* picker) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    EntityPredicate m_entity;
};

/**
 * @brief 投掷物被玩家拾取触发器（对齐 vanilla PickedUpItemTrigger.THROWN_ITEM_PICKED_UP_BY_PLAYER）
 *
 * 玩家投掷的物品被另一个玩家拾取时触发。条件：item（被拾取的物品）+ entity（拾取者）。
 * 与 `thrown_item_picked_up_by_entity` 的区别是拾取者必须是玩家——事件派发时就已限定，
 * 这里的条件结构与前者完全一致。
 */
class ThrownItemPickedUpByPlayerTrigger
    : public AbstractCriterionTrigger<ThrownItemPickedUpByPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:thrown_item_picked_up_by_player";

    [[nodiscard]] ResourceLocation getId() const override
    {
        return ResourceLocation(TRIGGER_ID);
    }
};

/**
 * @brief 投掷物被玩家拾取触发器实例（条件结构同 ThrownItemPickedUpByEntityTriggerInstance）
 */
class ThrownItemPickedUpByPlayerTriggerInstance
    : public CriterionInstance<ThrownItemPickedUpByPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:thrown_item_picked_up_by_player";

    [[nodiscard]] bool test(const ItemStack& item, const class Entity* picker) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    EntityPredicate m_entity;
};

} // namespace mc::advancement
