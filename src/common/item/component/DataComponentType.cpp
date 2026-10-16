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

#include "common/item/component/DataComponentType.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <optional>
#include <string_view>

namespace mc {
namespace item {
namespace component {

namespace {

struct _Entry {
    DataComponentType type;
    const char* name;
};

// 本项目已落地的组件子集。typeId 与 Java 1.21.11 一致（见枚举注释）。
constexpr std::array _entries{
    _Entry{DataComponentType::CustomData, "minecraft:custom_data"},
    _Entry{DataComponentType::MaxStackSize, "minecraft:max_stack_size"},
    _Entry{DataComponentType::MaxDamage, "minecraft:max_damage"},
    _Entry{DataComponentType::Damage, "minecraft:damage"},
    _Entry{DataComponentType::Unbreakable, "minecraft:unbreakable"},
    _Entry{DataComponentType::CustomName, "minecraft:custom_name"},
    _Entry{DataComponentType::ItemName, "minecraft:item_name"},
    _Entry{DataComponentType::ItemModel, "minecraft:item_model"},
    _Entry{DataComponentType::Lore, "minecraft:lore"},
    _Entry{DataComponentType::Rarity, "minecraft:rarity"},
    _Entry{DataComponentType::Enchantments, "minecraft:enchantments"},
    _Entry{DataComponentType::CanPlaceOn, "minecraft:can_place_on"},
    _Entry{DataComponentType::CanBreak, "minecraft:can_break"},
    _Entry{DataComponentType::RepairCost, "minecraft:repair_cost"},
    _Entry{DataComponentType::AttackRange, "minecraft:attack_range"},
    _Entry{DataComponentType::Enchantable, "minecraft:enchantable"},
    _Entry{DataComponentType::BundleContents, "minecraft:bundle_contents"},
    _Entry{DataComponentType::WritableBookContent, "minecraft:writable_book_content"},
    _Entry{DataComponentType::WrittenBookContent, "minecraft:written_book_content"},
    _Entry{DataComponentType::StoredEnchantments, "minecraft:stored_enchantments"},
    _Entry{DataComponentType::BannerPatterns, "minecraft:banner_patterns"},
    _Entry{DataComponentType::PotionContents, "minecraft:potion_contents"},
    _Entry{DataComponentType::FireworkExplosion, "minecraft:firework_explosion"},
    _Entry{DataComponentType::Fireworks, "minecraft:fireworks"},
};

} // namespace

std::optional<std::string_view> componentName(DataComponentType type) noexcept
{
    for (const auto& e : _entries) {
        if (e.type == type) {
            return std::string_view(e.name);
        }
    }
    return std::nullopt;
}

std::optional<DataComponentType> componentTypeByName(std::string_view name) noexcept
{
    for (const auto& e : _entries) {
        if (e.name == name) {
            return e.type;
        }
    }
    return std::nullopt;
}

std::optional<DataComponentType> componentTypeById(i32 typeId) noexcept
{
    for (const auto& e : _entries) {
        if (static_cast<i32>(e.type) == typeId) {
            return e.type;
        }
    }
    return std::nullopt;
}

} // namespace component
} // namespace item
} // namespace mc
