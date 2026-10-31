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
 * @file potion.cpp
 * @brief 差分能力：药水定义与酿造配方。
 *
 * `list`：全部药水 ID（排序）
 * `run` ：{ "potion": "minecraft:swiftness" }
 *         → { "baseName": "swiftness", "effects": [...], "format": [...] }
 *
 * 两层语义：
 *   1. **药水定义**：基础名与效果列表（效果 ID 排序后比较——效果列表的顺序在玩法上没有
 *      语义，原版 `Potion` 也只是 `List<MobEffectInstance>`）；
 *   2. **显示名**：`getNamePrefixed` 的三种前缀（空 / "Splash " / "Lingering "），
 *      它会按药水是否含瞬时效果决定用 `of` 还是 `of` 的变体，是易错的字符串逻辑。
 *
 * 全部是纯查表与字符串拼接，不触碰世界。酿造配方另由 `potion_brewing` 能力覆盖。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/item/potion/PotionRegistry.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensurePotions()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        mc::potion::Potions::initialize();
        s_initialized = true;
    }
}

/** 一条效果的可比较摘要。 */
json effectJson(const entity::effect::EffectInstance& effect)
{
    return json{{"id", entity::effect::getEffectResourceLocation(effect.type()).toString()},
        {"duration", effect.duration()},
        {"amplifier", effect.amplifier()},
        {"ambient", effect.isAmbient()},
        {"visible", effect.isVisible()},
        {"showIcon", effect.showIcon()}};
}

void writeList(FILE* out)
{
    ensurePotions();
    std::vector<std::string> names;
    for (const auto& [id, potion] : mc::potion::PotionRegistry::instance().getAllPotions()) {
        names.push_back(id.toString());
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensurePotions();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("potion").get<std::string>();

    const mc::potion::Potion* potion = mc::potion::PotionRegistry::instance().getPotion(ResourceLocation(requested));
    if (potion == nullptr) {
        return json{{"potion", requested}, {"missing", true}}.dump();
    }

    // 效果按 ID 排序：原版存的是 List，顺序无玩法语义，集合语义下才可比。
    json effects = json::array();
    for (const auto& effect : potion->effects()) {
        effects.push_back(effectJson(effect));
    }
    std::sort(effects.begin(), effects.end(), [](const json& lhs, const json& rhs) {
        return lhs.at("id").get<std::string>() < rhs.at("id").get<std::string>();
    });

    return json{{"potion", requested},
        {"baseName", potion->baseName()},
        {"effects", std::move(effects)},
        {"hasInstant", potion->hasInstantEffect()},
        // 三种前缀下的显示名：空（普通药水）、Splash、Lingering。
        {"names",
            json::array({potion->getNamePrefixed(""),
                potion->getNamePrefixed("Splash "),
                potion->getNamePrefixed("Lingering ")})}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("potion", &writeList, &runLine)

} // namespace mc::diff
