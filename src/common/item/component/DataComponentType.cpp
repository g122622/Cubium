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

// 全部内置组件：顺序与 Java 1.21.11 DataComponents.register 声明顺序严格一致。
constexpr std::array _entries{
    _Entry{DataComponentType::CustomData, "minecraft:custom_data"},
    _Entry{DataComponentType::MaxStackSize, "minecraft:max_stack_size"},
    _Entry{DataComponentType::MaxDamage, "minecraft:max_damage"},
    _Entry{DataComponentType::Damage, "minecraft:damage"},
    _Entry{DataComponentType::Unbreakable, "minecraft:unbreakable"},
    _Entry{DataComponentType::UseEffects, "minecraft:use_effects"},
    _Entry{DataComponentType::CustomName, "minecraft:custom_name"},
    _Entry{DataComponentType::MinimumAttackCharge, "minecraft:minimum_attack_charge"},
    _Entry{DataComponentType::DamageType, "minecraft:damage_type"},
    _Entry{DataComponentType::ItemName, "minecraft:item_name"},
    _Entry{DataComponentType::ItemModel, "minecraft:item_model"},
    _Entry{DataComponentType::Lore, "minecraft:lore"},
    _Entry{DataComponentType::Rarity, "minecraft:rarity"},
    _Entry{DataComponentType::Enchantments, "minecraft:enchantments"},
    _Entry{DataComponentType::CanPlaceOn, "minecraft:can_place_on"},
    _Entry{DataComponentType::CanBreak, "minecraft:can_break"},
    _Entry{DataComponentType::AttributeModifiers, "minecraft:attribute_modifiers"},
    _Entry{DataComponentType::CustomModelData, "minecraft:custom_model_data"},
    _Entry{DataComponentType::TooltipDisplay, "minecraft:tooltip_display"},
    _Entry{DataComponentType::RepairCost, "minecraft:repair_cost"},
    _Entry{DataComponentType::CreativeSlotLock, "minecraft:creative_slot_lock"},
    _Entry{DataComponentType::EnchantmentGlintOverride, "minecraft:enchantment_glint_override"},
    _Entry{DataComponentType::IntangibleProjectile, "minecraft:intangible_projectile"},
    _Entry{DataComponentType::Food, "minecraft:food"},
    _Entry{DataComponentType::Consumable, "minecraft:consumable"},
    _Entry{DataComponentType::UseRemainder, "minecraft:use_remainder"},
    _Entry{DataComponentType::UseCooldown, "minecraft:use_cooldown"},
    _Entry{DataComponentType::DamageResistant, "minecraft:damage_resistant"},
    _Entry{DataComponentType::Tool, "minecraft:tool"},
    _Entry{DataComponentType::Weapon, "minecraft:weapon"},
    _Entry{DataComponentType::AttackRange, "minecraft:attack_range"},
    _Entry{DataComponentType::Enchantable, "minecraft:enchantable"},
    _Entry{DataComponentType::Equippable, "minecraft:equippable"},
    _Entry{DataComponentType::Repairable, "minecraft:repairable"},
    _Entry{DataComponentType::Glider, "minecraft:glider"},
    _Entry{DataComponentType::TooltipStyle, "minecraft:tooltip_style"},
    _Entry{DataComponentType::DeathProtection, "minecraft:death_protection"},
    _Entry{DataComponentType::BlocksAttacks, "minecraft:blocks_attacks"},
    _Entry{DataComponentType::PiercingWeapon, "minecraft:piercing_weapon"},
    _Entry{DataComponentType::KineticWeapon, "minecraft:kinetic_weapon"},
    _Entry{DataComponentType::SwingAnimation, "minecraft:swing_animation"},
    _Entry{DataComponentType::StoredEnchantments, "minecraft:stored_enchantments"},
    _Entry{DataComponentType::DyedColor, "minecraft:dyed_color"},
    _Entry{DataComponentType::MapColor, "minecraft:map_color"},
    _Entry{DataComponentType::MapId, "minecraft:map_id"},
    _Entry{DataComponentType::MapDecorations, "minecraft:map_decorations"},
    _Entry{DataComponentType::MapPostProcessing, "minecraft:map_post_processing"},
    _Entry{DataComponentType::ChargedProjectiles, "minecraft:charged_projectiles"},
    _Entry{DataComponentType::BundleContents, "minecraft:bundle_contents"},
    _Entry{DataComponentType::PotionContents, "minecraft:potion_contents"},
    _Entry{DataComponentType::PotionDurationScale, "minecraft:potion_duration_scale"},
    _Entry{DataComponentType::SuspiciousStewEffects, "minecraft:suspicious_stew_effects"},
    _Entry{DataComponentType::WritableBookContent, "minecraft:writable_book_content"},
    _Entry{DataComponentType::WrittenBookContent, "minecraft:written_book_content"},
    _Entry{DataComponentType::Trim, "minecraft:trim"},
    _Entry{DataComponentType::DebugStickState, "minecraft:debug_stick_state"},
    _Entry{DataComponentType::EntityData, "minecraft:entity_data"},
    _Entry{DataComponentType::BucketEntityData, "minecraft:bucket_entity_data"},
    _Entry{DataComponentType::BlockEntityData, "minecraft:block_entity_data"},
    _Entry{DataComponentType::Instrument, "minecraft:instrument"},
    _Entry{DataComponentType::ProvidesTrimMaterial, "minecraft:provides_trim_material"},
    _Entry{DataComponentType::OminousBottleAmplifier, "minecraft:ominous_bottle_amplifier"},
    _Entry{DataComponentType::JukeboxPlayable, "minecraft:jukebox_playable"},
    _Entry{DataComponentType::ProvidesBannerPatterns, "minecraft:provides_banner_patterns"},
    _Entry{DataComponentType::Recipes, "minecraft:recipes"},
    _Entry{DataComponentType::LodestoneTracker, "minecraft:lodestone_tracker"},
    _Entry{DataComponentType::FireworkExplosion, "minecraft:firework_explosion"},
    _Entry{DataComponentType::Fireworks, "minecraft:fireworks"},
    _Entry{DataComponentType::Profile, "minecraft:profile"},
    _Entry{DataComponentType::NoteBlockSound, "minecraft:note_block_sound"},
    _Entry{DataComponentType::BannerPatterns, "minecraft:banner_patterns"},
    _Entry{DataComponentType::BaseColor, "minecraft:base_color"},
    _Entry{DataComponentType::PotDecorations, "minecraft:pot_decorations"},
    _Entry{DataComponentType::Container, "minecraft:container"},
    _Entry{DataComponentType::BlockState, "minecraft:block_state"},
    _Entry{DataComponentType::Bees, "minecraft:bees"},
    _Entry{DataComponentType::Lock, "minecraft:lock"},
    _Entry{DataComponentType::ContainerLoot, "minecraft:container_loot"},
    _Entry{DataComponentType::BreakSound, "minecraft:break_sound"},
    _Entry{DataComponentType::VillagerVariant, "minecraft:villager/variant"},
    _Entry{DataComponentType::WolfVariant, "minecraft:wolf/variant"},
    _Entry{DataComponentType::WolfSoundVariant, "minecraft:wolf/sound_variant"},
    _Entry{DataComponentType::WolfCollar, "minecraft:wolf/collar"},
    _Entry{DataComponentType::FoxVariant, "minecraft:fox/variant"},
    _Entry{DataComponentType::SalmonSize, "minecraft:salmon/size"},
    _Entry{DataComponentType::ParrotVariant, "minecraft:parrot/variant"},
    _Entry{DataComponentType::TropicalFishPattern, "minecraft:tropical_fish/pattern"},
    _Entry{DataComponentType::TropicalFishBaseColor, "minecraft:tropical_fish/base_color"},
    _Entry{DataComponentType::TropicalFishPatternColor, "minecraft:tropical_fish/pattern_color"},
    _Entry{DataComponentType::MooshroomVariant, "minecraft:mooshroom/variant"},
    _Entry{DataComponentType::RabbitVariant, "minecraft:rabbit/variant"},
    _Entry{DataComponentType::PigVariant, "minecraft:pig/variant"},
    _Entry{DataComponentType::CowVariant, "minecraft:cow/variant"},
    _Entry{DataComponentType::ChickenVariant, "minecraft:chicken/variant"},
    _Entry{DataComponentType::ZombieNautilusVariant, "minecraft:zombie_nautilus/variant"},
    _Entry{DataComponentType::FrogVariant, "minecraft:frog/variant"},
    _Entry{DataComponentType::HorseVariant, "minecraft:horse/variant"},
    _Entry{DataComponentType::PaintingVariant, "minecraft:painting/variant"},
    _Entry{DataComponentType::LlamaVariant, "minecraft:llama/variant"},
    _Entry{DataComponentType::AxolotlVariant, "minecraft:axolotl/variant"},
    _Entry{DataComponentType::CatVariant, "minecraft:cat/variant"},
    _Entry{DataComponentType::CatCollar, "minecraft:cat/collar"},
    _Entry{DataComponentType::SheepColor, "minecraft:sheep/color"},
    _Entry{DataComponentType::ShulkerColor, "minecraft:shulker/color"},
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
