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
 * @brief 差分能力：两个物品栈是否「同物品且同组件」（纯计算）。
 *
 * `list`：一组「左物品|左损伤|左数量|右物品|右损伤|右数量」
 * `run` ：同上一行
 *         → { "sameItemSameComponents": true, "matches": false }
 *
 * 复刻原版 `ItemStack` 的两个静态方法：
 *
 * ```java
 * public static boolean isSameItemSameComponents(ItemStack a, ItemStack b) {
 *     if (!a.is(b.getItem())) {
 *         return false;
 *     }
 *     return a.isEmpty() && b.isEmpty() ? true : Objects.equals(a.components, b.components);
 * }
 *
 * public static boolean matches(ItemStack a, ItemStack b) {
 *     if (a == b) {
 *         return true;
 *     }
 *     return a.getCount() != b.getCount() ? false : isSameItemSameComponents(a, b);
 * }
 * ```
 *
 * ## 三个关键点
 *
 * 1. **两个空栈被视为「相同」**。`a.isEmpty() && b.isEmpty()` 这一支返回 `true`——
 *    空栈之间没有组件可比，语义上就是相等。写成「任一为空即 false」会改变空槽合并行为。
 *
 * 2. **`matches` 比 `isSameItemSameComponents` 多一道数量判定**。前者要求数量也相等，
 *    后者不看数量。两者用途不同：前者用于「完整相等」的比较（如配方匹配），后者用于
 *    「能否堆叠到一起」。
 *
 * 3. **组件相等是深比较**。损伤值、附魔、自定义名称、lore 都属于组件；任意一项不同就
 *    不相等。因此「同物品、不同损伤」的两件钻石剑**不能**堆叠。
 *
 * ## 与 Cubium 的口径差异
 *
 * Cubium 没有 `isSameItemSameComponents`，最接近的是 `ItemStack::canMergeWith`。后者在
 * 任一为空时返回 `false`（原版两空返回 `true`），且只比较了损伤、修复成本、名称、lore
 * 等若干字段，不是完整的组件深比较。本能力按原版口径复算。
 *
 * TODO: 待 Cubium 实现完整的组件深比较后，探针应改为调用它以覆盖真实实现。
 * 当前单元表只能通过「损伤值」这一维制造组件差异（`ItemStack` 没有为其他组件提供
 * 设置入口），因此附魔、名称、lore 的差异未覆盖。
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
 * @brief 单元表：`左物品|左损伤|左数量|右物品|右损伤|右数量`。
 *
 * 覆盖：
 * - 同物品、同损伤、同数量（完全相等）；
 * - 同物品、同损伤、不同数量（`isSameItemSameComponents` 真、`matches` 假）；
 * - 同物品、不同损伤（组件不同）；
 * - 不同物品；
 * - 全空栈配对（原版返回 true）；
 * - 一空一非空；
 * - 上限 64 的物品与上限 1 的物品交叉配对。
 */
constexpr const char* UNITS[] = {
    // 完全相等
    "minecraft:dirt|0|1|minecraft:dirt|0|1",
    // 同物品同损伤、数量不同
    "minecraft:dirt|0|1|minecraft:dirt|0|5",
    // 同物品同损伤、数量 64
    "minecraft:dirt|0|64|minecraft:dirt|0|1",
    // 不同物品
    "minecraft:dirt|0|1|minecraft:stone|0|1",
    // 同物品、不同损伤（组件不同）
    "minecraft:diamond_sword|0|1|minecraft:diamond_sword|1|1",
    "minecraft:diamond_sword|100|1|minecraft:diamond_sword|200|1",
    // 同物品、同损伤
    "minecraft:diamond_sword|100|1|minecraft:diamond_sword|100|1",
    // 全空栈配对（air 在 Cubium 侧不是空栈，见 item_stackable 的说明）
    "minecraft:air|0|0|minecraft:air|0|0",
    // 空气物品对泥土
    "minecraft:air|0|0|minecraft:dirt|0|1",
    // 上限 1 与上限 64 的不同物品
    "minecraft:diamond_sword|0|1|minecraft:dirt|0|1",
    // 同物品、损伤相同时数量不同（可损坏物品）
    "minecraft:bow|50|1|minecraft:bow|50|2",
    // 同物品、一个损伤一个未损伤
    "minecraft:bow|0|1|minecraft:bow|1|1",
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

struct Spec {
    std::string itemId;
    i32 damage;
    i32 count;
};

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 6) {
        throw std::runtime_error(
            "item_same_components: 单元应为 6 段 '左物品|左损伤|左数量|右物品|右损伤|右数量'：" + unit);
    }

    const Spec leftSpec{parts[0], std::stoi(parts[1]), std::stoi(parts[2])};
    const Spec rightSpec{parts[3], std::stoi(parts[4]), std::stoi(parts[5])};

    Items::initialize();

    const auto build = [](const Spec& spec) {
        const Item* item = ItemRegistry::instance().getItem(ResourceLocation(spec.itemId));
        if (item == nullptr) {
            throw std::runtime_error("item_same_components: 未知物品 id：" + spec.itemId);
        }
        // 数量为 0 表示空栈；否则按给定数量构造。
        ItemStack stack(item, std::max(1, spec.count));
        if (spec.count == 0) {
            stack = ItemStack();
        }
        if (spec.damage > 0) {
            stack.setDamage(spec.damage);
        }
        return stack;
    };

    const ItemStack left = build(leftSpec);
    const ItemStack right = build(rightSpec);

    // 原版 isSameItemSameComponents：先比物品，两空视为相同，否则比组件。
    // 注意顺序——判空必须在比物品**之后**，由「两空」决定，而不是「任一为空即假」。
    bool sameItemSameComponents = false;
    if (left.isEmpty() && right.isEmpty()) {
        sameItemSameComponents = true;
    } else if (!left.isEmpty() && !right.isEmpty() && left.getItem() == right.getItem()) {
        // 组件差异在本能力里只能通过损伤值制造（ItemStack 未提供其他组件的设置入口）。
        sameItemSameComponents = left.getDamage() == right.getDamage();
    }

    // 原版 matches：数量也要相等。
    const bool matches = left.getCount() == right.getCount() && sameItemSameComponents;

    return json{{"unit", unit},
        {"leftEmpty", left.isEmpty()},
        {"rightEmpty", right.isEmpty()},
        {"leftDamage", left.getDamage()},
        {"rightDamage", right.getDamage()},
        {"leftCount", left.getCount()},
        {"rightCount", right.getCount()},
        {"sameItemSameComponents", sameItemSameComponents},
        {"matches", matches}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_same_components", &writeList, &runLine)

} // namespace mc::diff
