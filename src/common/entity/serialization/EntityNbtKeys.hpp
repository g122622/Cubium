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

/**
 * @brief 实体 NBT 序列化键名常量
 *
 * 所有键名与 Java 版保持一致，确保存档兼容性。
 * 参考：net.minecraft.entity.Entity.writeWithoutTypeId()
 */

namespace mc::entity::serialization::nbt_keys {

// ========== Entity 基础键 ==========

constexpr const char* ID = "id";
constexpr const char* UUID_MOST = "UUIDMost";
constexpr const char* UUID_LEAST = "UUIDLeast";

constexpr const char* POS = "Pos";
constexpr const char* MOTION = "Motion";
constexpr const char* ROTATION = "Rotation";

constexpr const char* FALL_DISTANCE = "FallDistance";
constexpr const char* FIRE = "Fire";
constexpr const char* AIR = "Air";
constexpr const char* ON_GROUND = "OnGround";
constexpr const char* INVULNERABLE = "Invulnerable";
constexpr const char* PORTAL_COOLDOWN = "PortalCooldown";

constexpr const char* CUSTOM_NAME = "CustomName";
constexpr const char* CUSTOM_NAME_VISIBLE = "CustomNameVisible";
constexpr const char* SILENT = "Silent";
constexpr const char* NO_GRAVITY = "NoGravity";
constexpr const char* GLOWING = "Glowing";

constexpr const char* TAGS = "Tags";
constexpr const char* PASSENGERS = "Passengers";

// ========== LivingEntity 键 ==========

constexpr const char* HEALTH = "Health";
constexpr const char* ABSORPTION_AMOUNT = "AbsorptionAmount";
constexpr const char* HURT_TIME = "HurtTime";
constexpr const char* HURT_BY_TIMESTAMP = "HurtByTimestamp";
constexpr const char* DEATH_TIME = "DeathTime";
constexpr const char* FALL_FLYING = "FallFlying";

constexpr const char* ACTIVE_EFFECTS = "ActiveEffects";
constexpr const char* ATTRIBUTES = "Attributes";

constexpr const char* HAND_ITEMS = "HandItems";
constexpr const char* ARMOR_ITEMS = "ArmorItems";
constexpr const char* HAND_DROP_CHANCES = "HandDropChances";
constexpr const char* ARMOR_DROP_CHANCES = "ArmorDropChances";
constexpr const char* DROP_CHANCES = "drop_chances";

constexpr const char* SLEEPING_X = "SleepingX";
constexpr const char* SLEEPING_Y = "SleepingY";
constexpr const char* SLEEPING_Z = "SleepingZ";

// ActiveEffect 子键
constexpr const char* EFFECT_ID = "Id";
constexpr const char* EFFECT_AMPLIFIER = "Amplifier";
constexpr const char* EFFECT_DURATION = "Duration";
constexpr const char* EFFECT_AMBIENT = "Ambient";
constexpr const char* EFFECT_SHOW_PARTICLES = "ShowParticles";
constexpr const char* EFFECT_SHOW_ICON = "ShowIcon";

// Attribute 子键
constexpr const char* ATTR_NAME = "Name";
constexpr const char* ATTR_BASE = "Base";
constexpr const char* ATTR_MODIFIERS = "Modifiers";
// AttributeModifier 子键
constexpr const char* ATTR_MOD_UUID_MOST = "UUIDMost";
constexpr const char* ATTR_MOD_UUID_LEAST = "UUIDLeast";
constexpr const char* ATTR_MOD_NAME = "Name";
constexpr const char* ATTR_MOD_OPERATION = "Operation";
constexpr const char* ATTR_MOD_AMOUNT = "Amount";

// ========== MobEntity 键 ==========

constexpr const char* CAN_PICK_UP_LOOT = "CanPickUpLoot";
constexpr const char* PERSISTENCE_REQUIRED = "PersistenceRequired";
constexpr const char* LEFT_HANDED = "LeftHanded";
constexpr const char* NO_AI = "NoAI";

constexpr const char* LEASH = "Leash";

// Leash 子键
constexpr const char* LEASH_UUID_MOST = "UUIDMost";
constexpr const char* LEASH_UUID_LEAST = "UUIDLeast";
constexpr const char* LEASH_X = "X";
constexpr const char* LEASH_Y = "Y";
constexpr const char* LEASH_Z = "Z";

constexpr const char* DEATH_LOOT_TABLE = "DeathLootTable";
constexpr const char* DEATH_LOOT_TABLE_SEED = "DeathLootTableSeed";

// ========== ItemEntity 键 ==========

constexpr const char* ITEM = "Item";
constexpr const char* AGE = "Age";
constexpr const char* PICKUP_DELAY = "PickupDelay";
constexpr const char* OWNER = "Owner";
constexpr const char* THROWER = "Thrower";

// ========== AgeableEntity 键 ==========

constexpr const char* FORCED_AGE = "ForcedAge";
constexpr const char* IN_LOVE = "InLove";

// ========== AnimalEntity 键 ==========

constexpr const char* LOVE_CAUSE = "LoveCause";

// ========== ZombieEntity 键 ==========

constexpr const char* IS_BABY = "IsBaby";
constexpr const char* CAN_BREAK_DOORS = "CanBreakDoors";
constexpr const char* DROWNED_CONVERSION_TIME = "DrownedConversionTime";
constexpr const char* IN_WATER_TIME = "InWaterTime";

// ========== CreeperEntity 键 ==========

constexpr const char* EXPLOSION_RADIUS = "ExplosionRadius";
constexpr const char* FUSE = "Fuse";
constexpr const char* IGNITED = "ignited";
constexpr const char* POWERED = "powered";

// ========== SkeletonEntity 键 ==========

constexpr const char* STRAY_CONVERSION_TIME = "StrayConversionTime";

// ========== ItemStack 子键 ==========

constexpr const char* ITEM_ID = "id";
constexpr const char* ITEM_COUNT = "Count";
constexpr const char* ITEM_TAG = "tag";

} // namespace mc::entity::serialization::nbt_keys
