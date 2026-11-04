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
 * @file fuel_values.cpp
 * @brief 差分能力：每种物品作为燃料的**燃烧时长**（原版 1.21.11 `FuelValues`）。
 *
 * `list`：全部已注册物品 ID
 * `run` ：{ "item": "minecraft:oak_planks" }
 *         → { "ok": true, "burnTime": 300 }
 *
 * ## 为什么值得单独比
 *
 * 燃料表是「一张把物品种类映射到 tick 数的表」，而玩家对它的容错极低：少一种木板，
 * 玩家在炉子里放它就不会被识别成燃料；数值差一倍，烧一组铁矿的时间整个不对。这类
 * 错误不会崩溃、不会报错，只会让玩家觉得「炉子好像有点怪」。
 *
 * 原版 1.21.11 的这张表用**标签聚合**表达——`ItemTags.LOGS` 一条就覆盖全部原木变体，
 * 再在末尾 `.remove(ItemTags.NON_FLAMMABLE_WOOD)` 把绯红/诡异木剔除。手写成逐物品的
 * if 链几乎必然漏掉新木材变体（1.21 加了不少），所以这里按物品逐个比对最终时长，
 * 而不比「用了哪些标签」：标签是原版的表达手段，玩家看到的只有时长。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/processing/AbstractFurnaceEntity.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 全部已注册物品 ID（排序去重）。
[[nodiscard]] const std::vector<std::string>& itemIds()
{
    static std::vector<std::string> s_ids;

    static bool s_initialised = false;
    if (!s_initialised) {
        s_initialised = true;

        datapack::initializeRegistries();

        ItemRegistry::instance().forEachItem([](Item& item) {
            s_ids.push_back(item.itemLocation().toString());
        });
        std::sort(s_ids.begin(), s_ids.end());
        s_ids.erase(std::unique(s_ids.begin(), s_ids.end()), s_ids.end());
    }
    return s_ids;
}

void writeList(FILE* out)
{
    for (const auto& id : itemIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    static_cast<void>(itemIds());

    const json input = json::parse(inputLine);
    const std::string unit = input.at("item").get<std::string>();

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(unit));
    // 空气不算「物品」：原版 oracle 把它当作未注册（`Item == Items.AIR` 直接判否），
    // 两侧口径必须一致，否则会在 `minecraft:air` 上凭空多出一条差异。
    if (item == nullptr || item->itemLocation() == ResourceLocation("minecraft:air")) {
        return json{{"item", unit}, {"ok", false}, {"error", "unknown item"}}.dump();
    }

    // 走服务端炉子同一条判定入口，避免探针自己复述燃料表。
    const i32 burnTime = blockentity::AbstractFurnaceEntity::getBurnTime(ItemStack(item, 1));
    return json{{"item", unit}, {"ok", true}, {"burnTime", burnTime}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(fuel_values, &writeList, &runLine)

} // namespace mc::diff
