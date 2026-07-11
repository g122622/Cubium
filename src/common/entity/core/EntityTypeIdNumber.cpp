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

#include "EntityTypeIdNumber.hpp"
#include "EntityRegistry.hpp"

namespace mc {
namespace entity {
namespace EntityTypeIdNumber {

namespace {

/**
 * @brief 安全获取实体类型 ID
 * @param registry 实体注册表
 * @param name 实体类型名称
 * @return 实体类型 ID，如果未注册返回 0
 */
EntityTypeId safeGetId(EntityRegistry& registry, const char* name)
{
    const auto* type = registry.getType(name);
    if (type) {
        return type->id();
    }
    // 未注册的类型保持 ID 为 0
    return 0;
}

} // namespace

// ============================================================================
// 被动生物
// ============================================================================

// 普通动物
EntityTypeId PIG = 0;
EntityTypeId COW = 0;
EntityTypeId SHEEP = 0;
EntityTypeId CHICKEN = 0;
EntityTypeId RABBIT = 0;
EntityTypeId MOOSHROOM = 0;

// 可驯服动物
EntityTypeId WOLF = 0;
EntityTypeId CAT = 0;
EntityTypeId OCELOT = 0;
EntityTypeId PARROT = 0;

// 特殊动物
EntityTypeId FOX = 0;
EntityTypeId PANDA = 0;
EntityTypeId POLAR_BEAR = 0;
EntityTypeId TURTLE = 0;
EntityTypeId BEE = 0;
EntityTypeId STRIDER = 0;

// 马类
EntityTypeId HORSE = 0;
EntityTypeId DONKEY = 0;
EntityTypeId MULE = 0;
EntityTypeId LLAMA = 0;
EntityTypeId TRADER_LLAMA = 0;
EntityTypeId SKELETON_HORSE = 0;
EntityTypeId ZOMBIE_HORSE = 0;

// 水生生物
EntityTypeId COD = 0;
EntityTypeId SALMON = 0;
EntityTypeId PUFFERFISH = 0;
EntityTypeId TROPICAL_FISH = 0;
EntityTypeId SQUID = 0;
EntityTypeId GLOW_SQUID = 0;
EntityTypeId DOLPHIN = 0;
EntityTypeId AXOLOTL = 0;
EntityTypeId NAUTILUS = 0;
EntityTypeId ZOMBIE_NAUTILUS = 0;

// 环境生物
EntityTypeId BAT = 0;

// 傀儡
EntityTypeId IRON_GOLEM = 0;
EntityTypeId SNOW_GOLEM = 0;
EntityTypeId COPPER_GOLEM = 0;

// ============================================================================
// 敌对生物
// ============================================================================

// 基础怪物
EntityTypeId ZOMBIE = 0;
EntityTypeId SKELETON = 0;
EntityTypeId CREEPER = 0;
EntityTypeId SPIDER = 0;
EntityTypeId ENDERMAN = 0;
EntityTypeId BLAZE = 0;
EntityTypeId WITCH = 0;
EntityTypeId SLIME = 0;
EntityTypeId GIANT = 0;

// 海洋怪物
EntityTypeId GUARDIAN = 0;
EntityTypeId ELDER_GUARDIAN = 0;

// 亡灵变种
EntityTypeId HUSK = 0;
EntityTypeId DROWNED = 0;
EntityTypeId STRAY = 0;
EntityTypeId WITHER_SKELETON = 0;
EntityTypeId PHANTOM = 0;
EntityTypeId ZOMBIE_VILLAGER = 0;
EntityTypeId ZOMBIFIED_PIGLIN = 0;

// 节肢动物变种
EntityTypeId CAVE_SPIDER = 0;
EntityTypeId SILVERFISH = 0;
EntityTypeId ENDERMITE = 0;

// 末地生物
EntityTypeId SHULKER = 0;

// 地狱生物
EntityTypeId GHAST = 0;
EntityTypeId MAGMA_CUBE = 0;
EntityTypeId PIGLIN = 0;
EntityTypeId PIGLIN_BRUTE = 0;
EntityTypeId HOGLIN = 0;
EntityTypeId ZOGLIN = 0;

// 灾厄村民
EntityTypeId VINDICATOR = 0;
EntityTypeId EVOKER = 0;
EntityTypeId ILLUSIONER = 0;
EntityTypeId PILLAGER = 0;
EntityTypeId RAVAGER = 0;
EntityTypeId VEX = 0;

// 试炼密室
EntityTypeId BREEZE = 0;

// ============================================================================
// Boss
// ============================================================================

EntityTypeId ENDER_DRAGON = 0;
EntityTypeId WITHER = 0;
EntityTypeId WARDEN = 0;

// ============================================================================
// 村民
// ============================================================================

EntityTypeId VILLAGER = 0;
EntityTypeId WANDERING_TRADER = 0;

// ============================================================================
// 其他实体
// ============================================================================

EntityTypeId Unknown = 0;
EntityTypeId PLAYER = 0;
EntityTypeId ITEM = 0;
EntityTypeId EXPERIENCE_ORB = 0;

// ============================================================================
// 投掷物
// ============================================================================

EntityTypeId ARROW = 0;
EntityTypeId SPECTRAL_ARROW = 0;
EntityTypeId TRIDENT = 0;
EntityTypeId SPEAR = 0;
EntityTypeId SNOWBALL = 0;
EntityTypeId EGG = 0;
EntityTypeId ENDER_PEARL = 0;
EntityTypeId POTION = 0;
EntityTypeId EXPERIENCE_BOTTLE = 0;
EntityTypeId FIREBALL = 0;
EntityTypeId SMALL_FIREBALL = 0;
EntityTypeId DRAGON_FIREBALL = 0;
EntityTypeId WITHER_SKULL = 0;
EntityTypeId LLAMA_SPIT = 0;
EntityTypeId SHULKER_BULLET = 0;
EntityTypeId EVOKER_FANGS = 0;
EntityTypeId FISHING_BOBBER = 0;
EntityTypeId EYE_OF_ENDER = 0;
EntityTypeId FIREWORK_ROCKET = 0;
EntityTypeId WIND_CHARGE = 0;
// ============================================================================

EntityTypeId BOAT = 0;
EntityTypeId CHEST_BOAT = 0;
EntityTypeId MINECART = 0;
EntityTypeId CHEST_MINECART = 0;
EntityTypeId FURNACE_MINECART = 0;
EntityTypeId HOPPER_MINECART = 0;
EntityTypeId TNT_MINECART = 0;
EntityTypeId SPAWNER_MINECART = 0;

// ============================================================================
// 其他实体
// ============================================================================

EntityTypeId FALLING_BLOCK = 0;
EntityTypeId TNT = 0;
EntityTypeId END_CRYSTAL = 0;
EntityTypeId LIGHTNING_BOLT = 0;
EntityTypeId AREA_EFFECT_CLOUD = 0;
EntityTypeId ARMOR_STAND = 0;
EntityTypeId OMINOUS_ITEM_SPAWNER = 0;
EntityTypeId PAINTING = 0;
EntityTypeId ITEM_FRAME = 0;
EntityTypeId LEASH_KNOT = 0;

// ============================================================================
// 初始化
// ============================================================================

void initialize()
{
    auto& registry = EntityRegistry::instance();

    // 被动生物 - 普通动物
    PIG = safeGetId(registry, EntityTypes::PIG);
    COW = safeGetId(registry, EntityTypes::COW);
    SHEEP = safeGetId(registry, EntityTypes::SHEEP);
    CHICKEN = safeGetId(registry, EntityTypes::CHICKEN);
    RABBIT = safeGetId(registry, EntityTypes::RABBIT);
    MOOSHROOM = safeGetId(registry, EntityTypes::MOOSHROOM);

    // 被动生物 - 可驯服动物
    WOLF = safeGetId(registry, EntityTypes::WOLF);
    CAT = safeGetId(registry, EntityTypes::CAT);
    OCELOT = safeGetId(registry, EntityTypes::OCELOT);
    PARROT = safeGetId(registry, EntityTypes::PARROT);

    // 被动生物 - 特殊动物
    FOX = safeGetId(registry, EntityTypes::FOX);
    PANDA = safeGetId(registry, EntityTypes::PANDA);
    POLAR_BEAR = safeGetId(registry, EntityTypes::POLAR_BEAR);
    TURTLE = safeGetId(registry, EntityTypes::TURTLE);
    BEE = safeGetId(registry, EntityTypes::BEE);
    STRIDER = safeGetId(registry, EntityTypes::STRIDER);

    // 被动生物 - 马类
    HORSE = safeGetId(registry, EntityTypes::HORSE);
    DONKEY = safeGetId(registry, EntityTypes::DONKEY);
    MULE = safeGetId(registry, EntityTypes::MULE);
    LLAMA = safeGetId(registry, EntityTypes::LLAMA);
    TRADER_LLAMA = safeGetId(registry, EntityTypes::TRADER_LLAMA);
    SKELETON_HORSE = safeGetId(registry, EntityTypes::SKELETON_HORSE);
    ZOMBIE_HORSE = safeGetId(registry, EntityTypes::ZOMBIE_HORSE);

    // 被动生物 - 水生生物
    COD = safeGetId(registry, EntityTypes::COD);
    SALMON = safeGetId(registry, EntityTypes::SALMON);
    PUFFERFISH = safeGetId(registry, EntityTypes::PUFFERFISH);
    TROPICAL_FISH = safeGetId(registry, EntityTypes::TROPICAL_FISH);
    SQUID = safeGetId(registry, EntityTypes::SQUID);
    GLOW_SQUID = safeGetId(registry, EntityTypes::GLOW_SQUID);
    DOLPHIN = safeGetId(registry, EntityTypes::DOLPHIN);
    AXOLOTL = safeGetId(registry, EntityTypes::AXOLOTL);
    NAUTILUS = safeGetId(registry, EntityTypes::NAUTILUS);
    ZOMBIE_NAUTILUS = safeGetId(registry, EntityTypes::ZOMBIE_NAUTILUS);

    // 被动生物 - 环境生物
    BAT = safeGetId(registry, EntityTypes::BAT);

    // 被动生物 - 傀儡
    IRON_GOLEM = safeGetId(registry, EntityTypes::IRON_GOLEM);
    SNOW_GOLEM = safeGetId(registry, EntityTypes::SNOW_GOLEM);
    COPPER_GOLEM = safeGetId(registry, EntityTypes::COPPER_GOLEM);

    // 敌对生物 - 基础怪物
    ZOMBIE = safeGetId(registry, EntityTypes::ZOMBIE);
    SKELETON = safeGetId(registry, EntityTypes::SKELETON);
    CREEPER = safeGetId(registry, EntityTypes::CREEPER);
    SPIDER = safeGetId(registry, EntityTypes::SPIDER);
    ENDERMAN = safeGetId(registry, EntityTypes::ENDERMAN);
    BLAZE = safeGetId(registry, EntityTypes::BLAZE);
    WITCH = safeGetId(registry, EntityTypes::WITCH);
    SLIME = safeGetId(registry, EntityTypes::SLIME);
    GIANT = safeGetId(registry, EntityTypes::GIANT);

    // 敌对生物 - 海洋怪物
    GUARDIAN = safeGetId(registry, EntityTypes::GUARDIAN);
    ELDER_GUARDIAN = safeGetId(registry, EntityTypes::ELDER_GUARDIAN);

    // 敌对生物 - 亡灵变种
    HUSK = safeGetId(registry, EntityTypes::HUSK);
    DROWNED = safeGetId(registry, EntityTypes::DROWNED);
    STRAY = safeGetId(registry, EntityTypes::STRAY);
    WITHER_SKELETON = safeGetId(registry, EntityTypes::WITHER_SKELETON);
    PHANTOM = safeGetId(registry, EntityTypes::PHANTOM);
    ZOMBIE_VILLAGER = safeGetId(registry, EntityTypes::ZOMBIE_VILLAGER);
    ZOMBIFIED_PIGLIN = safeGetId(registry, EntityTypes::ZOMBIFIED_PIGLIN);

    // 敌对生物 - 节肢动物变种
    CAVE_SPIDER = safeGetId(registry, EntityTypes::CAVE_SPIDER);
    SILVERFISH = safeGetId(registry, EntityTypes::SILVERFISH);
    ENDERMITE = safeGetId(registry, EntityTypes::ENDERMITE);

    // 敌对生物 - 末地生物
    SHULKER = safeGetId(registry, EntityTypes::SHULKER);

    // 敌对生物 - 地狱生物
    GHAST = safeGetId(registry, EntityTypes::GHAST);
    MAGMA_CUBE = safeGetId(registry, EntityTypes::MAGMA_CUBE);
    PIGLIN = safeGetId(registry, EntityTypes::PIGLIN);
    PIGLIN_BRUTE = safeGetId(registry, EntityTypes::PIGLIN_BRUTE);
    HOGLIN = safeGetId(registry, EntityTypes::HOGLIN);
    ZOGLIN = safeGetId(registry, EntityTypes::ZOGLIN);

    // 敌对生物 - 灾厄村民
    VINDICATOR = safeGetId(registry, EntityTypes::VINDICATOR);
    EVOKER = safeGetId(registry, EntityTypes::EVOKER);
    ILLUSIONER = safeGetId(registry, EntityTypes::ILLUSIONER);
    PILLAGER = safeGetId(registry, EntityTypes::PILLAGER);
    RAVAGER = safeGetId(registry, EntityTypes::RAVAGER);
    VEX = safeGetId(registry, EntityTypes::VEX);

    // 试炼密室
    BREEZE = safeGetId(registry, EntityTypes::BREEZE);

    // Boss
    ENDER_DRAGON = safeGetId(registry, EntityTypes::ENDER_DRAGON);
    WITHER = safeGetId(registry, EntityTypes::WITHER);
    WARDEN = safeGetId(registry, EntityTypes::WARDEN);

    // 村民
    VILLAGER = safeGetId(registry, EntityTypes::VILLAGER);
    WANDERING_TRADER = safeGetId(registry, EntityTypes::WANDERING_TRADER);

    // 其他实体
    // PLAYER 由 Player 类自行管理，不在此初始化
    ITEM = safeGetId(registry, EntityTypes::ITEM);
    EXPERIENCE_ORB = safeGetId(registry, EntityTypes::EXPERIENCE_ORB);

    // 投掷物
    ARROW = safeGetId(registry, EntityTypes::ARROW);
    SPECTRAL_ARROW = safeGetId(registry, EntityTypes::SPECTRAL_ARROW);
    TRIDENT = safeGetId(registry, EntityTypes::TRIDENT);
    SPEAR = safeGetId(registry, EntityTypes::SPEAR);
    SNOWBALL = safeGetId(registry, EntityTypes::SNOWBALL);
    EGG = safeGetId(registry, EntityTypes::EGG);
    ENDER_PEARL = safeGetId(registry, EntityTypes::ENDER_PEARL);
    POTION = safeGetId(registry, EntityTypes::POTION);
    EXPERIENCE_BOTTLE = safeGetId(registry, EntityTypes::EXPERIENCE_BOTTLE);
    FIREBALL = safeGetId(registry, EntityTypes::FIREBALL);
    SMALL_FIREBALL = safeGetId(registry, EntityTypes::SMALL_FIREBALL);
    DRAGON_FIREBALL = safeGetId(registry, EntityTypes::DRAGON_FIREBALL);
    WITHER_SKULL = safeGetId(registry, EntityTypes::WITHER_SKULL);
    LLAMA_SPIT = safeGetId(registry, EntityTypes::LLAMA_SPIT);
    SHULKER_BULLET = safeGetId(registry, EntityTypes::SHULKER_BULLET);
    EVOKER_FANGS = safeGetId(registry, EntityTypes::EVOKER_FANGS);
    FISHING_BOBBER = safeGetId(registry, EntityTypes::FISHING_BOBBER);
    EYE_OF_ENDER = safeGetId(registry, EntityTypes::EYE_OF_ENDER);
    FIREWORK_ROCKET = safeGetId(registry, EntityTypes::FIREWORK_ROCKET);
    WIND_CHARGE = safeGetId(registry, EntityTypes::WIND_CHARGE);
    BOAT = safeGetId(registry, EntityTypes::BOAT);
    CHEST_BOAT = safeGetId(registry, EntityTypes::CHEST_BOAT);
    MINECART = safeGetId(registry, EntityTypes::MINECART);
    CHEST_MINECART = safeGetId(registry, EntityTypes::CHEST_MINECART);
    FURNACE_MINECART = safeGetId(registry, EntityTypes::FURNACE_MINECART);
    HOPPER_MINECART = safeGetId(registry, EntityTypes::HOPPER_MINECART);
    TNT_MINECART = safeGetId(registry, EntityTypes::TNT_MINECART);
    SPAWNER_MINECART = safeGetId(registry, EntityTypes::SPAWNER_MINECART);

    // 其他实体
    FALLING_BLOCK = safeGetId(registry, EntityTypes::FALLING_BLOCK);
    TNT = safeGetId(registry, EntityTypes::TNT);
    END_CRYSTAL = safeGetId(registry, EntityTypes::END_CRYSTAL);
    LIGHTNING_BOLT = safeGetId(registry, EntityTypes::LIGHTNING_BOLT);
    AREA_EFFECT_CLOUD = safeGetId(registry, EntityTypes::AREA_EFFECT_CLOUD);
    ARMOR_STAND = safeGetId(registry, EntityTypes::ARMOR_STAND);
    OMINOUS_ITEM_SPAWNER = safeGetId(registry, EntityTypes::OMINOUS_ITEM_SPAWNER);
    PAINTING = safeGetId(registry, EntityTypes::PAINTING);
    ITEM_FRAME = safeGetId(registry, EntityTypes::ITEM_FRAME);
    LEASH_KNOT = safeGetId(registry, EntityTypes::LEASH_KNOT);
}

void reset()
{
    // 与 initialize() 中的赋值一一对应，全部置 0（未初始化）。
    // 保持与 EntityRegistry::clear() 后"注册表为空"状态一致。
    PIG = 0;
    COW = 0;
    SHEEP = 0;
    CHICKEN = 0;
    RABBIT = 0;
    MOOSHROOM = 0;
    WOLF = 0;
    CAT = 0;
    OCELOT = 0;
    PARROT = 0;
    FOX = 0;
    PANDA = 0;
    POLAR_BEAR = 0;
    TURTLE = 0;
    BEE = 0;
    STRIDER = 0;
    HORSE = 0;
    DONKEY = 0;
    MULE = 0;
    LLAMA = 0;
    TRADER_LLAMA = 0;
    SKELETON_HORSE = 0;
    ZOMBIE_HORSE = 0;
    COD = 0;
    SALMON = 0;
    PUFFERFISH = 0;
    TROPICAL_FISH = 0;
    SQUID = 0;
    GLOW_SQUID = 0;
    DOLPHIN = 0;
    AXOLOTL = 0;
    NAUTILUS = 0;
    ZOMBIE_NAUTILUS = 0;
    BAT = 0;
    IRON_GOLEM = 0;
    SNOW_GOLEM = 0;
    COPPER_GOLEM = 0;
    ZOMBIE = 0;
    SKELETON = 0;
    CREEPER = 0;
    SPIDER = 0;
    ENDERMAN = 0;
    BLAZE = 0;
    WITCH = 0;
    SLIME = 0;
    GIANT = 0;
    GUARDIAN = 0;
    ELDER_GUARDIAN = 0;
    HUSK = 0;
    DROWNED = 0;
    STRAY = 0;
    WITHER_SKELETON = 0;
    PHANTOM = 0;
    ZOMBIE_VILLAGER = 0;
    ZOMBIFIED_PIGLIN = 0;
    CAVE_SPIDER = 0;
    SILVERFISH = 0;
    ENDERMITE = 0;
    SHULKER = 0;
    GHAST = 0;
    MAGMA_CUBE = 0;
    PIGLIN = 0;
    PIGLIN_BRUTE = 0;
    HOGLIN = 0;
    ZOGLIN = 0;
    VINDICATOR = 0;
    EVOKER = 0;
    ILLUSIONER = 0;
    PILLAGER = 0;
    RAVAGER = 0;
    VEX = 0;
    BREEZE = 0;
    ENDER_DRAGON = 0;
    WITHER = 0;
    WARDEN = 0;
    VILLAGER = 0;
    WANDERING_TRADER = 0;
    Unknown = 0;
    PLAYER = 0;
    ITEM = 0;
    EXPERIENCE_ORB = 0;
    ARROW = 0;
    SPECTRAL_ARROW = 0;
    TRIDENT = 0;
    SPEAR = 0;
    SNOWBALL = 0;
    EGG = 0;
    ENDER_PEARL = 0;
    POTION = 0;
    EXPERIENCE_BOTTLE = 0;
    FIREBALL = 0;
    SMALL_FIREBALL = 0;
    DRAGON_FIREBALL = 0;
    WITHER_SKULL = 0;
    LLAMA_SPIT = 0;
    SHULKER_BULLET = 0;
    EVOKER_FANGS = 0;
    FISHING_BOBBER = 0;
    EYE_OF_ENDER = 0;
    FIREWORK_ROCKET = 0;
    WIND_CHARGE = 0;
    BOAT = 0;
    CHEST_BOAT = 0;
    MINECART = 0;
    CHEST_MINECART = 0;
    FURNACE_MINECART = 0;
    HOPPER_MINECART = 0;
    TNT_MINECART = 0;
    SPAWNER_MINECART = 0;
    FALLING_BLOCK = 0;
    TNT = 0;
    END_CRYSTAL = 0;
    LIGHTNING_BOLT = 0;
    AREA_EFFECT_CLOUD = 0;
    ARMOR_STAND = 0;
    OMINOUS_ITEM_SPAWNER = 0;
    PAINTING = 0;
    ITEM_FRAME = 0;
    LEASH_KNOT = 0;
}

} // namespace EntityTypeIdNumber
} // namespace entity
} // namespace mc
