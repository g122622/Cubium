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
 * @file enchantment_effects.cpp
 * @brief 差分能力：附魔在各等级下的效果数值。
 *
 * `list`：全部附魔 ID
 * `run` ：{ "enchantment": "minecraft:protection" }
 *         → 每个等级（1..maxLevel）的:
 *           - damageBonus：等级带来的额外伤害（锋利/亡灵杀手/节肢杀手/穿刺）
 *           - protection：每类伤害的减伤点数（保护类附魔）
 *           - attributes：等级提供的属性修饰符（水下呼吸的氧气、保护系的爆炸击退等）
 *
 * `enchantment_table` 只比等级代价（附魔台每格显示什么），这里比的是**效果本身**：
 * 伤害加成的斜率写错 0.5、保护点数的上限差一级，玩家只会觉得「这套附魔没想象中好用」，
 * 任何按「能不能附上」判定的测试都不会失败。
 *
 * 原版把效果表达成 `LevelBasedValue` 组件，Cubium 用各类的 `getDamageBonus` /
 * `getDamageProtection` / `getAttributeModifiers` 表达；两侧都对同一组等级求值。
 */
#include <algorithm>
#include <array>
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/serialization/EquipmentSlotNames.hpp"
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
        (void)enchantment;
        names.push_back(id);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

/// 保护类附魔的区别对象：伤害类别（对应原版的 damage_type 标签）。
struct ProtectionCategory {
    const char* name;
    u32 flags;
};

constexpr ProtectionCategory kProtectionCategories[] = {
    // 「通用保护」对任何伤害都有效：Cubium 用「掩码为 0」表达，原版用「没有类别条件」表达，
    // 两者是同一件事，因此也报成一个类别（原版侧同样叫 none）。
    {"none", 0},
    {"fire", DamageFlags::FIRE},
    {"fall", DamageFlags::FALL},
    {"explosion", DamageFlags::EXPLOSION},
    {"projectile", DamageFlags::PROJECTILE},
};

/** 运算名：与原版 `AttributeModifier.Operation` 的序列化键一致。 */
const char* operationText(entity::attribute::Operation operation)
{
    switch (operation) {
        case entity::attribute::Operation::Addition:
            return "add_value";
        case entity::attribute::Operation::MultiplyBase:
            return "add_multiplied_base";
        case entity::attribute::Operation::MultiplyTotal:
            return "add_multiplied_total";
    }
    return "unknown";
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

    // 等级是数组下标 +1：level 0 在原版里没有意义（附魔从 1 级开始），这里只报 1..maxLevel。
    json levels = json::array();
    for (i32 level = 1; level <= enchantment->maxLevel(); ++level) {
        // 「对谁生效」由附魔自己的标签条件表达（亡灵杀手只对亡灵……），原版的数值本身与
        // 目标无关，因此这里取 getDamageBonusPerLevel（命中可加成目标时的额外伤害）。
        json entry{{"level", level}, {"damageBonus", enchantment->getDamageBonusPerLevel(level)}};

        // 保护类附魔按伤害类别给减伤点数。类别取 Cubium 自己的 DamageFlags（原版等价物是
        // damage_type 标签），因此这里报「每类伤害各减多少」而不是枚举序号。
        json protection = json::object();
        constexpr auto kCount = sizeof(kProtectionCategories) / sizeof(kProtectionCategories[0]);
        std::array<i32, kCount> protectionPoints{};
        for (size_t index = 0; index < kCount; ++index) {
            protectionPoints[index] = enchantment->getDamageProtection(level, kProtectionCategories[index].flags);
        }
        // Cubium 的保护掩码是前缀匹配：泛保护（掩码 0）对每一类都返回同样的点数，
        // 原版则把泛保护写成「无条件的一条」，只算一次。两者表达的是同一件事，
        // 因此当各类取值与 none 完全相同时只保留 none（其余类别这时不携带额外信息）。
        const bool allEqualNone =
            std::all_of(std::next(protectionPoints.begin()), protectionPoints.end(), [&protectionPoints](i32 value) {
                return value == protectionPoints[0];
            });
        for (size_t index = 0; index < kCount; ++index) {
            protection[kProtectionCategories[index].name] = allEqualNone && index > 0 ? 0 : protectionPoints[index];
        }
        entry["protection"] = std::move(protection);

        json attributes = json::array();
        // 必须先存到局部变量：getAttributeModifiers 返回的是临时对象，直接在 range-for 的
        // 表达式里取 getEntries() 会让迭代器指向一个已经销毁的临时量（表现是两个
        // std::string 字段读出来是空的，数值字段却「碰巧」还正确）。
        const item::ItemAttributeModifiers levelModifiers = enchantment->getAttributeModifiers(level);
        for (const auto& entry : levelModifiers.getEntries()) {
            attributes.push_back(json{{"attribute", entry.attributeName},
                {"id", entry.modifier.id()},
                {"operation", operationText(entry.modifier.operation())},
                {"amount", entry.modifier.amount()},
                {"slot",
                    entity::serialization::EquipmentSlotNames::toName(
                        static_cast<EquipmentSlot>(entry.equipmentSlot))}});
        }
        entry["attributes"] = std::move(attributes);
        levels.push_back(std::move(entry));
    }

    return json{{"enchantment", requested}, {"levels", std::move(levels)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_effects", &writeList, &runLine)

} // namespace mc::diff
