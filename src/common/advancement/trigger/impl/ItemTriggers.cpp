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

#include "ItemTriggers.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

// ========== ConsumeItemTriggerInstance ==========

ConsumeItemTriggerInstance::ConsumeItemTriggerInstance(ItemPredicate item)
    : m_item(std::move(item))
{}

bool ConsumeItemTriggerInstance::test(const ItemStack& item) const
{
    return m_item.test(item);
}

Result<void> ConsumeItemTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json ConsumeItemTriggerInstance::conditionsToJson() const
{
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

// ========== ConsumeItemTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> ConsumeItemTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ConsumeItemTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ConsumeItemTrigger::trigger(ServerPlayer& player, const ItemStack& item)
{
    // 触发器通过 AdvancementEventHandler::onConsumeItem() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(item);
}

std::shared_ptr<ConsumeItemTriggerInstance> ConsumeItemTrigger::item(const ItemPredicate& item)
{
    return std::make_shared<ConsumeItemTriggerInstance>(item);
}

// ========== ItemDurabilityTriggerInstance ==========

ItemDurabilityTriggerInstance::ItemDurabilityTriggerInstance(ItemPredicate item, IntBounds durability, IntBounds delta)
    : m_item(std::move(item))
    , m_durability(std::move(durability))
    , m_delta(std::move(delta))
{}

bool ItemDurabilityTriggerInstance::test(const ItemStack& item, i32 oldDurability) const
{
    if (!m_item.test(item)) {
        return false;
    }

    const i32 newDurability = item.getMaxDamage() - item.getDamage();
    const i32 delta = oldDurability - newDurability;
    return m_durability.test(newDurability) && m_delta.test(delta);
}

Result<void> ItemDurabilityTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("durability")) {
        m_durability = IntBounds::fromJson(json["durability"]);
    }

    if (json.contains("delta")) {
        m_delta = IntBounds::fromJson(json["delta"]);
    }

    return {};
}

nlohmann::json ItemDurabilityTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_durability.isUnbounded()) {
        json["durability"] = m_durability.toJson();
    }
    if (!m_delta.isUnbounded()) {
        json["delta"] = m_delta.toJson();
    }

    return json;
}

// ========== ItemDurabilityTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> ItemDurabilityTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<ItemDurabilityTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void ItemDurabilityTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 oldDurability)
{
    // 触发器通过 AdvancementEventHandler::onItemDurability() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(oldDurability);
}

// ========== EnchantedItemTriggerInstance ==========

EnchantedItemTriggerInstance::EnchantedItemTriggerInstance(ItemPredicate item, IntBounds levels)
    : m_item(std::move(item))
    , m_levels(std::move(levels))
{}

bool EnchantedItemTriggerInstance::test(const ItemStack& item, i32 levels) const
{
    return m_item.test(item) && m_levels.test(levels);
}

Result<void> EnchantedItemTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    if (json.contains("levels")) {
        m_levels = IntBounds::fromJson(json["levels"]);
    }

    return {};
}

nlohmann::json EnchantedItemTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_item.isAny()) {
        json["item"] = m_item.toJson();
    }
    if (!m_levels.isUnbounded()) {
        json["levels"] = m_levels.toJson();
    }

    return json;
}

// ========== EnchantedItemTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> EnchantedItemTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<EnchantedItemTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void EnchantedItemTrigger::trigger(ServerPlayer& player, const ItemStack& item, i32 levels)
{
    // 触发器通过 AdvancementEventHandler::onEnchantItem() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(item);
    MC_UNUSED(levels);
}

// ========== FilledBucketTriggerInstance ==========

FilledBucketTriggerInstance::FilledBucketTriggerInstance(ItemPredicate item)
    : m_item(std::move(item))
{}

bool FilledBucketTriggerInstance::test(const ItemStack& item) const
{
    return m_item.test(item);
}

Result<void> FilledBucketTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("item")) {
        auto result = ItemPredicate::fromJson(json["item"]);
        if (result.failed()) {
            return result.error();
        }
        m_item = result.value();
    }

    return {};
}

nlohmann::json FilledBucketTriggerInstance::conditionsToJson() const
{
    if (!m_item.isAny()) {
        return {{"item", m_item.toJson()}};
    }
    return nullptr;
}

// ========== FilledBucketTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> FilledBucketTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<FilledBucketTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void FilledBucketTrigger::trigger(ServerPlayer& player, const ItemStack& item)
{
    // 触发器通过 AdvancementEventHandler::onFilledBucket() 调用基类的 trigger() 模板方法
    // 此方法保留作为接口，实际触发逻辑在服务端事件处理器中
    MC_UNUSED(player);
    MC_UNUSED(item);
}

} // namespace mc::advancement
