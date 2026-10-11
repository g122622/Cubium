#pragma once

#include "common/core/Result.hpp"
#include "common/entity/damage/tag/DamageTypeTag.hpp"
#include <algorithm>
#include <array>
#include <iterator>
#include <string_view>

namespace mc::network::backend::java {

/** @brief damage_type 的同步注册序与包编码共用的 ID 映射。 */
class JavaDamageTypeIds {
public:
    inline static constexpr std::array<std::string_view, 50> ENTRIES{"minecraft:in_fire",
        "minecraft:lightning_bolt",
        "minecraft:on_fire",
        "minecraft:lava",
        "minecraft:hot_floor",
        "minecraft:in_wall",
        "minecraft:cramming",
        "minecraft:drown",
        "minecraft:starve",
        "minecraft:cactus",
        "minecraft:fall",
        "minecraft:out_of_world",
        "minecraft:generic",
        "minecraft:magic",
        "minecraft:wither",
        "minecraft:dragon_breath",
        "minecraft:dry_out",
        "minecraft:sweet_berry_bush",
        "minecraft:freeze",
        "minecraft:stalagmite",
        "minecraft:falling_stalactite",
        "minecraft:arrow",
        "minecraft:trident",
        "minecraft:fireball",
        "minecraft:wither_skull",
        "minecraft:thrown",
        "minecraft:indirect_magic",
        "minecraft:thorns",
        "minecraft:explosion",
        "minecraft:player_attack",
        "minecraft:mob_attack",
        "minecraft:mob_attack_no_aggro",
        "minecraft:mob_projectile",
        "minecraft:sonic_boom",
        "minecraft:generic_kill",
        "minecraft:outside_border",
        "minecraft:bad_respawn_point",
        "minecraft:falling_anvil",
        "minecraft:falling_block",
        "minecraft:sting",
        "minecraft:campfire",
        "minecraft:fireworks",
        "minecraft:fly_into_wall",
        "minecraft:player_explosion",
        "minecraft:spear",
        "minecraft:spit",
        "minecraft:wind_charge",
        "minecraft:mace_smash",
        "minecraft:ender_pearl",
        "minecraft:unattributed_fireball"};

    /** @brief 将业务伤害类型映射为 Configuration 下发顺序中的索引。 */
    [[nodiscard]] static Result<i32> toWireId(DamageType type)
    {
        const auto name = DamageTypeNames::getResourceLocation(type).toString();
        const auto it = std::find(ENTRIES.begin(), ENTRIES.end(), name);
        if (it == ENTRIES.end()) {
            return Error(ErrorCode::InvalidArgument, "Unknown damage type");
        }
        return static_cast<i32>(std::distance(ENTRIES.begin(), it));
    }
};

} // namespace mc::network::backend::java
