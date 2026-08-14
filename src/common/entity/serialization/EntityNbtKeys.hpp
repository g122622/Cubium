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
constexpr const char* TICKS_FROZEN = "TicksFrozen";
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
constexpr const char* EQUIPMENT = "equipment";
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

// ========== AbstractHorseEntity 键 ==========

constexpr const char* TEMPER = "Temper";
constexpr const char* HORSE_JUMP_STRENGTH = "JumpStrength";
constexpr const char* HORSE_SPEED = "Speed";
constexpr const char* HORSE_HEALTH = "HorseHealth";
constexpr const char* EATING_HAYSTACK = "EatingHaystack";
constexpr const char* HORSE_OWNER_UUID_MOST = "OwnerUUIDMost";
constexpr const char* HORSE_OWNER_UUID_LEAST = "OwnerUUIDLeast";

// ========== TameableEntity 键 ==========

constexpr const char* SITTING = "Sitting";
constexpr const char* OWNER_UUID = "OwnerUUID";
constexpr const char* ANGER_TIME = "AngerTime";

// ========== OcelotEntity 键 ==========

constexpr const char* TRUSTING = "Trusting";

// ========== CatEntity 键 ==========

constexpr const char* CAT_TYPE = "CatType";
constexpr const char* COLLAR_COLOR = "CollarColor";

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

// ========== AreaEffectCloudEntity 键 ==========

constexpr const char* CLOUD_AGE = "Age";
constexpr const char* CLOUD_DURATION = "Duration";
constexpr const char* CLOUD_WAIT_TIME = "WaitTime";
constexpr const char* CLOUD_REAPPLICATION_DELAY = "ReapplicationDelay";
constexpr const char* CLOUD_DURATION_ON_USE = "DurationOnUse";
constexpr const char* CLOUD_RADIUS_ON_USE = "RadiusOnUse";
constexpr const char* CLOUD_RADIUS_PER_TICK = "RadiusPerTick";
constexpr const char* CLOUD_RADIUS = "Radius";
constexpr const char* CLOUD_OWNER = "Owner";
constexpr const char* CLOUD_PARTICLE = "ParticleType";
constexpr const char* CLOUD_COLOR = "Color";
constexpr const char* CLOUD_EFFECTS = "CustomPotionEffects";

// ========== EvokerFangsEntity 键 ==========
// 参考 MC 1.21.11 EvokerFangs.addAdditionalSaveData()，NBT 键名为 "Warmup" 和 "Owner"
// Owner UUID 使用 OwnerUUIDMost/OwnerUUIDLeast 双 long 格式存储
// 注意：MC 1.21.11 原版使用 int[4] 格式存储 "Owner" 键，
// 但为与项目现有模式（AreaEffectCloudEntity）保持一致，采用 OwnerUUIDMost/OwnerUUIDLeast 格式

constexpr const char* WARMUP = "Warmup";
constexpr const char* FANGS_OWNER_UUID_MOST = "OwnerUUIDMost";
constexpr const char* FANGS_OWNER_UUID_LEAST = "OwnerUUIDLeast";

// ========== FireworkRocketEntity 键 ==========
// 参考 MC 1.21.11 FireworkRocketEntity.addAdditionalSaveData()/readAdditionalSaveData()
// 持久化烟花物品、已存在时间、总生命时间、是否从弩射出标记

constexpr const char* FIREWORKS_ITEM = "FireworksItem"; ///< 烟花物品（compound，由 ItemStack::toNbt 写入）
constexpr const char* LIFE = "Life";                    ///< 已存在时间（i32，每 tick 递增）
constexpr const char* LIFE_TIME = "LifeTime";           ///< 总生命时间（i32，创建时一次性随机确定）
constexpr const char* SHOT_AT_ANGLE = "ShotAtAngle";    ///< 是否从弩射出（i8 bool，对应 m_shotFromCrossbow）

// ========== Projectile 族通用键 ==========
// 参考 MC 1.21.11 Projectile.addAdditionalSaveData()/readAdditionalSaveData()。
// owner UUID 格式说明：vanilla 1.21.11 已改用 EntityReference 单一 "Owner" 键（int[4] 或 UUID）。
// 项目沿用 OwnerUUIDMost/OwnerUUIDLeast 双 long 格式（与既有 EvokerFangs/AreaEffectCloud 一致，
// 零迁移成本），此为项目既有存档约定，非 vanilla 原版格式。

constexpr const char* PROJECTILE_OWNER_UUID_MOST = "OwnerUUIDMost";   ///< 投掷物发射者 UUID 高 64 位
constexpr const char* PROJECTILE_OWNER_UUID_LEAST = "OwnerUUIDLeast"; ///< 投掷物发射者 UUID 低 64 位
constexpr const char* PROJECTILE_LEFT_OWNER = "LeftOwner";            ///< 是否已离开发射者碰撞箱（bool）
constexpr const char* PROJECTILE_HAS_BEEN_SHOT = "HasBeenShot";       ///< 是否已发射（bool）

