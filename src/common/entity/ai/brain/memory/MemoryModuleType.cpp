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

#include "MemoryModuleType.hpp"
#include "IPositionTarget.hpp"
#include "WalkTarget.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/pathfinding/Path.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

std::unordered_map<std::string, std::unique_ptr<MemoryModuleTypeBase>> MemoryModuleTypes::s_types;

// ========== 基础类型 ==========
const MemoryModuleType<void>* MemoryModuleTypes::DUMMY = nullptr;

// ========== 位置相关 ==========
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::HOME = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::JOB_SITE = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::POTENTIAL_JOB_SITE = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::MEETING_POINT = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::NEAREST_BED = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::HIDING_PLACE = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::CELEBRATE_LOCATION = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::NEAREST_REPELLENT = nullptr;
const MemoryModuleType<std::vector<GlobalPos>>* MemoryModuleTypes::SECONDARY_JOB_SITE = nullptr;

// ========== 实体列表相关 ==========
const MemoryModuleType<std::vector<LivingEntity*>>* MemoryModuleTypes::MOBS = nullptr;
const MemoryModuleType<std::vector<LivingEntity*>>* MemoryModuleTypes::VISIBLE_MOBS = nullptr;
const MemoryModuleType<std::vector<LivingEntity*>>* MemoryModuleTypes::VISIBLE_VILLAGER_BABIES = nullptr;
const MemoryModuleType<std::vector<Player*>>* MemoryModuleTypes::NEAREST_PLAYERS = nullptr;
const MemoryModuleType<Player*>* MemoryModuleTypes::NEAREST_VISIBLE_PLAYER = nullptr;
const MemoryModuleType<Player*>* MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::ATTACK_TARGET = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::INTERACTION_TARGET = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::HURT_BY_ENTITY = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::AVOID_TARGET = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::NEAREST_HOSTILE = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_ZOMBIFIED = nullptr;
const MemoryModuleType<AgeableEntity*>* MemoryModuleTypes::BREED_TARGET = nullptr;
const MemoryModuleType<AgeableEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_ADULT = nullptr;
const MemoryModuleType<Entity*>* MemoryModuleTypes::RIDE_TARGET = nullptr;
const MemoryModuleType<MobEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_NEMESIS = nullptr;
const MemoryModuleType<ItemEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM = nullptr;

// ========== 移动相关 ==========
const MemoryModuleType<pathfinding::Path>* MemoryModuleTypes::PATH = nullptr;
const MemoryModuleType<WalkTarget>* MemoryModuleTypes::WALK_TARGET = nullptr;
const MemoryModuleType<std::shared_ptr<IPositionTarget>>* MemoryModuleTypes::LOOK_TARGET = nullptr;

// ========== 门相关 ==========
const MemoryModuleType<std::vector<GlobalPos>>* MemoryModuleTypes::INTERACTABLE_DOORS = nullptr;
const MemoryModuleType<std::unordered_set<GlobalPos>>* MemoryModuleTypes::OPENED_DOORS = nullptr;

// ========== 战斗相关 ==========
const MemoryModuleType<bool>* MemoryModuleTypes::ATTACK_COOLING_DOWN = nullptr;
const MemoryModuleType<DamageSource*>* MemoryModuleTypes::HURT_BY = nullptr;

// ========== 时间相关 ==========
const MemoryModuleType<i64>* MemoryModuleTypes::HEARD_BELL_TIME = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_SLEPT = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_WOKEN = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_WORKED_AT_POI = nullptr;

// ========== 状态相关 ==========
const MemoryModuleType<bool>* MemoryModuleTypes::ADMIRING_ITEM = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::ADMIRING_DISABLED = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::HUNTED_RECENTLY = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::DANCING = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::ATE_RECENTLY = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::PACIFIED = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::GOLEM_DETECTED_RECENTLY = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::UNIVERSAL_ANGER = nullptr;

// ========== 计时器相关 ==========
const MemoryModuleType<i32>* MemoryModuleTypes::TIME_TRYING_TO_REACH_ADMIRE_ITEM = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::DISABLE_WALK_TO_ADMIRE_ITEM = nullptr;

