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

#include "common/entity/core/EntityType.hpp"

namespace mc {
namespace entity {

/**
 * @brief 原版实体类型指针缓存命名空间
 *
 * 提供所有原版实体类型的 const EntityType* 指针别名快速访问。
 * 这些指针在 VanillaEntities::registerAll() 后由 initialize() 填充。
 *
 * 用法：
 * @code
 * if (entity->entityType() == VanillaEntityTypeKeys::PIG) {
 *     // 处理猪
 * }
 * @endcode
 *
 * 注意：必须在 VanillaEntities::registerAll() 后才能使用，指针指向注册表内部对象。
 */
namespace VanillaEntityTypeKeys {

// ============================================================================
// 被动生物
// ============================================================================

// 普通动物
extern const EntityType* PIG;
extern const EntityType* COW;
extern const EntityType* SHEEP;
extern const EntityType* CHICKEN;
extern const EntityType* RABBIT;
extern const EntityType* MOOSHROOM;

// 可驯服动物
extern const EntityType* WOLF;
extern const EntityType* CAT;
extern const EntityType* OCELOT;
extern const EntityType* PARROT;

// 特殊动物
extern const EntityType* FOX;
extern const EntityType* PANDA;
extern const EntityType* POLAR_BEAR;
extern const EntityType* TURTLE;
extern const EntityType* BEE;
extern const EntityType* STRIDER;
extern const EntityType* SNIFFER;

// 马类
extern const EntityType* HORSE;
extern const EntityType* DONKEY;
extern const EntityType* MULE;
extern const EntityType* LLAMA;
extern const EntityType* TRADER_LLAMA;
extern const EntityType* SKELETON_HORSE;
extern const EntityType* ZOMBIE_HORSE;

// 水生生物
extern const EntityType* COD;
extern const EntityType* SALMON;
extern const EntityType* PUFFERFISH;
extern const EntityType* TROPICAL_FISH;
extern const EntityType* SQUID;
extern const EntityType* GLOW_SQUID;
extern const EntityType* DOLPHIN;
extern const EntityType* AXOLOTL;
extern const EntityType* NAUTILUS;
extern const EntityType* ZOMBIE_NAUTILUS;

// 环境生物
extern const EntityType* BAT;

// 傀儡
extern const EntityType* IRON_GOLEM;
extern const EntityType* SNOW_GOLEM;
extern const EntityType* COPPER_GOLEM;

// ============================================================================
// 敌对生物
// ============================================================================

// 基础怪物
extern const EntityType* ZOMBIE;
extern const EntityType* SKELETON;
extern const EntityType* CREEPER;
extern const EntityType* SPIDER;
extern const EntityType* ENDERMAN;
extern const EntityType* BLAZE;
extern const EntityType* WITCH;
extern const EntityType* SLIME;
extern const EntityType* GIANT;

// 海洋怪物
extern const EntityType* GUARDIAN;
extern const EntityType* ELDER_GUARDIAN;

// 亡灵变种
extern const EntityType* HUSK;
extern const EntityType* DROWNED;
extern const EntityType* STRAY;
extern const EntityType* WITHER_SKELETON;
extern const EntityType* PHANTOM;
extern const EntityType* ZOMBIE_VILLAGER;
extern const EntityType* ZOMBIFIED_PIGLIN;

// 节肢动物变种
extern const EntityType* CAVE_SPIDER;
extern const EntityType* SILVERFISH;
extern const EntityType* ENDERMITE;

// 末地生物
extern const EntityType* SHULKER;

// 地狱生物
extern const EntityType* GHAST;
extern const EntityType* MAGMA_CUBE;
extern const EntityType* PIGLIN;
extern const EntityType* PIGLIN_BRUTE;
extern const EntityType* HOGLIN;
extern const EntityType* ZOGLIN;

// 灾厄村民
extern const EntityType* VINDICATOR;
extern const EntityType* EVOKER;
extern const EntityType* ILLUSIONER;
extern const EntityType* PILLAGER;
extern const EntityType* RAVAGER;
extern const EntityType* VEX;

// 试炼密室
extern const EntityType* BREEZE;
// ============================================================================

extern const EntityType* ENDER_DRAGON;
extern const EntityType* WITHER;
extern const EntityType* WARDEN;

// ============================================================================
// 村民
// ============================================================================

extern const EntityType* VILLAGER;
extern const EntityType* WANDERING_TRADER;

// ============================================================================
// 其他实体
// ============================================================================

extern const EntityType* Unknown; // 未知实体类型，用于测试和默认值
extern const EntityType* PLAYER;
extern const EntityType* ITEM;
extern const EntityType* EXPERIENCE_ORB;

// ============================================================================
// 投掷物
// ============================================================================

extern const EntityType* ARROW;
extern const EntityType* SPECTRAL_ARROW;
extern const EntityType* TRIDENT;
extern const EntityType* SPEAR;
extern const EntityType* SNOWBALL;
extern const EntityType* EGG;
extern const EntityType* ENDER_PEARL;
extern const EntityType* POTION;
extern const EntityType* EXPERIENCE_BOTTLE;
extern const EntityType* FIREBALL;
extern const EntityType* SMALL_FIREBALL;
extern const EntityType* DRAGON_FIREBALL;
extern const EntityType* WITHER_SKULL;
extern const EntityType* LLAMA_SPIT;
extern const EntityType* SHULKER_BULLET;
extern const EntityType* EVOKER_FANGS;
extern const EntityType* FISHING_BOBBER;
extern const EntityType* EYE_OF_ENDER;
extern const EntityType* FIREWORK_ROCKET;
extern const EntityType* WIND_CHARGE;
// ============================================================================

extern const EntityType* BOAT;
extern const EntityType* CHEST_BOAT;
extern const EntityType* MINECART;
extern const EntityType* CHEST_MINECART;
extern const EntityType* FURNACE_MINECART;
extern const EntityType* HOPPER_MINECART;
extern const EntityType* TNT_MINECART;
extern const EntityType* SPAWNER_MINECART;

// ============================================================================
// 其他实体
// ============================================================================

extern const EntityType* FALLING_BLOCK;
extern const EntityType* TNT;
extern const EntityType* END_CRYSTAL;
extern const EntityType* LIGHTNING_BOLT;
extern const EntityType* AREA_EFFECT_CLOUD;
extern const EntityType* ARMOR_STAND;
extern const EntityType* OMINOUS_ITEM_SPAWNER;
extern const EntityType* PAINTING;
extern const EntityType* ITEM_FRAME;
extern const EntityType* LEASH_KNOT;

/**
 * @brief 初始化所有实体类型 ID
 *
 * 从 EntityRegistry 读取已注册的实体类型 const EntityType* 指针并填充本命名空间的别名。
 * 此函数由 VanillaEntities::registerAll() 在注册完所有实体后调用。
 *
 * @warning 必须在 VanillaEntities::registerAll() 之后调用
 */
void initialize();

/**
 * @brief 将所有缓存的实体类型指针重置为 nullptr（未初始化）。
 *
 * 与 initialize() 配对，由 EntityRegistry::clear() 调用以保证"注册表空 ⇔
 * 指针缓存全 nullptr"的不变量。否则 clear() 仅清空注册表容器，而本命名空间的
 * extern 指针（如 ITEM）仍保留旧值，导致后续测试中 entityType()==nullptr 与
 * VanillaEntityTypeKeys::ITEM=旧值 比较失败（测试顺序污染）。
 */
void reset();

} // namespace VanillaEntityTypeKeys

} // namespace entity
} // namespace mc
