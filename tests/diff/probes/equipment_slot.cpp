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
 * @brief 差分能力：装备槽位与槽位组的匹配判定（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.entity.EquipmentSlot` 与 `EquipmentSlotGroup`：
 *
 * ```java
 * // EquipmentSlot： (类型, 索引, 数量上限, 数字 id, 名字)
 * MAINHAND(Type.HAND,           0, 0, 0, "mainhand")
 * OFFHAND (Type.HAND,           1, 5, 1, "offhand")
 * FEET    (Type.HUMANOID_ARMOR, 0, 1, 1, "feet")
 * LEGS    (Type.HUMANOID_ARMOR, 1, 1, 2, "legs")
 * CHEST   (Type.HUMANOID_ARMOR, 2, 1, 3, "chest")
 * HEAD    (Type.HUMANOID_ARMOR, 3, 1, 4, "head")
 * BODY    (Type.ANIMAL_ARMOR,   0, 1, 6, "body")
 * SADDLE  (Type.SADDLE,         0, 1, 7, "saddle")
 *
 * public boolean isArmor() {
 *     return type == Type.HUMANOID_ARMOR || type == Type.ANIMAL_ARMOR;
 * }
 *
 * // EquipmentSlotGroup
 * ANY     (0,  "any",      s -> true)
 * MAINHAND(1,  "mainhand", EquipmentSlot.MAINHAND)
 * OFFHAND (2,  "offhand",  EquipmentSlot.OFFHAND)
 * HAND    (3,  "hand",     s -> s.getType() == Type.HAND)
 * FEET    (4,  "feet",     EquipmentSlot.FEET)
 * LEGS    (5,  "legs",     EquipmentSlot.LEGS)
 * CHEST   (6,  "chest",    EquipmentSlot.CHEST)
 * HEAD    (7,  "head",     EquipmentSlot.HEAD)
 * ARMOR   (8,  "armor",    EquipmentSlot::isArmor)
 * BODY    (9,  "body",     EquipmentSlot.BODY)
 * SADDLE  (10, "saddle",   EquipmentSlot.SADDLE)
 *
 * public static final IntFunction<EquipmentSlotGroup> BY_ID =
 *     ByIdMap.continuous(g -> g.id, values(), ByIdMap.OutOfBoundsStrategy.ZERO);
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`isArmor()` 包含 `BODY`（动物护甲），但不包含 `SADDLE`。**
 *
 *    ```java
 *    return type == Type.HUMANOID_ARMOR || type == Type.ANIMAL_ARMOR;
 *    ```
 *
 *    `SADDLE` 是独立的 `Type.SADDLE`，**不算护甲**。所以 `ARMOR` 组恰好覆盖
 *    `{FEET, LEGS, CHEST, HEAD, BODY}` 五个槽位——**五个，不是四个**。经典的「四件套」忽略了
 *    鞍具槽里的动物护甲。
 *
 * 2. **`EquipmentSlot` 的「索引」与「数字 id」是两套不同的编号。**
 *
 *    | 槽位 | type | index | 数量上限 | 数字 id |
 *    |---|---|---|---|---|
 *    | `MAINHAND` | `HAND` | 0 | **0** | 0 |
 *    | `OFFHAND` | `HAND` | 1 | 5 | 1 |
 *    | `FEET` | `HUMANOID_ARMOR` | 0 | 1 | **1** |
 *    | `LEGS` | `HUMANOID_ARMOR` | 1 | 1 | 2 |
 *    | `CHEST` | `HUMANOID_ARMOR` | 2 | 1 | 3 |
 *    | `HEAD` | `HUMANOID_ARMOR` | 3 | 1 | 4 |
 *    | `BODY` | `ANIMAL_ARMOR` | 0 | 1 | **6** |
 *    | `SADDLE` | `SADDLE` | 0 | 1 | 7 |
 *
 *    `FEET` 的 `index` 是 0 而数字 `id` 是 1；`BODY` 的 `index` 是 0 而 `id` 是 6。**数字 id 有
 *    空洞（没有 5）**，因为历史上 `CHEST` 曾是 5 但后来重排。
 *
 *    `MAINHAND` 的**数量上限是 0**（不是 1），因为主手槽可以堆叠；`OFFHAND` 是 **5**。
 *
 * 3. **`EquipmentSlotGroup.BY_ID` 越界返回 `ANY`（id 0）。**
 *
 *    与 [`dye_color`](./dye_color.md) 的 `byId` 同一策略（`OutOfBoundsStrategy.ZERO`）。
 *    `byId(-1)`、`byId(11)`、`byId(999)` 全部返回 `ANY`。
 *
 * 4. **`HAND` 组按 `Type` 判定，`ARMOR` 组按 `isArmor()` 判定，语义层次不同。**
 *
 *    - `HAND` 组 = `{MAINHAND, OFFHAND}`（`Type.HAND`）
 *    - `ARMOR` 组 = `{FEET, LEGS, CHEST, HEAD, BODY}`（`HUMANOID_ARMOR ∪ ANIMAL_ARMOR`）
 *    - `ANY` 组 = 全部 8 个
 *
 *    `ARMOR` **不含** `SADDLE`，尽管鞍具在装备菜单里也占一格。
 *
 * 单元格式：
 * - `slotInfo|<槽位名>` —— 输出 `type,index,countLimit,id,name,isArmor`
 * - `groupTest|<组名>|<槽位名>` —— 该组是否匹配该槽位
 * - `groupMembers|<组名>` —— 该组匹配的全部槽位（逗号分隔，按 `EquipmentSlot` 顺序）
 * - `groupById|<id>` —— 槽位组的 `BY_ID` 查找
 * - `groupInfo|<组名>` —— 输出 `id,serializedName`
 *
 * TODO: 未覆盖 `EquipmentSlot.limit(ItemStack)`、`getIndex(int)` 的偏移重载、
 * `EquipmentSlotGroup.getSerializedName` 之外的自定义组（`createTestGroup` 等）。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 槽位类型。
