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
 * @brief 差分能力：附魔台给出的可选附魔列表（纯计算）。
 *
 * `list`：一组「物品 id|附魔等级」
 * `run` ：{ "item": "minecraft:diamond_sword", "level": 30 }
 *         → { "results": [ { "enchantment": "minecraft:sharpness", "level": 2 }, ... ] }
 *
 * 复刻原版 `EnchantmentHelper.getAvailableEnchantmentResults(level, stack, stream)`：
 *
 * ```java
 * boolean flag = stack.is(Items.BOOK);
 * stream.filter(e -> e.value().isPrimaryItem(stack) || flag)
 *       .forEach(e -> {
 *           for (int i = e.value().getMaxLevel(); i >= e.value().getMinLevel(); i--) {
 *               if (level >= e.value().getMinCost(i) && level <= e.value().getMaxCost(i)) {
 *                   list.add(new EnchantmentInstance(e, i));
 *                   break;                 // 每个附魔最多一项
 *               }
 *           }
 *       });
 * ```
 *
 * ## 三个关键点
 *
 * 1. **等级从高到低试，命中即停**。每个附魔最多产生一项，取的是「能负担的最高等级」。
 *    从低往高试会让同一附魔出现多次。
 *
 * 2. **书（BOOK）豁免主物品检查**。`isPrimaryItem(stack) || flag` 里 `flag` 就是
 *    「是不是书」——书可以承载任何附魔。
 *
 * 3. **区间判定用 `getMinCost` / `getMaxCost`**，不是 `getMaxEnchantability`。
 *    后者是 `min + 15`，而 `getMaxCost` 是 `min + 5`，上界不同会直接改变命中结果。
 *
 * ## 与 Cubium 的一处口径差异
 *
 * Cubium 的 `EnchantmentHelper::getEnchantmentDatas` 在区间判定上用的是
 * `getMinEnchantability` / `getMaxEnchantability`（上界 = min + 15），且额外加了
 * `isTreasure()` 与 `canGenerateInLoot()` 两道过滤——这两道在原版
 * `getAvailableEnchantmentResults` 里并不存在。因此本探针按原版口径复算，
 * 不复用 `getEnchantmentDatas`。
 *
 * TODO: 待 Cubium 把 `getEnchantmentDatas` 的区间与过滤条件对齐原版后，探针应改为
 * 直接调用它以覆盖真实实现。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 覆盖用的物品表。
 *
 * 选择依据：各主要装备类型的代表（它们的附魔池差异大）；书（检验 `|| isBook` 的
 * 豁免分支）；空气（检验空栈短路）；不能附魔的普通物品（检验「无任何候选」）。
 */
constexpr const char* ITEMS[] = {
    "minecraft:book",
    "minecraft:diamond_sword",
    "minecraft:diamond_pickaxe",
    "minecraft:diamond_axe",
    "minecraft:diamond_helmet",
    "minecraft:diamond_boots",
    "minecraft:bow",
    "minecraft:fishing_rod",
    "minecraft:trident",
    "minecraft:elytra",
    "minecraft:shield",
    "minecraft:air",
    "minecraft:dirt",
    "minecraft:stick",
};

/// 覆盖附魔等级代价曲线的关键取值：0（无效）、各档位、超出最大值。
constexpr i32 LEVELS[] = {0, 1, 5, 10, 15, 20, 25, 30, 35, 40, 50, 100};

void writeList(FILE* out)
{
    for (const auto* item : ITEMS) {
        for (i32 level : LEVELS) {
            std::fprintf(out, "%s|%d\n", item, level);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string itemId = input.at("item").get<std::string>();
    const i32 level = input.at("level").get<i32>();

    Items::initialize();
    // 附魔注册表需显式初始化：否则遍历为空，表现为「所有物品都没有可选附魔」。
    if (!item::enchant::EnchantmentRegistry::isInitialized()) {
        item::enchant::EnchantmentRegistry::initialize();
    }

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
    if (item == nullptr) {
        throw std::runtime_error("enchantment_selection: 未知物品 id：" + itemId);
    }

    ItemStack stack(item, 1);

    json results = json::array();
    const bool isBook = item == Items::BOOK;
    for (const auto& [id, enchantmentPtr] : item::enchant::EnchantmentRegistry::all()) {
        const auto* enchantment = enchantmentPtr.get();
        if (enchantment == nullptr) {
            continue;
        }
        // 原版只有这一道过滤：主物品匹配，或者目标是书。
        if (!enchantment->canApplyAtEnchantingTable(stack) && !isBook) {
            continue;
        }

        // 从高到低试，命中即停。
        for (i32 lvl = enchantment->maxLevel(); lvl >= enchantment->minLevel(); --lvl) {
            if (level >= enchantment->getMinCost(lvl) && level <= enchantment->getMaxCost(lvl)) {
                results.push_back(json{{"enchantment", enchantment->id()}, {"level", lvl}});
                break;
            }
        }
    }

    // 按附魔 id 排序，与 oracle 侧的排序一致。原版的返回顺序由附魔注册表决定，
    // 顺序本身不是本能力要验证的口径，排序后差异集中在「哪些附魔、各几级」上。
    std::sort(results.begin(), results.end(), [](const json& a, const json& b) {
        return a.at("enchantment").get<std::string>() < b.at("enchantment").get<std::string>();
    });

    // 单元标识：物品与等级的拼接。同一物品有多个等级，单靠 item 无法区分。
    const std::string unit = itemId + "|" + std::to_string(level);

    return json{{"unit", unit}, {"item", itemId}, {"level", level}, {"count", results.size()}, {"results", results}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_selection", &writeList, &runLine)

} // namespace mc::diff
