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

#pragma once

#include "common/core/Types.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace item {
namespace component {

/**
 * @brief 数据组件类型标识（1.21.11 DataComponentType）
 *
 * 每个组件类型有一个整数 typeId（网络 wire 中的 VarInt，= Java
 * DATA_COMPONENT_TYPE 注册表 id）和一个资源位置名（NBT patch 的键，如
 * "minecraft:damage"）。typeId 严格对齐 Java 1.21.11 DataComponents.register
 * 注册顺序由 assets/data/protocol-1.21.11.json 的 SlotComponentType 校验。
 *
 * 枚举值即注册表 id（0..103），名字由资源位置按 蛇形→驼峰 转换（villager/variant → VillagerVariant）。
 * 尚未落地载荷解码的组件仍以占位类型表示，可经 set_components/copy_components 以原始 NBT 承载。
 */
enum class DataComponentType : i32 {
    CustomData = 0,
    MaxStackSize = 1,
    MaxDamage = 2,
    Damage = 3,
    Unbreakable = 4,
    UseEffects = 5,
    CustomName = 6,
    MinimumAttackCharge = 7,
    DamageType = 8,
    ItemName = 9,
    ItemModel = 10,
    Lore = 11,
    Rarity = 12,
    Enchantments = 13,
    CanPlaceOn = 14,
    CanBreak = 15,
    AttributeModifiers = 16,
    CustomModelData = 17,
    TooltipDisplay = 18,
    RepairCost = 19,
    CreativeSlotLock = 20,
    EnchantmentGlintOverride = 21,
    IntangibleProjectile = 22,
    Food = 23,
    Consumable = 24,
    UseRemainder = 25,
    UseCooldown = 26,
    DamageResistant = 27,
    Tool = 28,
    Weapon = 29,
    AttackRange = 30,
    Enchantable = 31,
    Equippable = 32,
    Repairable = 33,
    Glider = 34,
    TooltipStyle = 35,
    DeathProtection = 36,
    BlocksAttacks = 37,
    PiercingWeapon = 38,
    KineticWeapon = 39,
    SwingAnimation = 40,
    StoredEnchantments = 41,
    DyedColor = 42,
    MapColor = 43,
    MapId = 44,
    MapDecorations = 45,
    MapPostProcessing = 46,
    ChargedProjectiles = 47,
    BundleContents = 48,
    PotionContents = 49,
    PotionDurationScale = 50,
    SuspiciousStewEffects = 51,
    WritableBookContent = 52,
    WrittenBookContent = 53,
    Trim = 54,
    DebugStickState = 55,
    EntityData = 56,
    BucketEntityData = 57,
    BlockEntityData = 58,
    Instrument = 59,
    ProvidesTrimMaterial = 60,
    OminousBottleAmplifier = 61,
    JukeboxPlayable = 62,
    ProvidesBannerPatterns = 63,
    Recipes = 64,
    LodestoneTracker = 65,
    FireworkExplosion = 66,
    Fireworks = 67,
    Profile = 68,
    NoteBlockSound = 69,
    BannerPatterns = 70,
    BaseColor = 71,
    PotDecorations = 72,
    Container = 73,
    BlockState = 74,
    Bees = 75,
    Lock = 76,
    ContainerLoot = 77,
    BreakSound = 78,
    VillagerVariant = 79,
    WolfVariant = 80,
    WolfSoundVariant = 81,
    WolfCollar = 82,
    FoxVariant = 83,
    SalmonSize = 84,
    ParrotVariant = 85,
    TropicalFishPattern = 86,
    TropicalFishBaseColor = 87,
    TropicalFishPatternColor = 88,
    MooshroomVariant = 89,
    RabbitVariant = 90,
    PigVariant = 91,
    CowVariant = 92,
    ChickenVariant = 93,
    ZombieNautilusVariant = 94,
    FrogVariant = 95,
    HorseVariant = 96,
    PaintingVariant = 97,
    LlamaVariant = 98,
    AxolotlVariant = 99,
    CatVariant = 100,
    CatCollar = 101,
    SheepColor = 102,
    ShulkerColor = 103,
};

/**
 * @brief 组件类型名（带 "minecraft:" 前缀的资源位置）
 *
 * 用于 NBT patch 的键名。与 DataComponentType 一一对应，仅覆盖本项目落地的子集。
 * 未落地的类型返回 std::nullopt。
 */
[[nodiscard]] std::optional<std::string_view> componentName(DataComponentType type) noexcept;

/**
 * @brief 由组件资源位置名查 typeId（NBT patch 的 "!" 键反向解析时用）
 *
 * 仅覆盖本项目落地的子集；未知名返回 std::nullopt。
 */
[[nodiscard]] std::optional<DataComponentType> componentTypeByName(std::string_view name) noexcept;

/**
 * @brief 由 typeId 查类型（wire codec 读 typeId 后分发时用）
 *
 * 越界/未落地返回 std::nullopt。
 */
[[nodiscard]] std::optional<DataComponentType> componentTypeById(i32 typeId) noexcept;

/**
 * @brief 组件 typeId（= static_cast<i32>(type)，集中出口便于将来调整）
 */
[[nodiscard]] constexpr i32 componentTypeId(DataComponentType type) noexcept
{
    return static_cast<i32>(type);
}

} // namespace component
} // namespace item
} // namespace mc