enum class SlotType { HAND, HUMANOID_ARMOR, ANIMAL_ARMOR, SADDLE };

/// 一个装备槽位。
struct EquipmentSlot {
    const char* name;
    SlotType type;
    i32 index;
    i32 countLimit;
    i32 id;
};

/// 原版全部 8 个装备槽位。
constexpr EquipmentSlot SLOTS[] = {
    {"MAINHAND", SlotType::HAND, 0, 0, 0},
    {"OFFHAND", SlotType::HAND, 1, 0, 5},
    {"FEET", SlotType::HUMANOID_ARMOR, 0, 1, 1},
    {"LEGS", SlotType::HUMANOID_ARMOR, 1, 1, 2},
    {"CHEST", SlotType::HUMANOID_ARMOR, 2, 1, 3},
    {"HEAD", SlotType::HUMANOID_ARMOR, 3, 1, 4},
    {"BODY", SlotType::ANIMAL_ARMOR, 0, 1, 6},
    {"SADDLE", SlotType::SADDLE, 0, 1, 7},
};

constexpr i32 SLOT_COUNT = 8;

/// 原版 `EquipmentSlot.isArmor()`：`HUMANOID_ARMOR` 或 `ANIMAL_ARMOR`。
bool isArmor(const EquipmentSlot& slot)
{
    return slot.type == SlotType::HUMANOID_ARMOR || slot.type == SlotType::ANIMAL_ARMOR;
}

/// 槽位类型名。
const char* typeName(SlotType type)
{
    switch (type) {
        case SlotType::HAND:
            return "HAND";
        case SlotType::HUMANOID_ARMOR:
            return "HUMANOID_ARMOR";
        case SlotType::ANIMAL_ARMOR:
            return "ANIMAL_ARMOR";
        case SlotType::SADDLE:
            return "SADDLE";
    }
    return "?";
}