// ========== AbstractArrow 键 ==========
// 参考 MC 1.21.11 AbstractArrow.addAdditionalSaveData()/readAdditionalSaveData()。
// 项目 AbstractArrow 持久化 8 字段（vanilla 11 字段中 SoundEvent/weapon 标 TODO 暂不持久化）。

constexpr const char* ARROW_LIFE = "life";                ///< 在地里存活 tick（short）
constexpr const char* ARROW_SHAKE = "shake";              ///< 抖动时间（byte）
constexpr const char* ARROW_IN_GROUND = "inGround";       ///< 是否插在方块中（bool）
constexpr const char* ARROW_PICKUP = "pickup";            ///< 拾取状态（byte：0/1/2）
constexpr const char* ARROW_DAMAGE = "damage";            ///< 基础伤害（float）
constexpr const char* ARROW_CRIT = "crit";                ///< 是否暴击（bool）
constexpr const char* ARROW_PIERCE_LEVEL = "PierceLevel"; ///< 穿透等级（byte）
constexpr const char* ARROW_ITEM = "item";                ///< 拾取物品堆（compound，ItemStack::toNbt）
// TODO: ARROW_SOUND_EVENT / ARROW_WEAPON（vanilla 持久化，项目暂无对应字段，待补）

// ========== ThrownTrident 键 ==========
// 参考 MC 1.21.11 ThrownTrident.addAdditionalSaveData()/readAdditionalSaveData()。
// loyalty 不存盘（从 item 忠诚附魔重算）；DealtDamage 存盘。

constexpr const char* TRIDENT_ITEM = "Trident";             ///< 三叉戟物品（compound，ItemStack::toNbt）
constexpr const char* TRIDENT_DEALT_DAMAGE = "DealtDamage"; ///< 是否已造成伤害（bool）

// ========== Fireball 族键 ==========
// 参考 MC 1.21.11 Fireball.addAdditionalSaveData()(Item) / WitherSkull(dangerous)。
// 项目 FireballStateComponent 为 Fireball+WitherSkull 共用：m_explosionPower(Fireball)/m_blue(WitherSkull)。
// 注意：vanilla Fireball 存 Item，ExplosionPower 是 LargeFireball(恶魂) 的键；项目 FireballEntity
// 用 m_explosionPower 作爆炸威力，此为项目既有设计差异，沿用项目现状持久化 explosionPower。

constexpr const char* FIREBALL_EXPLOSION_POWER = "ExplosionPower"; ///< 火球爆炸威力（byte，项目用 i32）
constexpr const char* WITHER_SKULL_DANGEROUS = "dangerous";        ///< 凋灵之首是否蓝色（bool）

// ========== DamagingProjectile 键 ==========
// 参考 MC 1.21.11 AbstractHurtingProjectile.addAdditionalSaveData()(acceleration_power)。

constexpr const char* ACCELERATION_POWER = "acceleration_power"; ///< 加速力（double，vanilla 默认 0.1）

// ========== ShulkerBullet 键 ==========
// 参考 MC 1.21.11 ShulkerBullet.addAdditionalSaveData()/readAdditionalSaveData()。

constexpr const char* SHULKER_BULLET_TARGET = "Target"; ///< 目标实体 UUID
constexpr const char* SHULKER_BULLET_DIR = "Dir";       ///< 当前移动方向（byte，Direction legacy id）
constexpr const char* SHULKER_BULLET_STEPS = "Steps";   ///< 飞行步数（int）
constexpr const char* SHULKER_BULLET_TXD = "TXD";       ///< 目标增量 X（double）
constexpr const char* SHULKER_BULLET_TYD = "TYD";       ///< 目标增量 Y（double）
constexpr const char* SHULKER_BULLET_TZD = "TZD";       ///< 目标增量 Z（double）

// ========== EyeOfEnder 键 ==========
// 参考 MC 1.21.11 EyeOfEnder.addAdditionalSaveData()(Item，不调 super 故不存 Owner)。
// 项目 EyeOfEnderEntity 直接继承 Entity 无 ProjectileOwnerComponent，与 vanilla 断链语义一致。

constexpr const char* EYE_OF_ENDER_ITEM = "Item"; ///< 末影之眼物品（compound，ItemStack::toNbt）

// ========== Player 键 ==========

