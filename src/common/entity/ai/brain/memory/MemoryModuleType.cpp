#include "MemoryModuleType.hpp"
#include "../../../../world/GlobalPos.hpp"
#include "../../../../world/block/BlockPos.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

std::unordered_map<std::string, std::unique_ptr<MemoryModuleTypeBase>> MemoryModuleTypes::s_types;

// 基础类型 - 初始化为nullptr，在initialize()中赋值
const MemoryModuleType<void>* MemoryModuleTypes::DUMMY = nullptr;

// 位置相关
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::HOME = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::JOB_SITE = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::POTENTIAL_JOB_SITE = nullptr;
const MemoryModuleType<GlobalPos>* MemoryModuleTypes::MEETING_POINT = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::NEAREST_BED = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::HIDING_PLACE = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::CELEBRATE_LOCATION = nullptr;
const MemoryModuleType<BlockPos>* MemoryModuleTypes::NEAREST_REPELLENT = nullptr;

// 实体相关
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

// 移动相关
const MemoryModuleType<Path>* MemoryModuleTypes::PATH = nullptr;
const MemoryModuleType<void>* MemoryModuleTypes::WALK_TARGET = nullptr;
const MemoryModuleType<void>* MemoryModuleTypes::LOOK_TARGET = nullptr;

// 战斗相关
const MemoryModuleType<bool>* MemoryModuleTypes::ATTACK_COOLING_DOWN = nullptr;
const MemoryModuleType<DamageSource*>* MemoryModuleTypes::HURT_BY = nullptr;

// 时间相关
const MemoryModuleType<i64>* MemoryModuleTypes::HEARD_BELL_TIME = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_SLEPT = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_WOKEN = nullptr;
const MemoryModuleType<i64>* MemoryModuleTypes::LAST_WORKED_AT_POI = nullptr;

// 状态相关
const MemoryModuleType<bool>* MemoryModuleTypes::ADMIRING_ITEM = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::ADMIRING_DISABLED = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::HUNTED_RECENTLY = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::DANCING = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::ATE_RECENTLY = nullptr;
const MemoryModuleType<bool>* MemoryModuleTypes::PACIFIED = nullptr;