// ========== 玩家相关 ==========
const MemoryModuleType<Player*>* MemoryModuleTypes::TEMPTING_PLAYER = nullptr;
const MemoryModuleType<Player*>* MemoryModuleTypes::NEAREST_PLAYER_HOLDING_WANTED_ITEM = nullptr;

// ========== UUID 相关 ==========
const MemoryModuleType<u64>* MemoryModuleTypes::ANGRY_AT = nullptr;

// ========== 猪灵/疣兽相关 ==========
const MemoryModuleType<HoglinEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_HUNTABLE_HOGLIN = nullptr;
const MemoryModuleType<HoglinEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_BABY_HOGLIN = nullptr;
const MemoryModuleType<Player*>* MemoryModuleTypes::NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD = nullptr;
const MemoryModuleType<std::vector<AbstractPiglinEntity*>>* MemoryModuleTypes::NEAREST_ADULT_PIGLINS = nullptr;
const MemoryModuleType<std::vector<AbstractPiglinEntity*>>* MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLINS = nullptr;
const MemoryModuleType<std::vector<HoglinEntity*>>* MemoryModuleTypes::NEAREST_VISIBLE_ADULT_HOGLINS = nullptr;
const MemoryModuleType<AbstractPiglinEntity*>* MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLIN = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::VISIBLE_ADULT_PIGLIN_COUNT = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::VISIBLE_ADULT_HOGLIN_COUNT = nullptr;

// ========== 扩展类型 ==========
const MemoryModuleType<bool>* MemoryModuleTypes::IS_IN_WATER = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::IS_PREGNANT = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::PLAY_DEAD = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::AGGRESSIVE = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::PLAY_DEAD_TICKS = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::TEMPTATION_COOLDOWN_TICKS = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::ITEM_PICKUP_COOLDOWN = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::CROPS_GROWTH = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::SKY_COOLDOWN = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::JUMP_COOLDOWN = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::LOVING_COOLDOWN = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::UNHAPPY_COUNTER = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::HOME_HOLDING_TICKS = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_ATTACKED_BY_PLAYER = nullptr;
const MemoryModuleType<std::unordered_set<GlobalPos>>* MemoryModuleTypes::DOORS_TO_CLOSE = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::OWNER_HURT_BY = nullptr;
const MemoryModuleType<LivingEntity*>* MemoryModuleTypes::OWNER_HURT_TARGET = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::LIKED_NOTEBLOCK = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::LISTENING_NOTEBLOCK = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::LIKED_NOTEBLOCK_COOLDOWN_TICKS = nullptr;
const MemoryModuleType<i32>* MemoryModuleTypes::LISTENING_NOTEBLOCK_COOLDOWN_TICKS = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::TONGUE_TARGET = nullptr;
const MemoryModuleType<Entity*>* MemoryModuleTypes::RAM_TARGET = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::SNIFFER_SNIFFING_TARGET = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::SNIFFER_DIGGING = nullptr;

