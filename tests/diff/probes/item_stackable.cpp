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
 * @brief 差分能力：物品能否堆叠（纯计算）。
 *
 * `list`：一组「物品 id|损伤值」
 * `run` ：同上一行
 *         → { "maxStackSize": 1, "damageable": true, "damaged": true, "stackable": false }
 *
 * 复刻原版 `ItemStack` 的三个方法：
 *
 * ```java
 * public int getMaxStackSize() {
 *     return this.getOrDefault(DataComponents.MAX_STACK_SIZE, 1);   // 默认 1
 * }
 *
 * public boolean isDamageableItem() {
 *     return this.has(DataComponents.MAX_DAMAGE)
 *         && !this.has(DataComponents.UNBREAKABLE)
 *         && this.has(DataComponents.DAMAGE);
 * }
 *
 * public boolean isStackable() {
 *     return this.getMaxStackSize() > 1 && (!this.isDamageableItem() || !this.isDamaged());
 * }
 * ```
 *
 * ## 四个容易搞错的点
 *
 * 1. **`MAX_STACK_SIZE` 默认是 1，不是 64**。没有这个组件的物品（工具、盔甲）一律
 *    不可堆叠。默认值写成 64 会让所有工具都被判为可堆叠。
 *
 * 2. **可损坏 ≠ 可堆叠上限为 1**。两者是**独立**的维度：可损坏物品的上限也可能是 1，
 *    但决定能否堆叠的是「是否已损坏」这一条附加条件。
 *
 * 3. **`UNBREAKABLE` 会让物品重新变得可堆叠**。`isDamageableItem()` 里有
 *    `!has(UNBREAKABLE)`——不可破坏的钻石剑在组件层面不再是「可损坏物品」，于是
 *    `isStackable()` 直接放行。这是一个非常反直觉的结论（虽然实际游戏里不可破坏工具
 *    通常仍是上限 1）。
 *
 * 4. **「已损坏」= 有损伤值**，不是「耐久低于一半」。
 *
 * TODO: Cubium 的 `ItemStack` 目前没有设置 `UNBREAKABLE` 组件的公开入口，因此本能力的
 * 单元表只通过真实物品覆盖「上限 1 / 上限 64 / 已损伤」三种组合，未覆盖第 3 条的
 * `UNBREAKABLE` 分支。待补上设置入口后应扩展。
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
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 单元表：`物品 id|损伤值`。
 *
 * 覆盖：
 * - 上限 64 且无损伤（泥土、木棍、苹果）——可堆叠；
 * - 上限 1 且不可损坏（下界之星、盾牌、鞍）——不可堆叠；
 * - 上限 1 且可损坏但无损伤（钻石剑、镐、盔甲）——可堆叠为 1？按原版是可堆叠的
 *   （`maxStack > 1` 为假，所以不可堆叠）；
 * - 上限 1 且可损坏且已损伤——不可堆叠；
 * - 上限 16（雪球、末影珍珠、告示牌）——可堆叠；
 * - 上限 1 的桶、药水——不可堆叠。
 */
constexpr const char* UNITS[] = {
    // 上限 64，不可损坏
    "minecraft:dirt|0",
    "minecraft:stick|0",
    "minecraft:apple|0",
    // 上限 64，但设置了损伤（不可损坏物品的损伤值会被忽略）
    "minecraft:dirt|5",
    // 上限 16
    "minecraft:snowball|0",
    "minecraft:ender_pearl|0",
    // 上限 1，不可损坏（无 max_damage）
    "minecraft:nether_star|0",
    "minecraft:shield|0",
    "minecraft:saddle|0",
    "minecraft:potion|0",
    "minecraft:bucket|0",
    // 上限 1，可损坏，无损伤
    "minecraft:diamond_sword|0",
    "minecraft:diamond_pickaxe|0",
    "minecraft:diamond_helmet|0",
    "minecraft:elytra|0",
    "minecraft:bow|0",
    // 上限 1，可损坏，已损伤
    "minecraft:diamond_sword|1",
    "minecraft:diamond_sword|500",
    "minecraft:diamond_pickaxe|1",
    "minecraft:diamond_helmet|1",
    "minecraft:elytra|1",
    // 接近损坏上限（钻石剑 1561）
    "minecraft:diamond_sword|1560",
    // 空气
    "minecraft:air|0",
};

void writeList(FILE* out)
{
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
    if (parts.size() != 2) {
        throw std::runtime_error("item_stackable: 单元应为 2 段 '物品id|损伤值'：" + unit);
    }

    const std::string itemId = parts[0];
    const i32 damage = std::stoi(parts[1]);

    Items::initialize();
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(itemId));
    if (item == nullptr) {
        throw std::runtime_error("item_stackable: 未知物品 id：" + itemId);
    }

    ItemStack stack(item, 1);
    // setDamage 对不可损坏物品是空操作；对可损坏物品钳到 [0, maxDamage)。
    if (damage > 0) {
        stack.setDamage(damage);
    }

    return json{{"unit", unit},
        {"item", itemId},
        {"damage", damage},
        {"empty", stack.isEmpty()},
        {"maxStackSize", stack.getMaxStackSize()},
        {"damageable", stack.isDamageable()},
        {"damaged", stack.isDamaged()},
        {"stackable", stack.isStackable()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_stackable", &writeList, &runLine)

} // namespace mc::diff