/// 枚举名 → 小写名（原版 `getName()` 返回小写形式）。
std::string lowerName(const char* name)
{
    std::string result;
    for (const char* p = name; *p != '\0'; ++p) {
        result.push_back(*p >= 'A' && *p <= 'Z' ? static_cast<char>(*p - 'A' + 'a') : *p);
    }
    return result;
}

/// 槽位名 → 槽位。
const EquipmentSlot* findSlot(const std::string& name)
{
    for (const auto& slot : SLOTS) {
        if (name == slot.name) {
            return &slot;
        }
    }
    return nullptr;
}

/// 一个槽位组。
struct SlotGroup {
    const char* name;
    i32 id;
    const char* serializedName;
};

/// 原版全部 11 个槽位组。
constexpr SlotGroup GROUPS[] = {
    {"ANY", 0, "any"},
    {"MAINHAND", 1, "mainhand"},
    {"OFFHAND", 2, "offhand"},
    {"HAND", 3, "hand"},
    {"FEET", 4, "feet"},
    {"LEGS", 5, "legs"},
    {"CHEST", 6, "chest"},
    {"HEAD", 7, "head"},
    {"ARMOR", 8, "armor"},
    {"BODY", 9, "body"},
    {"SADDLE", 10, "saddle"},
};

constexpr i32 GROUP_COUNT = 11;

/// 槽位组名 → 组。
const SlotGroup* findGroup(const std::string& name)
{
    for (const auto& group : GROUPS) {
        if (name == group.name) {
            return &group;
        }
    }
    return nullptr;
}

/// 原版 `EquipmentSlotGroup.test(EquipmentSlot)`。
bool groupTest(const std::string& groupName, const EquipmentSlot& slot)
{
    if (groupName == "ANY") {
        return true;
    }
    if (groupName == "MAINHAND") {
        return std::string(slot.name) == "MAINHAND";
    }
    if (groupName == "OFFHAND") {
        return std::string(slot.name) == "OFFHAND";
    }
    if (groupName == "HAND") {
        return slot.type == SlotType::HAND;
    }
    if (groupName == "FEET") {
        return std::string(slot.name) == "FEET";
    }
    if (groupName == "LEGS") {
        return std::string(slot.name) == "LEGS";
    }
    if (groupName == "CHEST") {
        return std::string(slot.name) == "CHEST";
    }
    if (groupName == "HEAD") {
        return std::string(slot.name) == "HEAD";
    }
    if (groupName == "ARMOR") {
        return isArmor(slot);
    }
    if (groupName == "BODY") {
        return std::string(slot.name) == "BODY";
    }
    if (groupName == "SADDLE") {
        return std::string(slot.name) == "SADDLE";
    }
    throw std::runtime_error("equipment_slot: 未知槽位组：" + groupName);
}

