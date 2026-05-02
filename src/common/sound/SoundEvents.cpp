#include "SoundEvents.hpp"

namespace mc {

namespace SoundEvents {

// ============================================================================
// 实体通用声音
// ============================================================================

const ResourceLocation ENTITY_GENERIC_EAT("minecraft:entity.generic.eat");
const ResourceLocation ENTITY_GENERIC_DRINK("minecraft:entity.generic.drink");
const ResourceLocation ENTITY_GENERIC_HURT("minecraft:entity.generic.hurt");
const ResourceLocation ENTITY_GENERIC_DEATH("minecraft:entity.generic.death");
const ResourceLocation ENTITY_GENERIC_BURN("minecraft:entity.generic.burn");
const ResourceLocation ENTITY_GENERIC_EXTINGUISH_FIRE("minecraft:entity.generic.extinguish_fire");
const ResourceLocation ENTITY_GENERIC_BIG_FALL("minecraft:entity.generic.big_fall");
const ResourceLocation ENTITY_GENERIC_SMALL_FALL("minecraft:entity.generic.small_fall");
const ResourceLocation ENTITY_GENERIC_SPLASH("minecraft:entity.generic.splash");
const ResourceLocation ENTITY_GENERIC_SWIM("minecraft:entity.generic.swim");

// ============================================================================
// 玩家声音
// ============================================================================

const ResourceLocation ENTITY_PLAYER_BURP("minecraft:entity.player.burp");
const ResourceLocation ENTITY_PLAYER_HURT("minecraft:entity.player.hurt");
const ResourceLocation ENTITY_PLAYER_DEATH("minecraft:entity.player.death");
const ResourceLocation ENTITY_PLAYER_SPLASH("minecraft:entity.player.splash");
const ResourceLocation ENTITY_PLAYER_SWIM("minecraft:entity.player.swim");
const ResourceLocation ENTITY_PLAYER_STEP("minecraft:entity.player.step");
const ResourceLocation ENTITY_PLAYER_ATTACK_SWEEP("minecraft:entity.player.attack.sweep");
const ResourceLocation ENTITY_PLAYER_ATTACK_CRIT("minecraft:entity.player.attack.crit");
const ResourceLocation ENTITY_PLAYER_ATTACK_KNOCKBACK("minecraft:entity.player.attack.knockback");
const ResourceLocation ENTITY_PLAYER_ATTACK_NODAMAGE("minecraft:entity.player.attack.nodamage");
const ResourceLocation ENTITY_PLAYER_BREATH("minecraft:entity.player.breath");
const ResourceLocation ENTITY_PLAYER_LEVELUP("minecraft:entity.player.levelup");

// ============================================================================
// 物品声音
// ============================================================================

const ResourceLocation ENTITY_ITEM_PICKUP("minecraft:entity.item.pickup");

// ============================================================================
// 武器声音
// ============================================================================

const ResourceLocation ENTITY_ARROW_SHOOT("minecraft:entity.arrow.shoot");
const ResourceLocation ITEM_BOW_PULL("minecraft:item.bow.pull");
const ResourceLocation ITEM_CROSSBOW_LOADING_START("minecraft:item.crossbow.loading_start");
const ResourceLocation ITEM_CROSSBOW_LOADING_MIDDLE("minecraft:item.crossbow.loading_middle");
const ResourceLocation ITEM_CROSSBOW_LOADING_END("minecraft:item.crossbow.loading_end");
const ResourceLocation ITEM_CROSSBOW_SHOOT("minecraft:item.crossbow.shoot");
const ResourceLocation ITEM_CROSSBOW_ROCKET("minecraft:item.crossbow.rocket");
const ResourceLocation ITEM_TRIDENT_THROW("minecraft:item.trident.throw");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_1("minecraft:item.trident.riptide_1");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_2("minecraft:item.trident.riptide_2");
const ResourceLocation ITEM_TRIDENT_RIPTIDE_3("minecraft:item.trident.riptide_3");
const ResourceLocation ITEM_TRIDENT_HIT("minecraft:item.trident.hit");
const ResourceLocation ITEM_TRIDENT_HIT_GROUND("minecraft:item.trident.hit_ground");
const ResourceLocation ITEM_SHIELD_BLOCK("minecraft:item.shield.block");
const ResourceLocation ITEM_SHIELD_BREAK("minecraft:item.shield.break");
const ResourceLocation ENTITY_FISHING_BOBBER_THROW("minecraft:entity.fishing_bobber.throw");
const ResourceLocation ENTITY_FISHING_BOBBER_RETRIEVE("minecraft:entity.fishing_bobber.retrieve");
const ResourceLocation ENTITY_FISHING_BOBBER_SPLASH("minecraft:entity.fishing_bobber.splash");
const ResourceLocation ENTITY_FISHING_BOBBER_CAST("minecraft:entity.fishing_bobber.cast");

// ============================================================================
// 环境声音
// ============================================================================

const ResourceLocation WEATHER_RAIN("minecraft:weather.rain");
const ResourceLocation WEATHER_THUNDER("minecraft:weather.thunder");

void initialize() {
    // 声音事件已通过静态初始化创建
    // 此函数可用于验证所有声音事件已正确初始化
}

} // namespace SoundEvents

} // namespace mc
