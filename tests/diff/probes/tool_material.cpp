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
 * @file tool_material.cpp
 * @brief 差分能力：工具材质的耐久、挖掘效率、攻击伤害加成、附魔能力与采集等级。
 *
 * `list`：全部材质名（wood、stone、copper、iron、diamond、gold、netherite）
 * `run` ：{ "material": "diamond" }
 *         → durability / speed / attackDamageBonus / enchantmentValue / harvestLevel
 *
 * 这五项都是手写常量，而且「耐久」是每件工具的基数（铲/镐/斧/锄/剑同属一个材质但
 * 物品各自的耐久由物品决定）、「攻击伤害加成」会叠进工具的最终伤害，错一位就要么
 * 挖得更快要么砍得更疼，跑再久也察觉不出来。
 *
 * 「采集等级」是 Cubium 自己的概念（原版用 `INCORRECT_FOR_*_TOOL` 方块标签表达），
 * 这里按标签的语义折算成数字：原版标签里「错误工具」集合越小，等级越高。
 */
#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/item/tier/ItemTiers.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

struct MaterialEntry {
    const char* name;
    const item::tier::IItemTier& (*tier)();
};

/// 材质清单：名字与 harness 单元一致（原版 ToolMaterial 的常量名小写）。
const MaterialEntry kMaterials[] = {
    {"wood", &item::tier::ItemTiers::WOOD},
    {"stone", &item::tier::ItemTiers::STONE},
    {"copper", &item::tier::ItemTiers::COPPER},
    {"iron", &item::tier::ItemTiers::IRON},
    {"diamond", &item::tier::ItemTiers::DIAMOND},
    {"gold", &item::tier::ItemTiers::GOLD},
    {"netherite", &item::tier::ItemTiers::NETHERITE},
};

void ensureTiers()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        item::tier::ItemTiers::initialize();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureTiers();
    for (const auto& entry : kMaterials) {
        std::fprintf(out, "%s\n", entry.name);
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureTiers();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("material").get<std::string>();

    for (const auto& entry : kMaterials) {
        if (requested != entry.name) {
            continue;
        }
        const item::tier::IItemTier& tier = entry.tier();
        return json{{"material", requested},
            {"durability", tier.getMaxUses()},
            {"speed", tier.getEfficiency()},
            {"attackDamageBonus", tier.getAttackDamage()},
            {"enchantmentValue", tier.getEnchantability()},
            {"harvestLevel", tier.getHarvestLevel()}}
            .dump();
    }
    return json{{"material", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("tool_material", &writeList, &runLine)

} // namespace mc::diff