/// 原版 `EquipmentSlotGroup.BY_ID`：越界返回 `ANY`（下标 0）。
const SlotGroup* groupById(i32 id)
{
    if (id >= 0 && id < GROUP_COUNT) {
        return &GROUPS[id];
    }
    return &GROUPS[0];
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // slotInfo：全部 8 个槽位
        "slotInfo|MAINHAND",
        "slotInfo|OFFHAND",
        "slotInfo|FEET",
        "slotInfo|LEGS",
        "slotInfo|CHEST",
        "slotInfo|HEAD",
        "slotInfo|BODY",
        "slotInfo|SADDLE",
        // groupMembers：全部 11 个组
        "groupMembers|ANY",
        "groupMembers|MAINHAND",
        "groupMembers|OFFHAND",
        "groupMembers|HAND",
        "groupMembers|FEET",
        "groupMembers|LEGS",
        "groupMembers|CHEST",
        "groupMembers|HEAD",
        "groupMembers|ARMOR",
        "groupMembers|BODY",
        "groupMembers|SADDLE",
        // groupTest：关键交叉点
        "groupTest|ARMOR|BODY",
        "groupTest|ARMOR|SADDLE",
        "groupTest|ARMOR|HEAD",
        "groupTest|HAND|MAINHAND",
        "groupTest|HAND|OFFHAND",
        "groupTest|HAND|FEET",
        "groupTest|ANY|SADDLE",
        "groupTest|BODY|BODY",
        "groupTest|BODY|CHEST",
        // groupById：合法与越界
        "groupById|0",
        "groupById|3",
        "groupById|8",
        "groupById|10",
        "groupById|11",
        "groupById|-1",
        "groupById|999",
        // filterBit 与 canIncreaseExperience
        "filterBit|MAINHAND|0",
        "filterBit|CHEST|0",
        "filterBit|BODY|0",
        "filterBit|SADDLE|0",
        "filterBit|HEAD|10",
        "canXp|MAINHAND",
        "canXp|CHEST",
        "canXp|BODY",
        "canXp|SADDLE",
        // groupInfo
        "groupInfo|ANY",
        "groupInfo|ARMOR",
        "groupInfo|SADDLE",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("equipment_slot: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "slotInfo" && parts.size() == 2) {
        const EquipmentSlot* slot = findSlot(parts[1]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const std::string value = std::string(typeName(slot->type)) + "," + std::to_string(slot->index) + "," +
            std::to_string(slot->countLimit) + "," + std::to_string(slot->id) + "," + lowerName(slot->name) + "," +
            (isArmor(*slot) ? "true" : "false");
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "groupTest" && parts.size() == 3) {
        const EquipmentSlot* slot = findSlot(parts[2]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const bool matched = groupTest(parts[1], *slot);
        return json{{"unit", unit}, {"value", matched ? "true" : "false"}}.dump();
    }
    if (mode == "groupMembers" && parts.size() == 2) {
        std::string value;
        for (const auto& slot : SLOTS) {
            if (groupTest(parts[1], slot)) {
                if (!value.empty()) {
                    value += ",";
                }
                value += slot.name;
            }
        }
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "groupById" && parts.size() == 2) {
        const SlotGroup* group = groupById(std::stoi(parts[1]));
        return json{{"unit", unit}, {"value", std::string(group->name)}}.dump();
    }
    if (mode == "filterBit" && parts.size() == 3) {
        const EquipmentSlot* slot = findSlot(parts[1]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const i32 bit = slot->id + std::stoi(parts[2]);
        return json{{"unit", unit}, {"value", bit}}.dump();
    }
    if (mode == "canXp" && parts.size() == 2) {
        const EquipmentSlot* slot = findSlot(parts[1]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        return json{{"unit", unit}, {"value", slot->type != SlotType::SADDLE ? "true" : "false"}}.dump();
    }
    if (mode == "filterBit" && parts.size() == 3) {
        const EquipmentSlot* slot = findSlot(parts[1]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        return json{{"unit", unit}, {"value", slot->id + std::stoi(parts[2])}}.dump();
    }
    if (mode == "canXp" && parts.size() == 2) {
        const EquipmentSlot* slot = findSlot(parts[1]);
        if (slot == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const bool can = slot->type != SlotType::SADDLE;
        return json{{"unit", unit}, {"value", can ? "true" : "false"}}.dump();
    }
    if (mode == "groupInfo" && parts.size() == 2) {
        const SlotGroup* group = findGroup(parts[1]);
        if (group == nullptr) {
            return json{{"unit", unit}, {"value", "null"}}.dump();
        }
        const std::string value = std::to_string(group->id) + "," + group->serializedName;
        return json{{"unit", unit}, {"value", value}}.dump();
    }

    throw std::runtime_error("equipment_slot: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("equipment_slot", &writeList, &runLine)

} // namespace mc::diff
