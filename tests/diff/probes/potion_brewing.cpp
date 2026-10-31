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
 * @file potion_brewing.cpp
 * @brief 差分能力：酿造配方（输入药水 + 试剂 → 输出药水）。
 *
 * `list`：不枚举（输入由编排层按「药水 × 试剂」展开，两侧读同一份输入）
 * `run` ：{ "unit": "...", "potion": "minecraft:swiftness", "reagent": "minecraft:sugar" }
 *         → { "isReagent": bool, "canBrew": bool, "output": "minecraft:swiftness" | null }
 *
 * 酿造配方是纯数据表：原版 `PotionBrewing.bootstrap` 注册一组 `Mix`，Cubium 在
 * `PotionBrewing::initialize` 里注册同一组。这里比对「该试剂是不是酿造材料」「这组输入能否
 * 酿造」「酿造后的输出药水」。不涉及世界、不消耗物品。
 */
#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionBrewing.hpp"
#include "common/item/potion/PotionRegistry.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
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
        mc::potion::Potions::initialize();
        mc::potion::PotionBrewing::initialize();
        s_initialized = true;
    }
}

void writeList(FILE* /*out*/) {}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);

    const std::string potionId = input.at("potion").get<std::string>();
    const std::string reagentId = input.at("reagent").get<std::string>();

    const mc::potion::Potion* potion = mc::potion::PotionRegistry::instance().getPotion(ResourceLocation(potionId));
    const Item* reagent = ItemRegistry::instance().getItem(ResourceLocation(reagentId));
    if (potion == nullptr || reagent == nullptr) {
        return json{{"unit", input.at("unit").get<std::string>()}, {"missing", true}}.dump();
    }

    ItemStack reagentStack(*reagent, 1);
    ItemStack potionStack = mc::potion::PotionUtils::createPotionItem(potion);

    const bool isReagent = mc::potion::PotionBrewing::isReagent(reagentStack);
    const bool canBrew = mc::potion::PotionBrewing::canBrew(potionStack, reagentStack);

    json output = nullptr;
    if (canBrew) {
        const ItemStack brewed = mc::potion::PotionBrewing::brew(potionStack, reagentStack);
        const mc::potion::Potion* result = mc::potion::PotionUtils::getPotion(brewed);
        output = result != nullptr ? json(result->id().toString()) : json(nullptr);
    }

    return json{{"unit", input.at("unit").get<std::string>()},
        {"isReagent", isReagent},
        {"canBrew", canBrew},
        {"output", std::move(output)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("potion_brewing", &writeList, &runLine)

} // namespace mc::diff
