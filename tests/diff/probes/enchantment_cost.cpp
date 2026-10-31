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
 * @file enchantment_cost.cpp
 * @brief 差分能力：附魔的等级与代价曲线。
 *
 * `list`：全部附魔 ID（排序）
 * `run` ：{ "enchantment": "minecraft:sharpness" }
 *         → { "maxLevel": 5, "anvilCost": 1,
 *             "minCost": [1, 12, 23, 34, 45], "maxCost": [21, 32, 43, 54, 65] }
 *
 * 比的是每组等级下的**附魔台等级代价**（`minCost` / `maxCost`）与铁砧代价。
 * 原版这两条曲线来自数据包的
 * `min_cost` / `max_cost`：`{ "base": N, "per_level_above_first": M }`，
 * 即第 L 级（从 1 起）的代价是 `N + M * (L - 1)`。
 *
 * 附魔台"刷不刷得出"、铁砧"能不能合"全由这几个数决定，写错一处就会让某些附魔
 * 在特定等级下永远刷不出来，且不会报错。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureEnchantments()
{
    if (!item::enchant::EnchantmentRegistry::isInitialized()) {
        item::enchant::EnchantmentRegistry::initialize();
    }
}

void writeList(FILE* out)
{
    ensureEnchantments();
    std::vector<std::string> names;
    for (const auto& [id, enchantment] : item::enchant::EnchantmentRegistry::all()) {
        names.push_back(id);
    }
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEnchantments();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("enchantment").get<std::string>();

    const item::enchant::Enchantment* enchantment = item::enchant::EnchantmentRegistry::get(requested);
    if (enchantment == nullptr) {
        return json{{"enchantment", requested}, {"missing", true}}.dump();
    }

    const i32 maxLevel = enchantment->maxLevel();
    json minCost = json::array();
    json maxCost = json::array();
    // 原版等级从 1 开始，按 1..maxLevel 逐级求值。
    for (i32 level = 1; level <= maxLevel; ++level) {
        minCost.push_back(enchantment->getMinCost(level));
        maxCost.push_back(enchantment->getMaxCost(level));
    }

    return json{{"enchantment", requested},
        {"maxLevel", maxLevel},
        {"anvilCost", enchantment->anvilCost()},
        {"minCost", std::move(minCost)},
        {"maxCost", std::move(maxCost)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_cost", &writeList, &runLine)

} // namespace mc::diff