constexpr const char* PLAYER_GAME_TYPE = "playerGameType";
constexpr const char* INVENTORY = "Inventory";
constexpr const char* SELECTED_ITEM_SLOT = "SelectedItemSlot";
constexpr const char* SCORE = "Score";
constexpr const char* FOOD_LEVEL = "foodLevel";
constexpr const char* FOOD_SATURATION_LEVEL = "foodSaturationLevel";
constexpr const char* FOOD_EXHAUSTION_LEVEL = "foodExhaustionLevel";
constexpr const char* FOOD_TICK_TIMER = "foodTickTimer";
constexpr const char* XP_LEVEL = "XpLevel";
constexpr const char* XP_P = "XpP";
constexpr const char* XP_TOTAL = "XpTotal";
constexpr const char* XP_SEED = "XpSeed";
constexpr const char* XP_COOLDOWN = "XpCooldown";
constexpr const char* ABILITIES = "abilities";
constexpr const char* ABILITIES_INVULNERABLE = "invulnerable";
constexpr const char* ABILITIES_FLYING = "flying";
constexpr const char* ABILITIES_MAY_FLY = "mayfly";
constexpr const char* ABILITIES_INSTABUILD = "instabuild";
constexpr const char* ABILITIES_MAY_BUILD = "mayBuild";
constexpr const char* ABILITIES_FLY_SPEED = "flySpeed";
constexpr const char* ABILITIES_WALK_SPEED = "walkSpeed";
constexpr const char* SPAWN_X = "SpawnX";
constexpr const char* SPAWN_Y = "SpawnY";
constexpr const char* SPAWN_Z = "SpawnZ";
constexpr const char* SPAWN_FORCED = "SpawnForced";
constexpr const char* SPAWN_DIM = "SpawnDimension";
constexpr const char* ENTERED_NETHER_POSITION = "enteredNetherPosition";
constexpr const char* CURRENT_EXPLOSION_IMPACT_POS = "current_explosion_impact_pos";
constexpr const char* IGNORE_FALL_DAMAGE_FROM_CURRENT_EXPLOSION = "ignore_fall_damage_from_current_explosion";
constexpr const char* CURRENT_IMPULSE_CONTEXT_RESET_GRACE_TIME = "current_impulse_context_reset_grace_time";
constexpr const char* ENDER_ITEMS = "EnderItems";
constexpr const char* LAST_DEATH_LOCATION = "LastDeathLocation";
constexpr const char* LAST_DEATH_LOCATION_DIMENSION = "dimension"; ///< LastDeathLocation 子键：维度名称
constexpr const char* LAST_DEATH_LOCATION_POS = "pos"; ///< LastDeathLocation 子键：方块位置（int 列表 [x, y, z]）
constexpr const char* SLEEP_TIMER = "SleepTimer";

// ========== SkeletonEntity 键 ==========

constexpr const char* STRAY_CONVERSION_TIME = "StrayConversionTime";

// ========== TurtleEntity 键 ==========

constexpr const char* HOME_X = "HomePosX";
constexpr const char* HOME_Y = "HomePosY";
constexpr const char* HOME_Z = "HomePosZ";
constexpr const char* HAS_EGG = "HasEgg";

// ========== TraderLlamaEntity 键 ==========

constexpr const char* DESPAWN_DELAY = "DespawnDelay";

// ========== Vehicle 容器通用键 ==========

constexpr const char* ITEMS = "Items";          ///< 容器物品列表（用于 ChestBoatEntity、ChestMinecartEntity 等）
constexpr const char* LOOT_TABLE = "LootTable"; ///< 战利品表ID（用于延迟填充容器）
constexpr const char* LOOT_TABLE_SEED = "LootTableSeed"; ///< 战利品表种子

// ========== ItemStack 子键 ==========

constexpr const char* ITEM_ID = "id";
constexpr const char* ITEM_COUNT = "Count";
constexpr const char* ITEM_TAG = "tag";

// ========== GlowSquidEntity 键 ==========

constexpr const char* DARK_TICKS_REMAINING = "DarkTicksRemaining"; ///< 剩余暗化 tick 数（i32）

// ========== CopperGolemEntity 键 ==========
// 对应 MC 1.21.11 CopperGolem.addAdditionalSaveData/readAdditionalSaveData
// 注意：MC 仅持久化 weatherState 与 nextWeatheringTick，behaviorState 为运行时动画状态不持久化

constexpr const char* NEXT_WEATHER_AGE =
    "next_weather_age";                                ///< 下次氧化 tick（i64，-2=涂蜡，-1=未设置，>=0=绝对 tick）
constexpr const char* WEATHER_STATE = "weather_state"; ///< 氧化等级（字符串：unaffected/exposed/weathered/oxidized）

// ========== SnifferEntity 键 ==========
// 对应 MC 1.21.11 Sniffer.addAdditionalSaveData/readAdditionalSaveData

constexpr const char* SNIFFER_STATE = "state";                         ///< 嗅探兽状态机当前状态（i8，0-6）
constexpr const char* SNIFFER_DROP_SEED_AT_TICK = "drop_seed_at_tick"; ///< 挖掘掉落种子的 tick（i32）

} // namespace mc::entity::serialization::nbt_keys
