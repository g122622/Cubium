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
 */

#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/core/EntityRegistry.hpp"

namespace mc {
namespace entity {
namespace VanillaEntityTypeKeys {

// ============================================================================
// 被动生物
// ============================================================================

// 普通动物
const EntityType* PIG = nullptr;
const EntityType* COW = nullptr;
const EntityType* SHEEP = nullptr;
const EntityType* CHICKEN = nullptr;
const EntityType* RABBIT = nullptr;
const EntityType* MOOSHROOM = nullptr;

// 可驯服动物
const EntityType* WOLF = nullptr;
const EntityType* CAT = nullptr;
const EntityType* OCELOT = nullptr;
const EntityType* PARROT = nullptr;

// 特殊动物
const EntityType* FOX = nullptr;
const EntityType* PANDA = nullptr;
const EntityType* POLAR_BEAR = nullptr;
const EntityType* TURTLE = nullptr;
const EntityType* BEE = nullptr;
const EntityType* STRIDER = nullptr;
const EntityType* SNIFFER = nullptr;

// 马类
const EntityType* HORSE = nullptr;
const EntityType* DONKEY = nullptr;
const EntityType* MULE = nullptr;
const EntityType* LLAMA = nullptr;
const EntityType* TRADER_LLAMA = nullptr;
const EntityType* SKELETON_HORSE = nullptr;
const EntityType* ZOMBIE_HORSE = nullptr;

// 水生生物
const EntityType* COD = nullptr;
const EntityType* SALMON = nullptr;
const EntityType* PUFFERFISH = nullptr;
const EntityType* TROPICAL_FISH = nullptr;
const EntityType* SQUID = nullptr;
const EntityType* GLOW_SQUID = nullptr;
const EntityType* DOLPHIN = nullptr;
const EntityType* AXOLOTL = nullptr;
const EntityType* NAUTILUS = nullptr;
const EntityType* ZOMBIE_NAUTILUS = nullptr;

// 环境生物
const EntityType* BAT = nullptr;

// 傀儡
const EntityType* IRON_GOLEM = nullptr;
const EntityType* SNOW_GOLEM = nullptr;
const EntityType* COPPER_GOLEM = nullptr;

// ============================================================================
// 敌对生物
// ============================================================================

// 基础怪物
const EntityType* ZOMBIE = nullptr;
const EntityType* SKELETON = nullptr;
const EntityType* CREEPER = nullptr;
const EntityType* SPIDER = nullptr;
const EntityType* ENDERMAN = nullptr;
const EntityType* BLAZE = nullptr;
const EntityType* WITCH = nullptr;
const EntityType* SLIME = nullptr;
const EntityType* GIANT = nullptr;

// 海洋怪物
const EntityType* GUARDIAN = nullptr;
const EntityType* ELDER_GUARDIAN = nullptr;

// 亡灵变种
const EntityType* HUSK = nullptr;
const EntityType* DROWNED = nullptr;
const EntityType* STRAY = nullptr;
const EntityType* WITHER_SKELETON = nullptr;
const EntityType* PHANTOM = nullptr;
const EntityType* ZOMBIE_VILLAGER = nullptr;
const EntityType* ZOMBIFIED_PIGLIN = nullptr;

// 节肢动物变种
const EntityType* CAVE_SPIDER = nullptr;
const EntityType* SILVERFISH = nullptr;
const EntityType* ENDERMITE = nullptr;

// 末地生物
const EntityType* SHULKER = nullptr;

// 地狱生物
const EntityType* GHAST = nullptr;
const EntityType* MAGMA_CUBE = nullptr;
const EntityType* PIGLIN = nullptr;
const EntityType* PIGLIN_BRUTE = nullptr;
const EntityType* HOGLIN = nullptr;
const EntityType* ZOGLIN = nullptr;

// 灾厄村民
const EntityType* VINDICATOR = nullptr;
const EntityType* EVOKER = nullptr;
const EntityType* ILLUSIONER = nullptr;
const EntityType* PILLAGER = nullptr;
const EntityType* RAVAGER = nullptr;
const EntityType* VEX = nullptr;

// 试炼密室
const EntityType* BREEZE = nullptr;

// ============================================================================
// Boss
// ============================================================================

const EntityType* ENDER_DRAGON = nullptr;
const EntityType* WITHER = nullptr;
const EntityType* WARDEN = nullptr;

// ============================================================================
// 村民
// ============================================================================

const EntityType* VILLAGER = nullptr;
const EntityType* WANDERING_TRADER = nullptr;

// ============================================================================
// 其他实体
// ============================================================================

const EntityType* Unknown = nullptr;
const EntityType* PLAYER = nullptr;
const EntityType* ITEM = nullptr;
const EntityType* EXPERIENCE_ORB = nullptr;

// ============================================================================
// 投掷物
// ============================================================================

const EntityType* ARROW = nullptr;
const EntityType* SPECTRAL_ARROW = nullptr;
const EntityType* TRIDENT = nullptr;
const EntityType* SPEAR = nullptr;
const EntityType* SNOWBALL = nullptr;
const EntityType* EGG = nullptr;
const EntityType* ENDER_PEARL = nullptr;
const EntityType* POTION = nullptr;
const EntityType* EXPERIENCE_BOTTLE = nullptr;
const EntityType* FIREBALL = nullptr;
const EntityType* SMALL_FIREBALL = nullptr;
const EntityType* DRAGON_FIREBALL = nullptr;
const EntityType* WITHER_SKULL = nullptr;
const EntityType* LLAMA_SPIT = nullptr;
const EntityType* SHULKER_BULLET = nullptr;
const EntityType* EVOKER_FANGS = nullptr;
const EntityType* FISHING_BOBBER = nullptr;
const EntityType* EYE_OF_ENDER = nullptr;
const EntityType* FIREWORK_ROCKET = nullptr;
const EntityType* WIND_CHARGE = nullptr;
// ============================================================================

const EntityType* BOAT = nullptr;
const EntityType* CHEST_BOAT = nullptr;
const EntityType* MINECART = nullptr;
const EntityType* CHEST_MINECART = nullptr;
const EntityType* FURNACE_MINECART = nullptr;
const EntityType* HOPPER_MINECART = nullptr;
const EntityType* TNT_MINECART = nullptr;
const EntityType* SPAWNER_MINECART = nullptr;

// ============================================================================
// 其他实体
// ============================================================================

const EntityType* FALLING_BLOCK = nullptr;
const EntityType* TNT = nullptr;
const EntityType* END_CRYSTAL = nullptr;
const EntityType* LIGHTNING_BOLT = nullptr;
const EntityType* AREA_EFFECT_CLOUD = nullptr;
const EntityType* ARMOR_STAND = nullptr;
const EntityType* OMINOUS_ITEM_SPAWNER = nullptr;
const EntityType* PAINTING = nullptr;
const EntityType* ITEM_FRAME = nullptr;
const EntityType* LEASH_KNOT = nullptr;

// ============================================================================
// 初始化
// ============================================================================

void initialize()
{
    auto& registry = EntityRegistry::instance();

    // 被动生物 - 普通动物
    PIG = registry.getType(EntityTypeKeys::PIG);
    COW = registry.getType(EntityTypeKeys::COW);
    SHEEP = registry.getType(EntityTypeKeys::SHEEP);
    CHICKEN = registry.getType(EntityTypeKeys::CHICKEN);
    RABBIT = registry.getType(EntityTypeKeys::RABBIT);
    MOOSHROOM = registry.getType(EntityTypeKeys::MOOSHROOM);

    // 被动生物 - 可驯服动物
    WOLF = registry.getType(EntityTypeKeys::WOLF);
    CAT = registry.getType(EntityTypeKeys::CAT);
    OCELOT = registry.getType(EntityTypeKeys::OCELOT);
    PARROT = registry.getType(EntityTypeKeys::PARROT);

    // 被动生物 - 特殊动物
    FOX = registry.getType(EntityTypeKeys::FOX);
    PANDA = registry.getType(EntityTypeKeys::PANDA);
    POLAR_BEAR = registry.getType(EntityTypeKeys::POLAR_BEAR);
    TURTLE = registry.getType(EntityTypeKeys::TURTLE);
    BEE = registry.getType(EntityTypeKeys::BEE);
    STRIDER = registry.getType(EntityTypeKeys::STRIDER);
    SNIFFER = registry.getType(EntityTypeKeys::SNIFFER);

    // 被动生物 - 马类
    HORSE = registry.getType(EntityTypeKeys::HORSE);
    DONKEY = registry.getType(EntityTypeKeys::DONKEY);
    MULE = registry.getType(EntityTypeKeys::MULE);
    LLAMA = registry.getType(EntityTypeKeys::LLAMA);
    TRADER_LLAMA = registry.getType(EntityTypeKeys::TRADER_LLAMA);
    SKELETON_HORSE = registry.getType(EntityTypeKeys::SKELETON_HORSE);
    ZOMBIE_HORSE = registry.getType(EntityTypeKeys::ZOMBIE_HORSE);

    // 被动生物 - 水生生物
    COD = registry.getType(EntityTypeKeys::COD);
    SALMON = registry.getType(EntityTypeKeys::SALMON);
    PUFFERFISH = registry.getType(EntityTypeKeys::PUFFERFISH);
    TROPICAL_FISH = registry.getType(EntityTypeKeys::TROPICAL_FISH);
    SQUID = registry.getType(EntityTypeKeys::SQUID);
    GLOW_SQUID = registry.getType(EntityTypeKeys::GLOW_SQUID);
    DOLPHIN = registry.getType(EntityTypeKeys::DOLPHIN);
    AXOLOTL = registry.getType(EntityTypeKeys::AXOLOTL);
    NAUTILUS = registry.getType(EntityTypeKeys::NAUTILUS);
    ZOMBIE_NAUTILUS = registry.getType(EntityTypeKeys::ZOMBIE_NAUTILUS);

    // 被动生物 - 环境生物
    BAT = registry.getType(EntityTypeKeys::BAT);

    // 被动生物 - 傀儡
    IRON_GOLEM = registry.getType(EntityTypeKeys::IRON_GOLEM);
    SNOW_GOLEM = registry.getType(EntityTypeKeys::SNOW_GOLEM);
    COPPER_GOLEM = registry.getType(EntityTypeKeys::COPPER_GOLEM);

    // 敌对生物 - 基础怪物
    ZOMBIE = registry.getType(EntityTypeKeys::ZOMBIE);
    SKELETON = registry.getType(EntityTypeKeys::SKELETON);
    CREEPER = registry.getType(EntityTypeKeys::CREEPER);
    SPIDER = registry.getType(EntityTypeKeys::SPIDER);
    ENDERMAN = registry.getType(EntityTypeKeys::ENDERMAN);
    BLAZE = registry.getType(EntityTypeKeys::BLAZE);
    WITCH = registry.getType(EntityTypeKeys::WITCH);
    SLIME = registry.getType(EntityTypeKeys::SLIME);
    GIANT = registry.getType(EntityTypeKeys::GIANT);

    // 敌对生物 - 海洋怪物
    GUARDIAN = registry.getType(EntityTypeKeys::GUARDIAN);
    ELDER_GUARDIAN = registry.getType(EntityTypeKeys::ELDER_GUARDIAN);

    // 敌对生物 - 亡灵变种
    HUSK = registry.getType(EntityTypeKeys::HUSK);
    DROWNED = registry.getType(EntityTypeKeys::DROWNED);
    STRAY = registry.getType(EntityTypeKeys::STRAY);
    WITHER_SKELETON = registry.getType(EntityTypeKeys::WITHER_SKELETON);
    PHANTOM = registry.getType(EntityTypeKeys::PHANTOM);
    ZOMBIE_VILLAGER = registry.getType(EntityTypeKeys::ZOMBIE_VILLAGER);
    ZOMBIFIED_PIGLIN = registry.getType(EntityTypeKeys::ZOMBIFIED_PIGLIN);

    // 敌对生物 - 节肢动物变种
    CAVE_SPIDER = registry.getType(EntityTypeKeys::CAVE_SPIDER);
    SILVERFISH = registry.getType(EntityTypeKeys::SILVERFISH);
    ENDERMITE = registry.getType(EntityTypeKeys::ENDERMITE);

    // 敌对生物 - 末地生物
    SHULKER = registry.getType(EntityTypeKeys::SHULKER);

    // 敌对生物 - 地狱生物
    GHAST = registry.getType(EntityTypeKeys::GHAST);
    MAGMA_CUBE = registry.getType(EntityTypeKeys::MAGMA_CUBE);
    PIGLIN = registry.getType(EntityTypeKeys::PIGLIN);
    PIGLIN_BRUTE = registry.getType(EntityTypeKeys::PIGLIN_BRUTE);
    HOGLIN = registry.getType(EntityTypeKeys::HOGLIN);
    ZOGLIN = registry.getType(EntityTypeKeys::ZOGLIN);

    // 敌对生物 - 灾厄村民
    VINDICATOR = registry.getType(EntityTypeKeys::VINDICATOR);
    EVOKER = registry.getType(EntityTypeKeys::EVOKER);
    ILLUSIONER = registry.getType(EntityTypeKeys::ILLUSIONER);
    PILLAGER = registry.getType(EntityTypeKeys::PILLAGER);
    RAVAGER = registry.getType(EntityTypeKeys::RAVAGER);
    VEX = registry.getType(EntityTypeKeys::VEX);

    // 试炼密室
    BREEZE = registry.getType(EntityTypeKeys::BREEZE);

    // Boss
    ENDER_DRAGON = registry.getType(EntityTypeKeys::ENDER_DRAGON);
    WITHER = registry.getType(EntityTypeKeys::WITHER);
    WARDEN = registry.getType(EntityTypeKeys::WARDEN);

    // 村民
    VILLAGER = registry.getType(EntityTypeKeys::VILLAGER);
    WANDERING_TRADER = registry.getType(EntityTypeKeys::WANDERING_TRADER);

    // 其他实体
    PLAYER = registry.getType(EntityTypeKeys::PLAYER);
    ITEM = registry.getType(EntityTypeKeys::ITEM);
    EXPERIENCE_ORB = registry.getType(EntityTypeKeys::EXPERIENCE_ORB);

    // 投掷物
    ARROW = registry.getType(EntityTypeKeys::ARROW);
    SPECTRAL_ARROW = registry.getType(EntityTypeKeys::SPECTRAL_ARROW);
    TRIDENT = registry.getType(EntityTypeKeys::TRIDENT);
    SPEAR = registry.getType(EntityTypeKeys::SPEAR);
    SNOWBALL = registry.getType(EntityTypeKeys::SNOWBALL);
    EGG = registry.getType(EntityTypeKeys::EGG);
    ENDER_PEARL = registry.getType(EntityTypeKeys::ENDER_PEARL);
    POTION = registry.getType(EntityTypeKeys::POTION);
    EXPERIENCE_BOTTLE = registry.getType(EntityTypeKeys::EXPERIENCE_BOTTLE);
    FIREBALL = registry.getType(EntityTypeKeys::FIREBALL);
    SMALL_FIREBALL = registry.getType(EntityTypeKeys::SMALL_FIREBALL);
    DRAGON_FIREBALL = registry.getType(EntityTypeKeys::DRAGON_FIREBALL);
    WITHER_SKULL = registry.getType(EntityTypeKeys::WITHER_SKULL);
    LLAMA_SPIT = registry.getType(EntityTypeKeys::LLAMA_SPIT);
    SHULKER_BULLET = registry.getType(EntityTypeKeys::SHULKER_BULLET);
    EVOKER_FANGS = registry.getType(EntityTypeKeys::EVOKER_FANGS);
    FISHING_BOBBER = registry.getType(EntityTypeKeys::FISHING_BOBBER);
    EYE_OF_ENDER = registry.getType(EntityTypeKeys::EYE_OF_ENDER);
    FIREWORK_ROCKET = registry.getType(EntityTypeKeys::FIREWORK_ROCKET);
    WIND_CHARGE = registry.getType(EntityTypeKeys::WIND_CHARGE);
    BOAT = registry.getType(EntityTypeKeys::BOAT);
    CHEST_BOAT = registry.getType(EntityTypeKeys::CHEST_BOAT);
    MINECART = registry.getType(EntityTypeKeys::MINECART);
    CHEST_MINECART = registry.getType(EntityTypeKeys::CHEST_MINECART);
    FURNACE_MINECART = registry.getType(EntityTypeKeys::FURNACE_MINECART);
    HOPPER_MINECART = registry.getType(EntityTypeKeys::HOPPER_MINECART);
    TNT_MINECART = registry.getType(EntityTypeKeys::TNT_MINECART);
    SPAWNER_MINECART = registry.getType(EntityTypeKeys::SPAWNER_MINECART);

    // 其他实体
    FALLING_BLOCK = registry.getType(EntityTypeKeys::FALLING_BLOCK);
    TNT = registry.getType(EntityTypeKeys::TNT);
    END_CRYSTAL = registry.getType(EntityTypeKeys::END_CRYSTAL);
    LIGHTNING_BOLT = registry.getType(EntityTypeKeys::LIGHTNING_BOLT);
    AREA_EFFECT_CLOUD = registry.getType(EntityTypeKeys::AREA_EFFECT_CLOUD);
    ARMOR_STAND = registry.getType(EntityTypeKeys::ARMOR_STAND);
    OMINOUS_ITEM_SPAWNER = registry.getType(EntityTypeKeys::OMINOUS_ITEM_SPAWNER);
    PAINTING = registry.getType(EntityTypeKeys::PAINTING);
    ITEM_FRAME = registry.getType(EntityTypeKeys::ITEM_FRAME);
    LEASH_KNOT = registry.getType(EntityTypeKeys::LEASH_KNOT);
}

void reset()
{
    // 与 initialize() 中的赋值一一对应，全部置 nullptr（未初始化）。
    // 保持与 EntityRegistry::clear() 后"注册表为空"状态一致。
    PIG = nullptr;
    COW = nullptr;
    SHEEP = nullptr;
    CHICKEN = nullptr;
    RABBIT = nullptr;
    MOOSHROOM = nullptr;
    WOLF = nullptr;
    CAT = nullptr;
    OCELOT = nullptr;
    PARROT = nullptr;
    FOX = nullptr;
    PANDA = nullptr;
    POLAR_BEAR = nullptr;
    TURTLE = nullptr;
    BEE = nullptr;
    STRIDER = nullptr;
    SNIFFER = nullptr;
    HORSE = nullptr;
    DONKEY = nullptr;
    MULE = nullptr;
    LLAMA = nullptr;
    TRADER_LLAMA = nullptr;
    SKELETON_HORSE = nullptr;
    ZOMBIE_HORSE = nullptr;
    COD = nullptr;
    SALMON = nullptr;
    PUFFERFISH = nullptr;
    TROPICAL_FISH = nullptr;
    SQUID = nullptr;
    GLOW_SQUID = nullptr;
    DOLPHIN = nullptr;
    AXOLOTL = nullptr;
    NAUTILUS = nullptr;
    ZOMBIE_NAUTILUS = nullptr;
    BAT = nullptr;
    IRON_GOLEM = nullptr;
    SNOW_GOLEM = nullptr;
    COPPER_GOLEM = nullptr;
    ZOMBIE = nullptr;
    SKELETON = nullptr;
    CREEPER = nullptr;
    SPIDER = nullptr;
    ENDERMAN = nullptr;
    BLAZE = nullptr;
    WITCH = nullptr;
    SLIME = nullptr;
    GIANT = nullptr;
    GUARDIAN = nullptr;
    ELDER_GUARDIAN = nullptr;
    HUSK = nullptr;
    DROWNED = nullptr;
    STRAY = nullptr;
    WITHER_SKELETON = nullptr;
    PHANTOM = nullptr;
    ZOMBIE_VILLAGER = nullptr;
    ZOMBIFIED_PIGLIN = nullptr;
    CAVE_SPIDER = nullptr;
    SILVERFISH = nullptr;
    ENDERMITE = nullptr;
    SHULKER = nullptr;
    GHAST = nullptr;
    MAGMA_CUBE = nullptr;
    PIGLIN = nullptr;
    PIGLIN_BRUTE = nullptr;
    HOGLIN = nullptr;
    ZOGLIN = nullptr;
    VINDICATOR = nullptr;
    EVOKER = nullptr;
    ILLUSIONER = nullptr;
    PILLAGER = nullptr;
    RAVAGER = nullptr;
    VEX = nullptr;
    BREEZE = nullptr;
    ENDER_DRAGON = nullptr;
    WITHER = nullptr;
    WARDEN = nullptr;
    VILLAGER = nullptr;
    WANDERING_TRADER = nullptr;
    Unknown = nullptr;
    PLAYER = nullptr;
    ITEM = nullptr;
    EXPERIENCE_ORB = nullptr;
    ARROW = nullptr;
    SPECTRAL_ARROW = nullptr;
    TRIDENT = nullptr;
    SPEAR = nullptr;
    SNOWBALL = nullptr;
    EGG = nullptr;
    ENDER_PEARL = nullptr;
    POTION = nullptr;
    EXPERIENCE_BOTTLE = nullptr;
    FIREBALL = nullptr;
    SMALL_FIREBALL = nullptr;
    DRAGON_FIREBALL = nullptr;
    WITHER_SKULL = nullptr;
    LLAMA_SPIT = nullptr;
    SHULKER_BULLET = nullptr;
    EVOKER_FANGS = nullptr;
    FISHING_BOBBER = nullptr;
    EYE_OF_ENDER = nullptr;
    FIREWORK_ROCKET = nullptr;
    WIND_CHARGE = nullptr;
    BOAT = nullptr;
    CHEST_BOAT = nullptr;
    MINECART = nullptr;
    CHEST_MINECART = nullptr;
    FURNACE_MINECART = nullptr;
    HOPPER_MINECART = nullptr;
    TNT_MINECART = nullptr;
    SPAWNER_MINECART = nullptr;
    FALLING_BLOCK = nullptr;
    TNT = nullptr;
    END_CRYSTAL = nullptr;
    LIGHTNING_BOLT = nullptr;
    AREA_EFFECT_CLOUD = nullptr;
    ARMOR_STAND = nullptr;
    OMINOUS_ITEM_SPAWNER = nullptr;
    PAINTING = nullptr;
    ITEM_FRAME = nullptr;
    LEASH_KNOT = nullptr;
}

} // namespace VanillaEntityTypeKeys
} // namespace entity
} // namespace mc
