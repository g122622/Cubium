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
 * @file armor_material.cpp
 * @brief 差分能力：护甲材质的耐久系数、护甲值、附魔能力、韧性与击退抗性。
 *
 * `list`：全部材质名（皮革、铜、锁链、铁、金、钻石、海龟壳、下界合金、犰狳鳞甲）
 * `run` ：{ "material": "diamond" }
 *         → 五个槽位的耐久与护甲值、附魔能力、韧性、击退抗性
 *
 * 这些数决定了全套护甲的最终护甲值（进而决定减伤）、每件盔甲的耐久，以及附魔台
 * 给盔甲的等级。它们都写在材质类里，且「耐久系数 × 槽位倍率」是两段相乘，手抄时
 * 最容易把系数当成最终耐久。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/armor/ArmorMaterial.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace item::armor;

struct MaterialEntry {
    const char* name;
    const ArmorMaterial* material;
};

/// 材质清单：名字与 harness 单元一致（小写，不带命名空间）。
const MaterialEntry kMaterials[] = {
    {"leather", &ArmorMaterials::LEATHER},
    {"copper", &ArmorMaterials::COPPER},
    {"chainmail", &ArmorMaterials::CHAIN},
    {"iron", &ArmorMaterials::IRON},
    {"gold", &ArmorMaterials::GOLD},
    {"diamond", &ArmorMaterials::DIAMOND},
    {"turtle_scute", &ArmorMaterials::TURTLE},
    {"netherite", &ArmorMaterials::NETHERITE},
    {"armadillo_scute", &ArmorMaterials::ARMADILLO_SCUTE},
};

/// 槽位名：与原版 `ArmorType` 的枚举名（小写）一致。
struct SlotEntry {
    ArmorSlot slot;
    const char* name;
};

constexpr SlotEntry kSlots[] = {
    {ArmorSlot::Feet, "boots"},
    {ArmorSlot::Legs, "leggings"},
    {ArmorSlot::Chest, "chestplate"},
    {ArmorSlot::Head, "helmet"},
    {ArmorSlot::Body, "body"},
};

void ensureMaterials()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        ArmorMaterials::initialize();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureMaterials();
    for (const auto& entry : kMaterials) {
        std::fprintf(out, "%s\n", entry.name);
    }
}

const ArmorMaterial* findMaterial(std::string_view name)
{
    for (const auto& entry : kMaterials) {
        if (name == entry.name) {
            return entry.material;
        }
    }
    return nullptr;
}

std::string runLine(std::string_view inputLine)
{
    ensureMaterials();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("material").get<std::string>();

    const ArmorMaterial* material = findMaterial(requested);
    if (material == nullptr) {
        return json{{"material", requested}, {"missing", true}}.dump();
    }

    // 槽位是对象：每个槽位同时报耐久（已含槽位倍率）与护甲值，两者都是玩家直接感知的量。
    json slots = json::object();
    for (const SlotEntry& slot : kSlots) {
        slots[slot.name] = json{{"durability", material->getDurability(slot.slot)},
            {"defense", material->getDefense(slot.slot)}};
    }

    return json{{"material", requested},
        {"enchantability", material->getEnchantability()},
        {"toughness", material->getToughness()},
        {"knockbackResistance", material->getKnockbackResistance()},
        {"slots", std::move(slots)}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("armor_material", &writeList, &runLine)

} // namespace mc::diff