void MemoryModuleTypes::initialize() {
    // 使用lambda辅助函数来简化代码
    auto initType = [](const char* name, auto*& outPtr) {
        using T = std::remove_pointer_t<decltype(outPtr)>::element_type;
        auto uptr = std::make_unique<MemoryModuleType<typename std::remove_pointer_t<decltype(outPtr)>::element_type::value_type>>(name);
        outPtr = static_cast<decltype(outPtr)>(uptr.get());
        s_types[name] = std::move(uptr);
    };

    // 基础类型
    s_types["dummy"] = std::make_unique<MemoryModuleType<void>>("dummy");
    DUMMY = static_cast<const MemoryModuleType<void>*>(s_types["dummy"].get());

    // 位置相关
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

    // 实体相关
    s_types["mobs"] = std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("mobs");
    MOBS = static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["mobs"].get());

    s_types["visible_mobs"] = std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("visible_mobs");
    VISIBLE_MOBS = static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["visible_mobs"].get());

    s_types["visible_villager_babies"] = std::make_unique<MemoryModuleType<std::vector<LivingEntity*>>>("visible_villager_babies");
    VISIBLE_VILLAGER_BABIES = static_cast<const MemoryModuleType<std::vector<LivingEntity*>>*>(s_types["visible_villager_babies"].get());

    s_types["nearest_players"] = std::make_unique<MemoryModuleType<std::vector<Player*>>>("nearest_players");
    NEAREST_PLAYERS = static_cast<const MemoryModuleType<std::vector<Player*>>*>(s_types["nearest_players"].get());

    s_types["nearest_visible_player"] = std::make_unique<MemoryModuleType<Player*>>("nearest_visible_player");
    NEAREST_VISIBLE_PLAYER = reinterpret_cast<const MemoryModuleType<Player*>*>(s_types["nearest_visible_player"].get());

    s_types["nearest_visible_targetable_player"] = std::make_unique<MemoryModuleType<Player*>>("nearest_visible_targetable_player");
    NEAREST_VISIBLE_TARGETABLE_PLAYER = reinterpret_cast<const MemoryModuleType<Player*>*>(s_types["nearest_visible_targetable_player"].get());

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

    s_types["nearest_visible_zombified"] = std::make_unique<MemoryModuleType<LivingEntity*>>("nearest_visible_zombified");
    NEAREST_VISIBLE_ZOMBIFIED = static_cast<const MemoryModuleType<LivingEntity*>*>(s_types["nearest_visible_zombified"].get());

    s_types["breed_target"] = std::make_unique<MemoryModuleType<AgeableEntity*>>("breed_target");
    BREED_TARGET = static_cast<const MemoryModuleType<AgeableEntity*>*>(s_types["breed_target"].get());

    s_types["nearest_visible_adult"] = std::make_unique<MemoryModuleType<AgeableEntity*>>("nearest_visible_adult");
    NEAREST_VISIBLE_ADULT = static_cast<const MemoryModuleType<AgeableEntity*>*>(s_types["nearest_visible_adult"].get());

    s_types["ride_target"] = std::make_unique<MemoryModuleType<Entity*>>("ride_target");
    RIDE_TARGET = static_cast<const MemoryModuleType<Entity*>*>(s_types["ride_target"].get());

    s_types["nearest_visible_nemesis"] = std::make_unique<MemoryModuleType<MobEntity*>>("nearest_visible_nemesis");
    NEAREST_VISIBLE_NEMESIS = static_cast<const MemoryModuleType<MobEntity*>*>(s_types["nearest_visible_nemesis"].get());

    s_types["nearest_visible_wanted_item"] = std::make_unique<MemoryModuleType<ItemEntity*>>("nearest_visible_wanted_item");
    NEAREST_VISIBLE_WANTED_ITEM = static_cast<const MemoryModuleType<ItemEntity*>*>(s_types["nearest_visible_wanted_item"].get());

    // 移动相关
    s_types["path"] = std::make_unique<MemoryModuleType<Path>>("path");
    PATH = static_cast<const MemoryModuleType<Path>*>(s_types["path"].get());

    s_types["walk_target"] = std::make_unique<MemoryModuleType<void>>("walk_target");
    WALK_TARGET = static_cast<const MemoryModuleType<void>*>(s_types["walk_target"].get());

    s_types["look_target"] = std::make_unique<MemoryModuleType<void>>("look_target");
    LOOK_TARGET = static_cast<const MemoryModuleType<void>*>(s_types["look_target"].get());

    // 战斗相关
    s_types["attack_cooling_down"] = std::make_unique<MemoryModuleType<bool>>("attack_cooling_down");
    ATTACK_COOLING_DOWN = static_cast<const MemoryModuleType<bool>*>(s_types["attack_cooling_down"].get());

    s_types["hurt_by"] = std::make_unique<MemoryModuleType<DamageSource*>>("hurt_by");
    HURT_BY = static_cast<const MemoryModuleType<DamageSource*>*>(s_types["hurt_by"].get());

    // 时间相关
    s_types["heard_bell_time"] = std::make_unique<MemoryModuleType<i64>>("heard_bell_time");
    HEARD_BELL_TIME = static_cast<const MemoryModuleType<i64>*>(s_types["heard_bell_time"].get());

    s_types["cant_reach_walk_target_since"] = std::make_unique<MemoryModuleType<i64>>("cant_reach_walk_target_since");
    CANT_REACH_WALK_TARGET_SINCE = static_cast<const MemoryModuleType<i64>*>(s_types["cant_reach_walk_target_since"].get());

    s_types["last_slept"] = std::make_unique<MemoryModuleType<i64>>("last_slept");
    LAST_SLEPT = static_cast<const MemoryModuleType<i64>*>(s_types["last_slept"].get());

    s_types["last_woken"] = std::make_unique<MemoryModuleType<i64>>("last_woken");
    LAST_WOKEN = static_cast<const MemoryModuleType<i64>*>(s_types["last_woken"].get());

    s_types["last_worked_at_poi"] = std::make_unique<MemoryModuleType<i64>>("last_worked_at_poi");
    LAST_WORKED_AT_POI = static_cast<const MemoryModuleType<i64>*>(s_types["last_worked_at_poi"].get());

    // 状态相关
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
}

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
