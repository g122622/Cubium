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
 * @file food_properties.cpp
 * @brief 差分能力：食物属性（`FoodProperties`）。
 *
 * `list`：全部**带食物属性**的物品 ID（排序）
 * `run` ：{ "item": "minecraft:golden_apple" }
 *         → { "food": true, "nutrition": 4, "saturation": 0.3, "canAlwaysEat": true }
 *
 * 只比较两侧语义严格相同的三项：饥饿值、饱和度修正、能否饱腹后继续吃。原版这三项在
 * `DataComponents.FOOD`（`FoodProperties` record）里；食用时长、动画、附加效果等属于
 * `Consumable` 组件与数据包，与 Cubium 的 `Food` 字段划分不同，**不纳入本能力**。
 *
 * 饱和度字段是 f32：原版 `FoodProperties.saturation` 是 float。食用时实际恢复的饱和值
 * = `nutrition * saturation * 2`，那个换算属于 `FoodData.eat`，本能力不做。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/food/Food.hpp"
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

/**
 * @brief 把 f32 按 6 位有效数字格式化。
 *
 * f32 的 0.3f 若交给 nlohmann 直接输出，会被精确展开成 0.30000001192092896；
 * 而 Java 侧同一个 `float` 经 Gson 输出是 0.3。两者是同一个数值，差别只在打印精度，
 * 用 6 位有效数字与 Java 的 `Float.toString` 口径对齐，才能逐字节比较。
 */
std::string formatModifier(f32 value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<f64>(value));
    return buffer;
}

void writeList(FILE* out)
{
    ensureItems();
    std::vector<std::string> names;
    ItemRegistry::instance().forEachItem([&](Item& item) {
        if (item.foodProperties() != nullptr) {
            names.push_back(item.itemLocation().toString());
        }
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

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation(requested));
    if (item == nullptr) {
        return json{{"item", requested}, {"missing", true}}.dump();
    }

    const item::food::Food* food = item->foodProperties();
    if (food == nullptr) {
        // 不是食物：两侧都应如此。显式给出 food=false 而不是 missing，
        // 以区分「物品不存在」与「物品存在但不是食物」。
        return json{{"item", requested}, {"food", false}}.dump();
    }

    // 比较 `saturationModifier`（**修正系数**本身），而不是换算后的饱和值。
    //
    // 两侧把它放在了不同的字段语义里：原版 `FoodProperties.saturation` 存的是「已经
    // 换算过的饱和值」，Cubium `Food::getSaturationModifier()` 存的是系数（食用时由
    // `FoodStats::addStats` 乘 `food × 2`，与原版 `FoodConstants.saturationByModifier`
    // 一致）。比起在探针侧正算 `nutrition × modifier × 2`（会与两侧写死的 f32 常量差
    // 最后一个 ulp），直接比系数更稳。
    //
    // 系数按 6 位有效数字输出：f32 的 0.3f 若由 nlohmann 精确展开会变成
    // 0.30000001192092896，而 Java 侧 `float` 经 Gson 输出是 0.3。两者是同一个 f32，
    // 差别只在打印精度。用 6 位有效数字与 Java 的 `Float.toString` 口径对齐。

    return json{{"item", requested},
        {"food", true},
        {"nutrition", food->getHunger()},
        {"saturationModifier", formatModifier(food->getSaturationModifier())},
        {"canAlwaysEat", food->canAlwaysEat()}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("food_properties", &writeList, &runLine)

} // namespace mc::diff
