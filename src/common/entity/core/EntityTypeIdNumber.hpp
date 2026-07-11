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

#pragma once

#include "EntityType.hpp"

namespace mc {
namespace entity {

/**
 * @brief 实体类型 ID 缓存命名空间
 *
 * 提供所有原版实体类型的 EntityTypeId 快速访问。
 * 这些 ID 在 VanillaEntities::registerAll() 后初始化。
 *
 * 用法：
 * @code
 * if (entity->typeId() == EntityTypeIdNumber::PIG) {
 *     // 处理猪
 * }
 * @endcode
 *
 * 注意：必须在调用 VanillaEntities::registerAll() 后才能使用。
 */
namespace EntityTypeIdNumber {

// ============================================================================
// 被动生物
// ============================================================================

// 普通动物
extern EntityTypeId PIG;
extern EntityTypeId COW;
extern EntityTypeId SHEEP;
extern EntityTypeId CHICKEN;
extern EntityTypeId RABBIT;
extern EntityTypeId MOOSHROOM;

// 可驯服动物
extern EntityTypeId WOLF;
extern EntityTypeId CAT;
extern EntityTypeId OCELOT;
extern EntityTypeId PARROT;

// 特殊动物
extern EntityTypeId FOX;
extern EntityTypeId PANDA;
extern EntityTypeId POLAR_BEAR;
extern EntityTypeId TURTLE;
extern EntityTypeId BEE;
extern EntityTypeId STRIDER;

// 马类
extern EntityTypeId HORSE;
extern EntityTypeId DONKEY;
extern EntityTypeId MULE;
extern EntityTypeId LLAMA;
extern EntityTypeId TRADER_LLAMA;
extern EntityTypeId SKELETON_HORSE;
extern EntityTypeId ZOMBIE_HORSE;

// 水生生物
extern EntityTypeId COD;
extern EntityTypeId SALMON;
extern EntityTypeId PUFFERFISH;
extern EntityTypeId TROPICAL_FISH;
extern EntityTypeId SQUID;
extern EntityTypeId GLOW_SQUID;
extern EntityTypeId DOLPHIN;
extern EntityTypeId AXOLOTL;
extern EntityTypeId NAUTILUS;
extern EntityTypeId ZOMBIE_NAUTILUS;

// 环境生物
extern EntityTypeId BAT;

// 傀儡
extern EntityTypeId IRON_GOLEM;
extern EntityTypeId SNOW_GOLEM;
extern EntityTypeId COPPER_GOLEM;

// ============================================================================
// 敌对生物
// ============================================================================

// 基础怪物
extern EntityTypeId ZOMBIE;
extern EntityTypeId SKELETON;
extern EntityTypeId CREEPER;
extern EntityTypeId SPIDER;
extern EntityTypeId ENDERMAN;
extern EntityTypeId BLAZE;
extern EntityTypeId WITCH;
extern EntityTypeId SLIME;
extern EntityTypeId GIANT;

// 海洋怪物
extern EntityTypeId GUARDIAN;
extern EntityTypeId ELDER_GUARDIAN;

// 亡灵变种
extern EntityTypeId HUSK;
extern EntityTypeId DROWNED;
extern EntityTypeId STRAY;
extern EntityTypeId WITHER_SKELETON;
extern EntityTypeId PHANTOM;
extern EntityTypeId ZOMBIE_VILLAGER;
extern EntityTypeId ZOMBIFIED_PIGLIN;

// 节肢动物变种
extern EntityTypeId CAVE_SPIDER;
extern EntityTypeId SILVERFISH;
extern EntityTypeId ENDERMITE;

// 末地生物
extern EntityTypeId SHULKER;

// 地狱生物
extern EntityTypeId GHAST;
extern EntityTypeId MAGMA_CUBE;
extern EntityTypeId PIGLIN;
extern EntityTypeId PIGLIN_BRUTE;
extern EntityTypeId HOGLIN;
extern EntityTypeId ZOGLIN;

// 灾厄村民
extern EntityTypeId VINDICATOR;
extern EntityTypeId EVOKER;
extern EntityTypeId ILLUSIONER;
extern EntityTypeId PILLAGER;
extern EntityTypeId RAVAGER;
extern EntityTypeId VEX;

// 试炼密室
extern EntityTypeId BREEZE;
// ============================================================================

extern EntityTypeId ENDER_DRAGON;
extern EntityTypeId WITHER;
extern EntityTypeId WARDEN;

// ============================================================================
// 村民
// ============================================================================

extern EntityTypeId VILLAGER;
extern EntityTypeId WANDERING_TRADER;

// ============================================================================
// 其他实体
// ============================================================================

extern EntityTypeId Unknown; // 未知实体类型，用于测试和默认值
extern EntityTypeId PLAYER;
extern EntityTypeId ITEM;
extern EntityTypeId EXPERIENCE_ORB;

// ============================================================================
// 投掷物
// ============================================================================

extern EntityTypeId ARROW;
extern EntityTypeId SPECTRAL_ARROW;
extern EntityTypeId TRIDENT;
extern EntityTypeId SPEAR;
extern EntityTypeId SNOWBALL;
extern EntityTypeId EGG;
extern EntityTypeId ENDER_PEARL;
extern EntityTypeId POTION;
extern EntityTypeId EXPERIENCE_BOTTLE;
extern EntityTypeId FIREBALL;
extern EntityTypeId SMALL_FIREBALL;
extern EntityTypeId DRAGON_FIREBALL;
extern EntityTypeId WITHER_SKULL;
extern EntityTypeId LLAMA_SPIT;
extern EntityTypeId SHULKER_BULLET;
extern EntityTypeId EVOKER_FANGS;
extern EntityTypeId FISHING_BOBBER;
extern EntityTypeId EYE_OF_ENDER;
extern EntityTypeId FIREWORK_ROCKET;
extern EntityTypeId WIND_CHARGE;
// ============================================================================

extern EntityTypeId BOAT;
extern EntityTypeId CHEST_BOAT;
extern EntityTypeId MINECART;
extern EntityTypeId CHEST_MINECART;
extern EntityTypeId FURNACE_MINECART;
extern EntityTypeId HOPPER_MINECART;
extern EntityTypeId TNT_MINECART;
extern EntityTypeId SPAWNER_MINECART;

// ============================================================================
// 其他实体
// ============================================================================

extern EntityTypeId FALLING_BLOCK;
extern EntityTypeId TNT;
extern EntityTypeId END_CRYSTAL;
extern EntityTypeId LIGHTNING_BOLT;
extern EntityTypeId AREA_EFFECT_CLOUD;
extern EntityTypeId ARMOR_STAND;
extern EntityTypeId OMINOUS_ITEM_SPAWNER;
extern EntityTypeId PAINTING;
extern EntityTypeId ITEM_FRAME;
extern EntityTypeId LEASH_KNOT;

/**
 * @brief 初始化所有实体类型 ID
 *
 * 从 EntityRegistry 读取已注册的实体类型 ID 并填充本命名空间的变量。
 * 此函数由 VanillaEntities::registerAll() 在注册完所有实体后调用。
 *
 * @warning 必须在 VanillaEntities::registerAll() 之后调用
 */
void initialize();

/**
 * @brief 将所有缓存的实体类型 ID 重置为 0（未初始化）。
 *
 * 与 initialize() 配对，由 EntityRegistry::clear() 调用以保证"注册表空 ⇔
 * ID 缓存全 0"的不变量。否则 clear() 仅清空注册表容器，而本命名空间的
 * extern 变量（如 ITEM=77）仍保留旧值，导致后续测试中 typeId()==0 与
 * EntityTypeIdNumber::ITEM=旧值 比较失败（测试顺序污染）。
 */
void reset();

} // namespace EntityTypeIdNumber

} // namespace entity
} // namespace mc
