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

#include "probe.hpp"

#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace mc::diff {

namespace {

using json = nlohmann::json;

/**
 * @file inventory_add.cpp
 * @brief 物品进入玩家背包时的装填顺序（原版 `Inventory.add` / `addResource`）。
 *
 * 玩家捡起物品、合成产物入包、漏斗输出到玩家，最后都走这条路径。原版的顺序是：
 *
 *   1. `getSlotWithRemainingSpace`：先看**选中槽**能不能装下，再看**副手槽（40）**，
 *      最后从 0 开始按索引递增扫全部槽位；
 *   2. 都装不下时 `getFreeSlot`：同样从 0 递增找第一个空槽。
 *
 * 两个细节最容易写错：
 *
 *   - **副手槽排在全部槽位扫描之前**，不是「顺便也看一眼副手」；
 *   - **扫描与找空槽都是从索引 0 开始递增**，而 0..8 是快捷栏——所以捡东西会先塞满
 *     快捷栏再去主背包。写成「先主背包后快捷栏」会让物品凭空出现在背包第二排。
 *
 * 另外 `add` 返回**没装下的剩余数量**，而不是装进去了多少，这两个值在很多场景下不相等。
 *
 * 单元格式：`<场景名>`，场景描述一组「预置槽位 + 要加入的物品」。
 */

/// 预置：往某个槽位放一个指定数量的物品。
struct Preset {
    const char* item;
    i32 slot;
    i32 count;
};

/// 一个场景。
struct Scene {
    const char* name;
    /// 选中槽
    i32 selected;
    /// 预置槽位
    std::vector<Preset> presets;
    /// 要加入的物品
    const char* addItem;
    /// 要加入的数量
    i32 addCount;
};

const std::vector<Scene>& scenes()
{
    static const std::vector<Scene> table = {
        // 空背包：应从索引 0 开始放，落在快捷栏第一格
        {"empty_inventory", 0, {}, "minecraft:stone", 1},
        // 空背包放一整组：仍是一个槽
        {"empty_full_stack", 0, {}, "minecraft:stone", 64},
        // 空背包放超过一组：应铺到第二个槽
        {"empty_over_stack", 0, {}, "minecraft:stone", 100},
        // 选中槽已有同类且未满：应补齐选中槽
        {"merge_into_selected", 5, {{"minecraft:stone", 5, 10}}, "minecraft:stone", 20},
        // 选中槽是空的，而副手有同类未满：副手优先于扫描
        {"merge_into_offhand", 0, {{"minecraft:stone", 40, 10}}, "minecraft:stone", 20},
        // 索引 1 有同类未满，副手也有同类未满：副手应先被填
        {"offhand_before_scan", 0, {{"minecraft:stone", 1, 10}, {"minecraft:stone", 40, 10}},
            "minecraft:stone", 20},
        // 索引 1、3 都有同类未满：应从下标小的开始
        {"scan_ascending", 0, {{"minecraft:stone", 1, 50}, {"minecraft:stone", 3, 50}}, "minecraft:stone", 20},
        // 快捷栏满、主背包有空位：应落到主背包
        {"hotbar_full", 0,
            {{"minecraft:dirt", 0, 64}, {"minecraft:dirt", 1, 64}, {"minecraft:dirt", 2, 64},
                {"minecraft:dirt", 3, 64}, {"minecraft:dirt", 4, 64}, {"minecraft:dirt", 5, 64},
                {"minecraft:dirt", 6, 64}, {"minecraft:dirt", 7, 64}, {"minecraft:dirt", 8, 64}},
            "minecraft:stone", 1},
        // 不同物品不合并
        {"different_item_no_merge", 0, {{"minecraft:dirt", 0, 10}}, "minecraft:stone", 10},
        // 加入 0 个：不应改变任何槽
        {"add_zero", 0, {}, "minecraft:stone", 0},
        // 加入空物品：不应改变任何槽
        {"add_air", 0, {}, "minecraft:air", 5},
        // 选中槽已满：跳到副手
        {"selected_full", 0, {{"minecraft:stone", 0, 64}}, "minecraft:stone", 10},
        // 选中槽、副手、索引都满，但有空槽：应进第一个空槽
        {"all_matching_full", 0, {{"minecraft:stone", 0, 64}, {"minecraft:stone", 40, 64}},
            "minecraft:stone", 10},
        // 加入数量跨多个槽：应依次铺满
        {"spans_multiple", 0, {{"minecraft:stone", 0, 60}}, "minecraft:stone", 200},
    };
    return table;
}

void ensureRegistries()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        s_initialized = true;
    }
}

const Item* findItem(const std::string& id)
{
    return ItemRegistry::instance().getItem(ResourceLocation(id));
}

void writeList(std::FILE* out)
{
    for (const auto& scene : scenes()) {
        std::fprintf(out, "%s\n", scene.name);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureRegistries();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("unit").get<std::string>();

    const Scene* found = nullptr;
    for (const auto& candidate : scenes()) {
        if (requested == candidate.name) {
            found = &candidate;
            break;
        }
    }
    if (found == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }

    PlayerInventory inventory(nullptr);
    inventory.setSelectedSlot(found->selected);

    for (const auto& preset : found->presets) {
        const Item* item = findItem(preset.item);
        if (item == nullptr) {
            return json{{"unit", requested}, {"missing", true}}.dump();
        }
        inventory.setItem(preset.slot, ItemStack(*item, preset.count));
    }

    const Item* addItem = findItem(found->addItem);
    if (addItem == nullptr) {
        return json{{"unit", requested}, {"missing", true}}.dump();
    }
    ItemStack toAdd(*addItem, found->addCount);
    const i32 beforeCount = toAdd.getCount();
    inventory.add(toAdd);
    // 输出「实际进了背包多少个」，= 加入前数量 - 加入后剩余。
    //
    // 不输出 add 的返回值（原版是 bool，Cubium 是剩余量，两者在空堆叠上分叉），
    // 也不输出 toAdd 的残留数量（空气这类物品两侧的堆叠归一化时机不同，会造出
    // 与游戏表现无关的假差异）。进包数量是玩家唯一能观察到的结果。
    const i32 placed = beforeCount - toAdd.getCount();

    // 只输出有内容的槽位，避免把 41 个空格塞进报告。
    json slots = json::array();
    for (i32 slot = 0; slot < PlayerInventory::TOTAL_SIZE; ++slot) {
        const ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }
        slots.push_back(
            json{{"slot", slot}, {"item", stack.getItem()->itemLocation().toString()}, {"count", stack.getCount()}});
    }

    return json{{"unit", requested}, {"placed", placed}, {"slots", slots}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(inventory_add, &writeList, &runLine)

} // namespace mc::diff