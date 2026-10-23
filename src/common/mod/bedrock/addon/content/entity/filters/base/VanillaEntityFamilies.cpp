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

#include "common/mod/bedrock/addon/content/entity/filters/base/VanillaEntityFamilies.hpp"
#include <algorithm>
#include <array>

namespace mc::mod::bedrock::addon {
namespace {
struct _FamilyEntry {
    std::string_view type;
    std::string_view families;
};

// 数据来自官方文档仓库 Reference/Source/VanillaBehaviorPack/entities 的基础组件。
// 存在不同家族组配置的实体不进入静态表，防止把条件家族当作始终活动的家族。
constexpr std::array _FAMILIES{
    _FamilyEntry{"minecraft:allay", "allay|mob"},
    _FamilyEntry{"minecraft:area_effect_cloud", ""},
    _FamilyEntry{"minecraft:armor_stand", "armor_stand|inanimate|mob"},
    _FamilyEntry{"minecraft:arrow", ""},
    _FamilyEntry{"minecraft:axolotl", "axolotl|mob"},
    _FamilyEntry{"minecraft:bat", "bat|mob"},
    _FamilyEntry{"minecraft:blaze", "blaze|mob|monster"},
    _FamilyEntry{"minecraft:boat", "boat|inanimate"},
    _FamilyEntry{"minecraft:cat", "cat|mob"},
    _FamilyEntry{"minecraft:cave_spider", "arthropod|cavespider|mob|monster"},
    _FamilyEntry{"minecraft:chest_boat", "boat|inanimate"},
    _FamilyEntry{"minecraft:chest_minecart", "inanimate|minecart"},
    _FamilyEntry{"minecraft:chicken", "chicken|mob"},
    _FamilyEntry{"minecraft:cod", "cod|fish"},
    _FamilyEntry{"minecraft:command_block_minecart", "inanimate|minecart"},
    _FamilyEntry{"minecraft:cow", "cow|mob"},
    _FamilyEntry{"minecraft:creeper", "creeper|mob|monster"},
    _FamilyEntry{"minecraft:dolphin", "dolphin|mob"},
    _FamilyEntry{"minecraft:donkey", "donkey|mob"},
    _FamilyEntry{"minecraft:dragon_fireball", ""},
    _FamilyEntry{"minecraft:drowned", "drowned|mob|monster|undead|zombie"},
    _FamilyEntry{"minecraft:egg", ""},
    _FamilyEntry{"minecraft:elder_guardian", "guardian_elder|mob|monster"},
    _FamilyEntry{"minecraft:ender_crystal", ""},
    _FamilyEntry{"minecraft:ender_dragon", "dragon|mob"},
    _FamilyEntry{"minecraft:ender_pearl", ""},
    _FamilyEntry{"minecraft:enderman", "enderman|mob|monster"},
    _FamilyEntry{"minecraft:endermite", "arthropod|endermite|lightweight|mob|monster"},
    _FamilyEntry{"minecraft:evocation_illager", "evocation_illager|illager|mob|monster"},
    _FamilyEntry{"minecraft:eye_of_ender_signal", ""},
    _FamilyEntry{"minecraft:fireball", ""},
    _FamilyEntry{"minecraft:fireworks_rocket", ""},
    _FamilyEntry{"minecraft:fishing_hook", ""},
    _FamilyEntry{"minecraft:fox", "fox|lightweight|mob"},
    _FamilyEntry{"minecraft:frog", "frog|mob"},
    _FamilyEntry{"minecraft:ghast", "ghast|mob|monster"},
    _FamilyEntry{"minecraft:glow_squid", "mob|squid"},
    _FamilyEntry{"minecraft:goat", "animal|goat"},
    _FamilyEntry{"minecraft:guardian", "guardian|mob|monster"},
    _FamilyEntry{"minecraft:hopper_minecart", "inanimate|minecart"},
    _FamilyEntry{"minecraft:horse", "horse|mob"},
    _FamilyEntry{"minecraft:husk", "husk|mob|monster|undead|zombie"},
    _FamilyEntry{"minecraft:iron_golem", "irongolem|mob"},
    _FamilyEntry{"minecraft:lightning_bolt", "lightning"},
    _FamilyEntry{"minecraft:lingering_potion", ""},
    _FamilyEntry{"minecraft:llama", "llama|mob"},
    _FamilyEntry{"minecraft:llama_spit", ""},
    _FamilyEntry{"minecraft:magma_cube", "magmacube|mob|monster"},
    _FamilyEntry{"minecraft:minecart", "inanimate|minecart"},
    _FamilyEntry{"minecraft:mooshroom", "mob|mushroomcow"},
    _FamilyEntry{"minecraft:mule", "mob|mule"},
    _FamilyEntry{"minecraft:ocelot", "mob|ocelot"},
    _FamilyEntry{"minecraft:phantom", "mob|monster|phantom|undead"},
    _FamilyEntry{"minecraft:pig", "mob|pig"},
    _FamilyEntry{"minecraft:piglin_brute", "adult_piglin|monster|piglin|piglin_brute"},
    _FamilyEntry{"minecraft:pillager", "illager|mob|monster|pillager"},
    _FamilyEntry{"minecraft:player", "player"},
    _FamilyEntry{"minecraft:polar_bear", "mob|polarbear"},
    _FamilyEntry{"minecraft:pufferfish", "fish|pufferfish"},
    _FamilyEntry{"minecraft:rabbit", "lightweight|mob|rabbit"},
    _FamilyEntry{"minecraft:ravager", "illager|mob|monster|ravager"},
    _FamilyEntry{"minecraft:salmon", "fish|salmon"},
    _FamilyEntry{"minecraft:sheep", "mob|sheep"},
    _FamilyEntry{"minecraft:shulker", "mob|monster|shulker"},
    _FamilyEntry{"minecraft:shulker_bullet", ""},
    _FamilyEntry{"minecraft:silverfish", "arthropod|lightweight|mob|monster|silverfish"},
    _FamilyEntry{"minecraft:skeleton", "mob|monster|skeleton|undead"},
    _FamilyEntry{"minecraft:skeleton_horse", "mob|skeletonhorse|undead"},
    _FamilyEntry{"minecraft:slime", "mob|monster|slime"},
    _FamilyEntry{"minecraft:small_fireball", ""},
    _FamilyEntry{"minecraft:snow_golem", "mob|snowgolem"},
    _FamilyEntry{"minecraft:snowball", ""},
    _FamilyEntry{"minecraft:spider", "arthropod|mob|monster|spider"},
    _FamilyEntry{"minecraft:splash_potion", ""},
    _FamilyEntry{"minecraft:squid", "mob|squid"},
    _FamilyEntry{"minecraft:stray", "mob|monster|skeleton|stray|undead"},
    _FamilyEntry{"minecraft:tadpole", "mob|tadpole"},
    _FamilyEntry{"minecraft:thrown_trident", ""},
    _FamilyEntry{"minecraft:tnt", "inanimate|tnt"},
    _FamilyEntry{"minecraft:tnt_minecart", "inanimate|minecart"},
    _FamilyEntry{"minecraft:trader_llama", "llama|mob|trader_llama"},
    _FamilyEntry{"minecraft:tripod_camera", "inanimate|mob|tripodcamera"},
    _FamilyEntry{"minecraft:tropicalfish", "fish|tropicalfish"},
    _FamilyEntry{"minecraft:vex", "mob|monster|vex"},
    _FamilyEntry{"minecraft:vindicator", "illager|mob|monster|vindicator"},
    _FamilyEntry{"minecraft:warden", "mob|monster|warden"},
    _FamilyEntry{"minecraft:witch", "mob|monster|witch"},
    _FamilyEntry{"minecraft:wither", "mob|monster|skeleton|undead|wither"},
    _FamilyEntry{"minecraft:wither_skeleton", "mob|monster|skeleton|undead|wither"},
    _FamilyEntry{"minecraft:wither_skull", ""},
    _FamilyEntry{"minecraft:wither_skull_dangerous", ""},
    _FamilyEntry{"minecraft:wolf", "mob|wolf"},
    _FamilyEntry{"minecraft:xp_bottle", ""},
    _FamilyEntry{"minecraft:xp_orb", "inanimate"},
    _FamilyEntry{"minecraft:zombie", "mob|monster|undead|zombie"},
    _FamilyEntry{"minecraft:zombie_horse", "mob|undead|zombiehorse"},
    _FamilyEntry{"minecraft:zombie_pigman", "mob|monster|undead|zombie_pigman"},
};
} // namespace

std::optional<bool> vanillaEntityFamilyMatch(std::string_view typeId, std::string_view family)
{
    const auto found = std::lower_bound(
        _FAMILIES.begin(), _FAMILIES.end(), typeId, [](const _FamilyEntry& entry, std::string_view type) {
            return entry.type < type;
        });
    if (found == _FAMILIES.end() || found->type != typeId) {
        return std::nullopt;
    }
    auto remaining = found->families;
    while (!remaining.empty()) {
        const auto delimiter = remaining.find('|');
        if (remaining.substr(0, delimiter) == family) {
            return true;
        }
        if (delimiter == std::string_view::npos) {
            break;
        }
        remaining.remove_prefix(delimiter + 1);
    }
    return false;
}
} // namespace mc::mod::bedrock::addon
