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

#include "InventoryChangedTrigger.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

// 注意：trigger() 方法的完整实现需要服务端模块的支持
// 服务端代码应包含 server/advancement/TriggerInstantiation.hpp
// 并使用 triggerWithPredicate() 方法或直接调用基类的 trigger() 模板方法

namespace mc::advancement {

// ========== InventoryChangedTriggerInstance ==========

InventoryChangedTriggerInstance::InventoryChangedTriggerInstance(
    IntBounds slotsOccupied, IntBounds slotsFull, IntBounds slotsEmpty, std::vector<ItemPredicate> items)
    : m_slotsOccupied(std::move(slotsOccupied))
    , m_slotsFull(std::move(slotsFull))
    , m_slotsEmpty(std::move(slotsEmpty))
    , m_items(std::move(items))
{}

bool InventoryChangedTriggerInstance::testWithInventory(
    i32 totalSlots, const std::function<ItemStack(i32)>& getSlot) const
{
    // 计算槽位数量
    i32 occupied = 0;
    i32 full = 0;
    i32 empty = 0;

    // 遍历所有槽位
    for (i32 i = 0; i < totalSlots; ++i) {
        ItemStack slot = getSlot(i);
        if (slot.isEmpty()) {
            ++empty;
        } else {
            ++occupied;
            if (slot.getCount() >= slot.getMaxStackSize()) {
                ++full;
            }
        }
    }

    // 检查槽位范围条件
    if (!m_slotsOccupied.test(occupied)) {
        return false;
    }
    if (!m_slotsFull.test(full)) {
        return false;
    }
    if (!m_slotsEmpty.test(empty)) {
        return false;
    }

    // 检查物品谓词
    // 如果只有一个物品谓词，则检查变更的物品堆
    // 如果有多个物品谓词，则遍历整个物品栏，所有谓词都必须匹配
    if (!m_items.empty()) {
        const i32 itemCount = static_cast<i32>(m_items.size());

        if (itemCount == 1) {
            // 单个物品谓词：需要物品栏中至少有一个匹配的物品
            const ItemPredicate& predicate = m_items[0];
            for (i32 i = 0; i < totalSlots; ++i) {
                if (predicate.test(getSlot(i))) {
                    return true;
                }
            }
            return false;
        } else {
            // 多个物品谓词：所有谓词都必须匹配
            // 创建一个谓词匹配标记列表
            std::vector<bool> matched(itemCount, false);

            for (i32 i = 0; i < totalSlots; ++i) {
                ItemStack slot = getSlot(i);
                if (slot.isEmpty()) {
                    continue;
                }

                // 检查每个未匹配的谓词
                for (i32 j = 0; j < itemCount; ++j) {
                    if (!matched[j] && m_items[j].test(slot)) {
                        matched[j] = true;
                    }
                }
            }

            // 检查所有谓词是否都已匹配
            for (bool m : matched) {
                if (!m) {
                    return false;
                }
            }
        }
    }

    return true;
}

Result<void> InventoryChangedTriggerInstance::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return {};
    }

    if (json.contains("slots")) {
        const auto& slots = json["slots"];
        if (slots.contains("occupied")) {
            m_slotsOccupied = IntBounds::fromJson(slots["occupied"]);
        }
        if (slots.contains("full")) {
            m_slotsFull = IntBounds::fromJson(slots["full"]);
        }
        if (slots.contains("empty")) {
            m_slotsEmpty = IntBounds::fromJson(slots["empty"]);
        }
    }

    if (json.contains("items")) {
        for (const auto& itemJson : json["items"]) {
            auto result = ItemPredicate::fromJson(itemJson);
            if (result.failed()) {
                return result.error();
            }
            m_items.push_back(result.value());
        }
    }

    return {};
}

nlohmann::json InventoryChangedTriggerInstance::conditionsToJson() const
{
    nlohmann::json json;

    if (!m_slotsOccupied.isUnbounded() || !m_slotsFull.isUnbounded() || !m_slotsEmpty.isUnbounded()) {
        nlohmann::json slots;
        if (!m_slotsOccupied.isUnbounded()) {
            slots["occupied"] = m_slotsOccupied.toJson();
        }
        if (!m_slotsFull.isUnbounded()) {
            slots["full"] = m_slotsFull.toJson();
        }
        if (!m_slotsEmpty.isUnbounded()) {
            slots["empty"] = m_slotsEmpty.toJson();
        }
        json["slots"] = std::move(slots);
    }

    if (!m_items.empty()) {
        nlohmann::json items = nlohmann::json::array();
        for (const auto& item : m_items) {
            items.push_back(item.toJson());
        }
        json["items"] = std::move(items);
    }

    return json;
}

// ========== InventoryChangedTrigger ==========

Result<std::shared_ptr<ICriterionInstance>> InventoryChangedTrigger::fromJson(const nlohmann::json& json)
{
    auto instance = std::make_shared<InventoryChangedTriggerInstance>();
    auto result = instance->fromJson(json);
    if (result.failed()) {
        return result.error();
    }
    return instance;
}

void InventoryChangedTrigger::trigger(ServerPlayer& player, const PlayerInventory& inventory)
{
    // 此方法在 common 模块中无法完整实现，因为需要访问 PlayerAdvancements 的完整定义
    // 服务端代码应使用以下方式之一触发检测：
    //
    // 方法1：使用 triggerWithPredicate()
    // #include "server/advancement/TriggerInstantiation.hpp"
    // auto* trigger = CriterionTriggers::instance().getTrigger<InventoryChangedTrigger>();
    // trigger->triggerWithPredicate(*player.getAdvancements(), [&](const auto& instance) {
    //     return instance.testWithInventory(PlayerInventory::TOTAL_SIZE, getter);
    // });
    //
    // 方法2：使用 AdvancementEventHandler（推荐）
    // #include "server/advancement/AdvancementEventHandler.hpp"
    // AdvancementEventHandler::instance().onInventoryChanged(event);
    //
    // 参考：server/advancement/AdvancementEventHandler.hpp
    MC_UNUSED(player);
    MC_UNUSED(inventory);
}

std::shared_ptr<InventoryChangedTriggerInstance> InventoryChangedTrigger::hasItems(std::vector<ItemPredicate> items)
{
    return std::make_shared<InventoryChangedTriggerInstance>(IntBounds(), IntBounds(), IntBounds(), std::move(items));
}

std::shared_ptr<InventoryChangedTriggerInstance> InventoryChangedTrigger::hasItem(const ItemPredicate& item)
{
    return hasItems({item});
}

} // namespace mc::advancement