void MemoryModuleTypes::initialize()
{
    // ========== 基础类型 ==========
    s_types["dummy"] = std::make_unique<MemoryModuleType<void>>("dummy");
    DUMMY = static_cast<const MemoryModuleType<void>*>(s_types["dummy"].get());

    // ========== 位置相关 ==========
    s_types["home"] = std::make_unique<MemoryModuleType<GlobalPos>>("home");
    HOME = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["home"].get());

    s_types["job_site"] = std::make_unique<MemoryModuleType<GlobalPos>>("job_site");
    JOB_SITE = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["job_site"].get());

    s_types["potential_job_site"] = std::make_unique<MemoryModuleType<GlobalPos>>("potential_job_site");
    POTENTIAL_JOB_SITE = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["potential_job_site"].get());

    s_types["meeting_point"] = std::make_unique<MemoryModuleType<GlobalPos>>("meeting_point");
    MEETING_POINT = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["meeting_point"].get());

    s_types["nearest_bed"] = std::make_unique<MemoryModuleType<BlockPos>>("nearest_bed");
    NEAREST_BED = static_cast<const MemoryModuleType<BlockPos>*>(s_types["nearest_bed"].get());

    s_types["hiding_place"] = std::make_unique<MemoryModuleType<BlockPos>>("hiding_place");
    HIDING_PLACE = static_cast<const MemoryModuleType<BlockPos>*>(s_types["hiding_place"].get());

    s_types["celebrate_location"] = std::make_unique<MemoryModuleType<BlockPos>>("celebrate_location");
    CELEBRATE_LOCATION = static_cast<const MemoryModuleType<BlockPos>*>(s_types["celebrate_location"].get());

    s_types["nearest_repellent"] = std::make_unique<MemoryModuleType<BlockPos>>("nearest_repellent");
    NEAREST_REPELLENT = static_cast<const MemoryModuleType<BlockPos>*>(s_types["nearest_repellent"].get());

    s_types["secondary_job_site"] = std::make_unique<MemoryModuleType<std::vector<GlobalPos>>>("secondary_job_site");
    SECONDARY_JOB_SITE =
        static_cast<const MemoryModuleType<std::vector<GlobalPos>>*>(s_types["secondary_job_site"].get());

    // ========== 实体列表相关 ==========
    s_types["mobs"] = std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("mobs");
    MOBS = static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["mobs"].get());

    s_types["visible_mobs"] = std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("visible_mobs");
    VISIBLE_MOBS = static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["visible_mobs"].get());

    s_types["visible_villager_babies"] =
        std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("visible_villager_babies");
    VISIBLE_VILLAGER_BABIES =
        static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["visible_villager_babies"].get());

    s_types["nearest_players"] = std::make_unique<MemoryModuleType<std::vector<Player*>>>("nearest_players");
    NEAREST_PLAYERS = static_cast<const MemoryModuleType<std::vector<Player*>>*>(s_types["nearest_players"].get());

    s_types["nearest_visible_player"] = std::make_unique<MemoryModuleType<Player*>>("nearest_visible_player");
    NEAREST_VISIBLE_PLAYER = static_cast<const MemoryModuleType<Player*>*>(s_types["nearest_visible_player"].get());

    s_types["nearest_visible_targetable_player"] =
        std::make_unique<MemoryModuleType<Player*>>("nearest_visible_targetable_player");
    NEAREST_VISIBLE_TARGETABLE_PLAYER =
        static_cast<const MemoryModuleType<Player*>*>(s_types["nearest_visible_targetable_player"].get());

    s_types["attack_target"] = std::make_unique<MemoryModuleType<LivingEntity*>>("attack_target");
    ATTACK_TARGET = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["attack_target"].get());

    s_types["interaction_target"] = std::make_unique<MemoryModuleType<LivingEntity*>>("interaction_target");
    INTERACTION_TARGET = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["interaction_target"].get());

    s_types["hurt_by_entity"] = std::make_unique<MemoryModuleType<LivingEntity*>>("hurt_by_entity");
    HURT_BY_ENTITY = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["hurt_by_entity"].get());

    s_types["avoid_target"] = std::make_unique<MemoryModuleType<LivingEntity*>>("avoid_target");
    AVOID_TARGET = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["avoid_target"].get());

    s_types["nearest_hostile"] = std::make_unique<MemoryModuleType<LivingEntity*>>("nearest_hostile");
    NEAREST_HOSTILE = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["nearest_hostile"].get());

    s_types["nearest_visible_zombified"] =
        std::make_unique<MemoryModuleType<LivingEntity*>>("nearest_visible_zombified");
    NEAREST_VISIBLE_ZOMBIFIED =
        static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["nearest_visible_zombified"].get());

    s_types["breed_target"] = std::make_unique<MemoryModuleType<AgeableEntity*>>("breed_target");
    BREED_TARGET = static_cast<const MemoryModuleType<AgeableEntity*>*>(s_types["breed_target"].get());

    s_types["nearest_visible_adult"] = std::make_unique<MemoryModuleType<AgeableEntity*>>("nearest_visible_adult");
    NEAREST_VISIBLE_ADULT =
        static_cast<const MemoryModuleType<AgeableEntity*>*>(s_types["nearest_visible_adult"].get());

    s_types["ride_target"] = std::make_unique<MemoryModuleType<Entity*>>("ride_target");
    RIDE_TARGET = static_cast<const MemoryModuleType<Entity*>*>(s_types["ride_target"].get());

    s_types["nearest_visible_nemesis"] = std::make_unique<MemoryModuleType<MobEntity*>>("nearest_visible_nemesis");
    NEAREST_VISIBLE_NEMESIS =
        static_cast<const MemoryModuleType<MobEntity*>*>(s_types["nearest_visible_nemesis"].get());

    s_types["nearest_visible_wanted_item"] =
        std::make_unique<MemoryModuleType<ItemEntity*>>("nearest_visible_wanted_item");
    NEAREST_VISIBLE_WANTED_ITEM =
        static_cast<const MemoryModuleType<ItemEntity*>*>(s_types["nearest_visible_wanted_item"].get());

    // ========== 移动相关 ==========
    s_types["path"] = std::make_unique<MemoryModuleType<pathfinding::Path>>("path");
    PATH = static_cast<const MemoryModuleType<pathfinding::Path>*>(s_types["path"].get());

    s_types["walk_target"] = std::make_unique<MemoryModuleType<WalkTarget>>("walk_target");
    WALK_TARGET = static_cast<const MemoryModuleType<WalkTarget>*>(s_types["walk_target"].get());

    s_types["look_target"] = std::make_unique<MemoryModuleType<std::shared_ptr<IPositionTarget>>>("look_target");
    LOOK_TARGET = static_cast<const MemoryModuleType<std::shared_ptr<IPositionTarget>>*>(s_types["look_target"].get());

    // ========== 门相关 ==========
    s_types["interactable_doors"] = std::make_unique<MemoryModuleType<std::vector<GlobalPos>>>("interactable_doors");
    INTERACTABLE_DOORS =
        static_cast<const MemoryModuleType<std::vector<GlobalPos>>*>(s_types["interactable_doors"].get());

    s_types["doors_to_close"] = std::make_unique<MemoryModuleType<std::unordered_set<GlobalPos>>>("doors_to_close");
    OPENED_DOORS = static_cast<const MemoryModuleType<std::unordered_set<GlobalPos>>*>(s_types["doors_to_close"].get());

    // ========== 战斗相关 ==========
    s_types["attack_cooling_down"] = std::make_unique<MemoryModuleType<bool>>("attack_cooling_down");
    ATTACK_COOLING_DOWN = static_cast<const MemoryModuleType<bool>*>(s_types["attack_cooling_down"].get());

    s_types["hurt_by"] = std::make_unique<MemoryModuleType<DamageSource*>>("hurt_by");
    HURT_BY = static_cast<const MemoryModuleType<DamageSource*>*>(s_types["hurt_by"].get());

    // ========== 时间相关 ==========
    s_types["heard_bell_time"] = std::make_unique<MemoryModuleType<i64>>("heard_bell_time");
    HEARD_BELL_TIME = static_cast<const MemoryModuleType<i64>*>(s_types["heard_bell_time"].get());

    s_types["cant_reach_walk_target_since"] = std::make_unique<MemoryModuleType<i64>>("cant_reach_walk_target_since");
    CANT_REACH_WALK_TARGET_SINCE =
        static_cast<const MemoryModuleType<i64>*>(s_types["cant_reach_walk_target_since"].get());

    s_types["last_slept"] = std::make_unique<MemoryModuleType<i64>>("last_slept");
    LAST_SLEPT = static_cast<const MemoryModuleType<i64>*>(s_types["last_slept"].get());

    s_types["last_woken"] = std::make_unique<MemoryModuleType<i64>>("last_woken");
    LAST_WOKEN = static_cast<const MemoryModuleType<i64>*>(s_types["last_woken"].get());

    s_types["last_worked_at_poi"] = std::make_unique<MemoryModuleType<i64>>("last_worked_at_poi");
    LAST_WORKED_AT_POI = static_cast<const MemoryModuleType<i64>*>(s_types["last_worked_at_poi"].get());

    // ========== 状态相关 ==========
    s_types["admiring_item"] = std::make_unique<MemoryModuleType<bool>>("admiring_item");
    ADMIRING_ITEM = static_cast<const MemoryModuleType<bool>*>(s_types["admiring_item"].get());

    s_types["admiring_disabled"] = std::make_unique<MemoryModuleType<bool>>("admiring_disabled");
    ADMIRING_DISABLED = static_cast<const MemoryModuleType<bool>*>(s_types["admiring_disabled"].get());

    s_types["hunted_recently"] = std::make_unique<MemoryModuleType<bool>>("hunted_recently");
    HUNTED_RECENTLY = static_cast<const MemoryModuleType<bool>*>(s_types["hunted_recently"].get());

    s_types["dancing"] = std::make_unique<MemoryModuleType<bool>>("dancing");
    DANCING = static_cast<const MemoryModuleType<bool>*>(s_types["dancing"].get());

    s_types["ate_recently"] = std::make_unique<MemoryModuleType<bool>>("ate_recently");
    ATE_RECENTLY = static_cast<const MemoryModuleType<bool>*>(s_types["ate_recently"].get());

    s_types["pacified"] = std::make_unique<MemoryModuleType<bool>>("pacified");
    PACIFIED = static_cast<const MemoryModuleType<bool>*>(s_types["pacified"].get());

    s_types["golem_detected_recently"] = std::make_unique<MemoryModuleType<bool>>("golem_detected_recently");
    GOLEM_DETECTED_RECENTLY = static_cast<const MemoryModuleType<bool>*>(s_types["golem_detected_recently"].get());

    s_types["universal_anger"] = std::make_unique<MemoryModuleType<bool>>("universal_anger");
    UNIVERSAL_ANGER = static_cast<const MemoryModuleType<bool>*>(s_types["universal_anger"].get());

    // ========== 计时器相关 ==========
    s_types["time_trying_to_reach_admire_item"] =
        std::make_unique<MemoryModuleType<i32>>("time_trying_to_reach_admire_item");
    TIME_TRYING_TO_REACH_ADMIRE_ITEM =
        static_cast<const MemoryModuleType<i32>*>(s_types["time_trying_to_reach_admire_item"].get());

    s_types["disable_walk_to_admire_item"] = std::make_unique<MemoryModuleType<bool>>("disable_walk_to_admire_item");
    DISABLE_WALK_TO_ADMIRE_ITEM =
        static_cast<const MemoryModuleType<bool>*>(s_types["disable_walk_to_admire_item"].get());

    // ========== 玩家相关 ==========
    s_types["tempting_player"] = std::make_unique<MemoryModuleType<Player*>>("tempting_player");
    TEMPTING_PLAYER = static_cast<const MemoryModuleType<Player*>*>(s_types["tempting_player"].get());

    s_types["nearest_player_holding_wanted_item"] =
        std::make_unique<MemoryModuleType<Player*>>("nearest_player_holding_wanted_item");
    NEAREST_PLAYER_HOLDING_WANTED_ITEM =
        static_cast<const MemoryModuleType<Player*>*>(s_types["nearest_player_holding_wanted_item"].get());

    // ========== UUID 相关 ==========
    s_types["angry_at"] = std::make_unique<MemoryModuleType<u64>>("angry_at");
    ANGRY_AT = static_cast<const MemoryModuleType<u64>*>(s_types["angry_at"].get());

    // ========== 猪灵/疣兽相关 ==========
    s_types["nearest_visible_huntable_hoglin"] =
        std::make_unique<MemoryModuleType<HoglinEntity*>>("nearest_visible_huntable_hoglin");
    NEAREST_VISIBLE_HUNTABLE_HOGLIN =
        static_cast<const MemoryModuleType<HoglinEntity*>*>(s_types["nearest_visible_huntable_hoglin"].get());

    s_types["nearest_visible_baby_hoglin"] =
        std::make_unique<MemoryModuleType<HoglinEntity*>>("nearest_visible_baby_hoglin");
    NEAREST_VISIBLE_BABY_HOGLIN =
        static_cast<const MemoryModuleType<HoglinEntity*>*>(s_types["nearest_visible_baby_hoglin"].get());

    s_types["nearest_targetable_player_not_wearing_gold"] =
        std::make_unique<MemoryModuleType<Player*>>("nearest_targetable_player_not_wearing_gold");
    NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD =
        static_cast<const MemoryModuleType<Player*>*>(s_types["nearest_targetable_player_not_wearing_gold"].get());

    s_types["nearby_adult_piglins"] =
        std::make_unique<MemoryModuleType<std::vector<AbstractPiglinEntity*>>>("nearby_adult_piglins");
    NEAREST_ADULT_PIGLINS =
        static_cast<const MemoryModuleType<std::vector<AbstractPiglinEntity*>>*>(s_types["nearby_adult_piglins"].get());

    s_types["nearest_visible_adult_piglins"] =
        std::make_unique<MemoryModuleType<std::vector<AbstractPiglinEntity*>>>("nearest_visible_adult_piglins");
    NEAREST_VISIBLE_ADULT_PIGLINS = static_cast<const MemoryModuleType<std::vector<AbstractPiglinEntity*>>*>(
        s_types["nearest_visible_adult_piglins"].get());

    s_types["nearest_visible_adult_hoglins"] =
        std::make_unique<MemoryModuleType<std::vector<HoglinEntity*>>>("nearest_visible_adult_hoglins");
    NEAREST_VISIBLE_ADULT_HOGLINS = static_cast<const MemoryModuleType<std::vector<HoglinEntity*>>*>(
        s_types["nearest_visible_adult_hoglins"].get());

    s_types["nearest_visible_adult_piglin"] =
        std::make_unique<MemoryModuleType<AbstractPiglinEntity*>>("nearest_visible_adult_piglin");
    NEAREST_VISIBLE_ADULT_PIGLIN =
        static_cast<const MemoryModuleType<AbstractPiglinEntity*>*>(s_types["nearest_visible_adult_piglin"].get());

    s_types["visible_adult_piglin_count"] = std::make_unique<MemoryModuleType<i32>>("visible_adult_piglin_count");
    VISIBLE_ADULT_PIGLIN_COUNT = static_cast<const MemoryModuleType<i32>*>(s_types["visible_adult_piglin_count"].get());

    s_types["visible_adult_hoglin_count"] = std::make_unique<MemoryModuleType<i32>>("visible_adult_hoglin_count");
    VISIBLE_ADULT_HOGLIN_COUNT = static_cast<const MemoryModuleType<i32>*>(s_types["visible_adult_hoglin_count"].get());

    // ========== 扩展类型 ==========
    s_types["is_in_water"] = std::make_unique<MemoryModuleType<bool>>("is_in_water");
    IS_IN_WATER = static_cast<const MemoryModuleType<bool>*>(s_types["is_in_water"].get());

    s_types["is_pregnant"] = std::make_unique<MemoryModuleType<bool>>("is_pregnant");
    IS_PREGNANT = static_cast<const MemoryModuleType<bool>*>(s_types["is_pregnant"].get());

    s_types["play_dead"] = std::make_unique<MemoryModuleType<bool>>("play_dead");
    PLAY_DEAD = static_cast<const MemoryModuleType<bool>*>(s_types["play_dead"].get());

    s_types["aggressive"] = std::make_unique<MemoryModuleType<bool>>("aggressive");
    AGGRESSIVE = static_cast<const MemoryModuleType<bool>*>(s_types["aggressive"].get());

    s_types["play_dead_ticks"] = std::make_unique<MemoryModuleType<i32>>("play_dead_ticks");
    PLAY_DEAD_TICKS = static_cast<const MemoryModuleType<i32>*>(s_types["play_dead_ticks"].get());

    s_types["temptation_cooldown_ticks"] = std::make_unique<MemoryModuleType<i32>>("temptation_cooldown_ticks");
    TEMPTATION_COOLDOWN_TICKS = static_cast<const MemoryModuleType<i32>*>(s_types["temptation_cooldown_ticks"].get());

    s_types["item_pickup_cooldown"] = std::make_unique<MemoryModuleType<i32>>("item_pickup_cooldown");
    ITEM_PICKUP_COOLDOWN = static_cast<const MemoryModuleType<i32>*>(s_types["item_pickup_cooldown"].get());

    s_types["crops_growth"] = std::make_unique<MemoryModuleType<i32>>("crops_growth");
    CROPS_GROWTH = static_cast<const MemoryModuleType<i32>*>(s_types["crops_growth"].get());

    s_types["sky_cooldown"] = std::make_unique<MemoryModuleType<i32>>("sky_cooldown");
    SKY_COOLDOWN = static_cast<const MemoryModuleType<i32>*>(s_types["sky_cooldown"].get());

    s_types["jump_cooldown"] = std::make_unique<MemoryModuleType<i32>>("jump_cooldown");
    JUMP_COOLDOWN = static_cast<const MemoryModuleType<i32>*>(s_types["jump_cooldown"].get());

    s_types["loving_cooldown"] = std::make_unique<MemoryModuleType<i32>>("loving_cooldown");
    LOVING_COOLDOWN = static_cast<const MemoryModuleType<i32>*>(s_types["loving_cooldown"].get());

    s_types["unhappy_counter"] = std::make_unique<MemoryModuleType<i32>>("unhappy_counter");
    UNHAPPY_COUNTER = static_cast<const MemoryModuleType<i32>*>(s_types["unhappy_counter"].get());

    s_types["home_holding_ticks"] = std::make_unique<MemoryModuleType<i32>>("home_holding_ticks");
    HOME_HOLDING_TICKS = static_cast<const MemoryModuleType<i32>*>(s_types["home_holding_ticks"].get());

    s_types["last_attacked_by_player"] = std::make_unique<MemoryModuleType<i64>>("last_attacked_by_player");
    LAST_ATTACKED_BY_PLAYER = static_cast<const MemoryModuleType<i64>*>(s_types["last_attacked_by_player"].get());

    s_types["doors_to_close_ext"] =
        std::make_unique<MemoryModuleType<std::unordered_set<GlobalPos>>>("doors_to_close_ext");
    DOORS_TO_CLOSE =
        static_cast<const MemoryModuleType<std::unordered_set<GlobalPos>>*>(s_types["doors_to_close_ext"].get());

    s_types["owner_hurt_by"] = std::make_unique<MemoryModuleType<LivingEntity*>>("owner_hurt_by");
    OWNER_HURT_BY = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["owner_hurt_by"].get());

    s_types["owner_hurt_target"] = std::make_unique<MemoryModuleType<LivingEntity*>>("owner_hurt_target");
    OWNER_HURT_TARGET = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["owner_hurt_target"].get());

    // 1.17+ 扩展
    s_types["liked_noteblock"] = std::make_unique<MemoryModuleType<GlobalPos>>("liked_noteblock");
    LIKED_NOTEBLOCK = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["liked_noteblock"].get());

    s_types["listening_noteblock"] = std::make_unique<MemoryModuleType<GlobalPos>>("listening_noteblock");
    LISTENING_NOTEBLOCK = static_cast<const MemoryModuleType<GlobalPos>*>(s_types["listening_noteblock"].get());

    s_types["liked_noteblock_cooldown_ticks"] =
        std::make_unique<MemoryModuleType<i32>>("liked_noteblock_cooldown_ticks");
    LIKED_NOTEBLOCK_COOLDOWN_TICKS =
        static_cast<const MemoryModuleType<i32>*>(s_types["liked_noteblock_cooldown_ticks"].get());

    s_types["listening_noteblock_cooldown_ticks"] =
        std::make_unique<MemoryModuleType<i32>>("listening_noteblock_cooldown_ticks");
    LISTENING_NOTEBLOCK_COOLDOWN_TICKS =
        static_cast<const MemoryModuleType<i32>*>(s_types["listening_noteblock_cooldown_ticks"].get());

    // 1.17+ 青蛙/山羊
    s_types["tongue_target"] = std::make_unique<MemoryModuleType<BlockPos>>("tongue_target");
    TONGUE_TARGET = static_cast<const MemoryModuleType<BlockPos>*>(s_types["tongue_target"].get());

    s_types["ram_target"] = std::make_unique<MemoryModuleType<Entity*>>("ram_target");
    RAM_TARGET = static_cast<const MemoryModuleType<Entity*>*>(s_types["ram_target"].get());

    // 1.19+ Sniffer
    s_types["sniffer_sniffing_target"] = std::make_unique<MemoryModuleType<BlockPos>>("sniffer_sniffing_target");
    SNIFFER_SNIFFING_TARGET = static_cast<const MemoryModuleType<BlockPos>*>(s_types["sniffer_sniffing_target"].get());

    s_types["sniffer_digging"] = std::make_unique<MemoryModuleType<bool>>("sniffer_digging");
    SNIFFER_DIGGING = static_cast<const MemoryModuleType<bool>*>(s_types["sniffer_digging"].get());
}

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
