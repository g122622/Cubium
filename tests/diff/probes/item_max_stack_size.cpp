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
 * @brief 差分能力：全部物品的堆叠上限（`ItemStack.getMaxStackSize` 的默认值）。
 *
 * `list`：每个已注册物品一个单元
 * `run` ：一行 -> { "maxStackSize": i32, "damageable": bool }
 *
 * ## 为什么值得全表比对
 *
 * 堆叠上限是**每个物品**的一个小整数（64 / 16 / 1），散落在上千处物品注册调用里。
 * 抄错的后果不会崩溃，但会实实在在影响玩法：
 *
 * - 上限写成 64 的物品（本该 1，例如桶、工具）会在一个格子里叠一摞；
 * - 上限写成 1 的物品（本该 64）会让玩家的物品栏凭空多占格子；
 * - 上限写成 16 的食材（本该 64）会让烤肉、面包按「药水/末影珍珠」的规则占格；
 * - 比较器读箱子时按 `count / maxStackSize` 折算占比，上限错会让**信号强度整体偏移**。
 *
 * 上一轮的 `container_analog_signal` 正是因为空桶被注册成 16 才暴露出问题；那条能力
 * 只覆盖 15 个容器布局，而这里把**每个物品**的上限都过一遍，才能把同类错误一次清零。
 *
 * ## 与 `item_stackable` 的关系
 *
 * `item_stackable` 用 23 个手工挑选的「物品 × 损伤值」验证 `isStackable()` 的三态逻辑
 * （可堆叠 / 有耐久未损 / 有耐久已损）；本能力不关心损伤，只把**全表的默认上限**铺开，
 * 两者互补。
 *
 * ## 单元格式
 *
 * 物品的资源位置，例如 `minecraft:bucket`。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

/// 全部物品 ID，按字典序排序（两端口径一致，便于定位差异）。
const std::vector<std::string>& itemIds()
{
    static const std::vector<std::string> s_ids = []() {
        std::vector<std::string> ids;
        ItemRegistry::instance().forEachItem([&ids](Item& item) {
            ids.push_back(item.itemLocation().toString());
        });
        std::sort(ids.begin(), ids.end());
        return ids;
    }();
    return s_ids;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const std::string& id : itemIds()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(unit));
    if (item == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    // 用空白的 ItemStack 取默认上限；带组件覆盖的栈由 item_stackable 覆盖。
    const ItemStack stack(*item, 1);

    return json{{"unit", unit}, {"maxStackSize", stack.getMaxStackSize()}, {"damageable", stack.isDamageable()}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_max_stack_size", &writeList, &runLine)

} // namespace mc::diff
