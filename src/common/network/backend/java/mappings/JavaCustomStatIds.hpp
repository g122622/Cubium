#pragma once

#include "common/core/Types.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <string_view>

namespace mc::network::backend::java {

/** @brief 1.21.11 客户端内置 custom_stat 的注册顺序。 */
class JavaCustomStatIds {
public:
    inline static constexpr std::array<std::string_view, 77> ENTRIES{"minecraft:leave_game",
        "minecraft:play_time",
        "minecraft:total_world_time",
        "minecraft:time_since_death",
        "minecraft:time_since_rest",
        "minecraft:sneak_time",
        "minecraft:walk_one_cm",
        "minecraft:crouch_one_cm",
        "minecraft:sprint_one_cm",
        "minecraft:walk_on_water_one_cm",
        "minecraft:fall_one_cm",
        "minecraft:climb_one_cm",
        "minecraft:fly_one_cm",
        "minecraft:walk_under_water_one_cm",
        "minecraft:minecart_one_cm",
        "minecraft:boat_one_cm",
        "minecraft:pig_one_cm",
        "minecraft:happy_ghast_one_cm",
        "minecraft:horse_one_cm",
        "minecraft:aviate_one_cm",
        "minecraft:swim_one_cm",
        "minecraft:strider_one_cm",
        "minecraft:nautilus_one_cm",
        "minecraft:jump",
        "minecraft:drop",
        "minecraft:damage_dealt",
        "minecraft:damage_dealt_absorbed",
        "minecraft:damage_dealt_resisted",
        "minecraft:damage_taken",
        "minecraft:damage_blocked_by_shield",
        "minecraft:damage_absorbed",
        "minecraft:damage_resisted",
        "minecraft:deaths",
        "minecraft:mob_kills",
        "minecraft:animals_bred",
        "minecraft:player_kills",
        "minecraft:fish_caught",
        "minecraft:talked_to_villager",
        "minecraft:traded_with_villager",
        "minecraft:eat_cake_slice",
        "minecraft:fill_cauldron",
        "minecraft:use_cauldron",
        "minecraft:clean_armor",
        "minecraft:clean_banner",
        "minecraft:clean_shulker_box",
        "minecraft:interact_with_brewingstand",
        "minecraft:interact_with_beacon",
        "minecraft:inspect_dropper",
        "minecraft:inspect_hopper",
        "minecraft:inspect_dispenser",
        "minecraft:play_noteblock",
        "minecraft:tune_noteblock",
        "minecraft:pot_flower",
        "minecraft:trigger_trapped_chest",
        "minecraft:open_enderchest",
        "minecraft:enchant_item",
        "minecraft:play_record",
        "minecraft:interact_with_furnace",
        "minecraft:interact_with_crafting_table",
        "minecraft:open_chest",
        "minecraft:sleep_in_bed",
        "minecraft:open_shulker_box",
        "minecraft:open_barrel",
        "minecraft:interact_with_blast_furnace",
        "minecraft:interact_with_smoker",
        "minecraft:interact_with_lectern",
        "minecraft:interact_with_campfire",
        "minecraft:interact_with_cartography_table",
        "minecraft:interact_with_loom",
        "minecraft:interact_with_stonecutter",
        "minecraft:bell_ring",
        "minecraft:raid_trigger",
        "minecraft:raid_win",
        "minecraft:interact_with_anvil",
        "minecraft:interact_with_grindstone",
        "minecraft:target_hit",
        "minecraft:interact_with_smithing_table"};

    /** @brief 根据资源名查找 Java 自定义统计 ID，不存在时返回空值。 */
    [[nodiscard]] static std::optional<i32> toWireId(std::string_view name)
    {
        const auto it = std::find(ENTRIES.begin(), ENTRIES.end(), name);
        return it == ENTRIES.end() ? std::nullopt
                                   : std::optional<i32>(static_cast<i32>(std::distance(ENTRIES.begin(), it)));
    }
};

} // namespace mc::network::backend::java
