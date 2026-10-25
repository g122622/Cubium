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
 * @file enchantment_table.cpp
 * @brief 差分能力：附魔注册表与等级代价。
 *
 * `list`：全部附魔 ID
 * `run` ：{ "enchantment": "minecraft:sharpness" }
 *         → minLevel / maxLevel，以及这两端等级的最小/最大附魔代价
 *           （vanilla 的 getMinCost / getMaxCost）
 *
 * 这两个代价公式决定附魔台每格显示的等级要求，错一位就会让玩家在正确的等级下
 * 看不到某个附魔。它同时也是 `docs/数据驱动差异.md` 里「附魔仍是 1.16.5 风格」
 * 这类硬编码表最直接的验收指标。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/enchantment/Enchantment.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using item::enchant::Enchantment;
using item::enchant::EnchantmentRegistry;

void ensureEnchantments()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        EnchantmentRegistry::initialize();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureEnchantments();
    std::vector<std::string> names;
    names.reserve(EnchantmentRegistry::all().size());
    for (const auto& [id, enchantment] : EnchantmentRegistry::all()) {
        names.push_back(id);
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureEnchantments();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("enchantment").get<std::string>();

    const Enchantment* enchantment = EnchantmentRegistry::get(requested);
    if (enchantment == nullptr) {
        return json{{"enchantment", requested}, {"missing", true}}.dump();
    }

    const i32 minLevel = enchantment->minLevel();
    const i32 maxLevel = enchantment->maxLevel();

    // 附魔台按 getMinCost / getMaxCost 决定每格能给的等级。原版 oracle 只给两端
    // （minLevel、maxLevel）的取值：代价公式随等级线性增长，端点一致即全等级一致。
    json costs = json::array();
    for (const i32 level : {minLevel, maxLevel}) {
        costs.push_back({{"level", level},
            {"minCost", enchantment->getMinCost(level)},
            {"maxCost", enchantment->getMaxCost(level)}});
    }

    // 权重与铁砧代价：附魔台的加权随机与铁砧合并的扣级都直接读这两个值。
    return json{{"enchantment", requested}, {"minLevel", minLevel}, {"maxLevel", maxLevel},
        {"weight", enchantment->rarityWeight()}, {"anvilCost", enchantment->anvilCost()}, {"costs", std::move(costs)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_table", &writeList, &runLine)

} // namespace mc::diff