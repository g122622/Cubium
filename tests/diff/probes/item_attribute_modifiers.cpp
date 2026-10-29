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

/**
 * @file item_attribute_modifiers.cpp
 * @brief 差分能力：物品按装备槽位提供的属性修饰符（vanilla `ItemAttributeModifiers` 组件）。
 *
 * `list`：全部物品 ID
 * `run` ：{ "item": "minecraft:diamond_sword" }
 *         → 每个装备槽位下该物品提供的修饰符列表
 *
 * 这一项是玩家面板数值的直接来源：武器的攻击伤害与攻击速度、盔甲的护甲值与韧性、
 * 击退抗性。差异不体现在任何「报错」上——数值低一点，游戏照样能玩，只有和原版并排
 * 才能看出来。修饰符的 ID 同样要比：同一槽位上的修饰符靠 ID 去重与替换，
 * ID 不一致会让两件装备叠加或互相覆盖，与原版行为分叉。
 *
 * 只读 `Item::getAttributeModifiers(slot)`——原版 `getDefaultAttributeModifiers`
 * 会把该槽位的条目过滤出来，这里由各物品自己的重写实现完成同样的过滤。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/Items.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        s_initialized = true;
    }
}

/** 装备槽位的名字：与原版 `EquipmentSlot` 的 `slotName`（序列化键）一致。 */
struct SlotName {
    i32 slot;
    const char* name;
};

constexpr SlotName kSlots[] = {
    {static_cast<i32>(EquipmentSlot::MainHand), "mainhand"},
    {static_cast<i32>(EquipmentSlot::OffHand), "offhand"},
    {static_cast<i32>(EquipmentSlot::Feet), "feet"},
    {static_cast<i32>(EquipmentSlot::Legs), "legs"},
    {static_cast<i32>(EquipmentSlot::Chest), "chest"},
    {static_cast<i32>(EquipmentSlot::Head), "head"},
    {static_cast<i32>(EquipmentSlot::Body), "body"},
};

/** 运算名：与原版 `AttributeModifier.Operation` 的序列化键一致。 */
const char* operationText(entity::attribute::Operation operation)
{
    switch (operation) {
        case entity::attribute::Operation::Addition:
            return "add_value";
        case entity::attribute::Operation::MultiplyBase:
            return "add_multiplied_base";
        case entity::attribute::Operation::MultiplyTotal:
            return "add_multiplied_total";
    }
    return "unknown";
}

void writeList(FILE* out)
{
    ensureItems();
    std::vector<std::string> names;
    ItemRegistry::instance().forEachItem([&](Item& item) {
        names.push_back(item.itemLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("item").get<std::string>();

    Item* item = ItemRegistry::instance().getItem(ResourceLocation(requested));
    if (item == nullptr) {
        return json{{"item", requested}, {"missing", true}}.dump();
    }

    // 槽位是对象而不是数组：原版是「槽位 → 条目列表」的映射，空槽位直接缺席，
    // 比对一个全空的槽位数组只是噪声。
    json slots = json::object();
    for (const SlotName& slot : kSlots) {
        const item::ItemAttributeModifiers modifiers = item->getAttributeModifiers(slot.slot);
        json entries = json::array();
        for (const auto& entry : modifiers.getEntries()) {
            entries.push_back(json{{"attribute", entry.attributeName},
                {"id", entry.modifier.id()},
                {"operation", operationText(entry.modifier.operation())},
                {"amount", entry.modifier.amount()}});
        }
        if (!entries.empty()) {
            slots[slot.name] = std::move(entries);
        }
    }

    return json{{"item", requested}, {"slots", std::move(slots)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_attribute_modifiers", &writeList, &runLine)

} // namespace mc::diff
