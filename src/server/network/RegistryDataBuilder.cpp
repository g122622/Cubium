/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#include "server/network/RegistryDataBuilder.hpp"

namespace mc::server::net {

namespace {

/// 给定一组资源位置 id，构造 data=nullopt 的 RegistryData（声明客户端已知）
mc::network::ir::configuration::RegistryData makeKnownRegistry(
    const char* registryKey, std::initializer_list<const char*> ids)
{
    mc::network::ir::configuration::RegistryData data;
    data.registryKey = registryKey;
    data.entries.reserve(ids.size());
    for (const char* id : ids) {
        data.entries.push_back({std::string(id), std::nullopt});
    }
    return data;
}

} // namespace

std::vector<mc::network::ir::configuration::RegistryData> buildConfigurationRegistryData()
{
    // TODO(Phase6): 这里所有条目 data=nullopt，依赖客户端 SelectKnownPacks 命中 minecraft:core。
    // 真 Java 互通需要补全 dimension_type/biome/chat_type 等的 NBT 序列化。
    // 注册表 key 与条目 id 对齐 Java 1.21.11 RegistrySynchronization.PATCHED_REGISTRIES。
    std::vector<mc::network::ir::configuration::RegistryData> registries;
    registries.reserve(8);

    registries.push_back(makeKnownRegistry(
        "minecraft:dimension_type", {"minecraft:overworld", "minecraft:the_nether", "minecraft:the_end"}));

    registries.push_back(makeKnownRegistry("minecraft:biome",
        {"minecraft:plains",
            "minecraft:desert",
            "minecraft:forest",
            "minecraft:taiga",
            "minecraft:swamp",
            "minecraft:river",
            "minecraft:beach",
            "minecraft:snowy_plains",
            "minecraft:jungle",
            "minecraft:savanna"}));

    registries.push_back(makeKnownRegistry("minecraft:chat_type",
        {"minecraft:chat",
            "minecraft:emote_command",
            "minecraft:msg_command_incoming",
            "minecraft:msg_command_outgoing",
            "minecraft:narration",
            "minecraft:say_command",
            "minecraft:team_msg_command_incoming",
            "minecraft:team_msg_command_outgoing",
            "minecraft:system"}));

    registries.push_back(makeKnownRegistry("minecraft:damage_type",
        {"minecraft:in_fire",
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
            "minecraft:avoid",
            "minecraft:thistle",
            "minecraft:generic_kill",
            "minecraft:outside_border",
            "minecraft:merging"}));

    // 以下注册表条目较多，仅列代表性条目占位（客户端命中 core 后不消费 NBT）
    registries.push_back(makeKnownRegistry("minecraft:trim_material",
        {"minecraft:quartz",
            "minecraft:iron",
            "minecraft:netherite",
            "minecraft:redstone",
            "minecraft:copper",
            "minecraft:gold",
            "minecraft:emerald",
            "minecraft:diamond",
            "minecraft:lapis",
            "minecraft:amethyst"}));

    registries.push_back(makeKnownRegistry("minecraft:trim_pattern",
        {"minecraft:bolt",
            "minecraft:coast",
            "minecraft:dune",
            "minecraft:eye",
            "minecraft:host",
            "minecraft:raiser",
            "minecraft:rib",
            "minecraft:sentry",
            "minecraft:shaper",
            "minecraft:silence",
            "minecraft:snout",
            "minecraft:spire",
            "minecraft:tide",
            "minecraft:vex",
            "minecraft:ward",
            "minecraft:wayfinder",
            "minecraft:wild"}));

    registries.push_back(makeKnownRegistry("minecraft:wolf_variant",
        {"minecraft:pale",
            "minecraft:woods",
            "minecraft:ashen",
            "minecraft:black",
            "minecraft:chestnut",
            "minecraft:rusty",
            "minecraft:snowy",
            "minecraft:spotted",
            "minecraft:striped"}));

    registries.push_back(makeKnownRegistry("minecraft:painting_variant",
        {"minecraft:kebab",
            "minecraft:aztec",
            "minecraft:alban",
            "minecraft:aztec2",
            "minecraft:bomb",
            "minecraft:plant",
            "minecraft:wasteland",
            "minecraft:pool",
            "minecraft:courbet",
            "minecraft:sea",
            "minecraft:sunset",
            "minecraft:creebet",
            "minecraft:wanderer",
            "minecraft:graal",
            "minecraft:bust",
            "minecraft:match",
            "minecraft:skull_and_roses",
            "minecraft:stage",
            "minecraft:void",
            "minecraft:skull_and_roses",
            "minecraft:wither",
            "minecraft:fighters",
            "minecraft:pointer",
            "minecraft:pigscene",
            "minecraft:burning_skull",
            "minecraft:skeleton",
            "minecraft:donkey_kong"}));

    return registries;
}

std::vector<mc::network::ir::configuration::KnownPack> buildServerKnownPacks()
{
    return {mc::network::ir::configuration::KnownPack{"minecraft", "core", "1.21.11"}};
}

} // namespace mc::server::net
