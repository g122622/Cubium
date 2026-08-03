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

#include "common/world/biome/JavaBiomeRegistryIdMap.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc::world::biome {

// ============================================================================
// 单例
// ============================================================================

JavaBiomeRegistryIdMap& JavaBiomeRegistryIdMap::instance()
{
    static JavaBiomeRegistryIdMap s_instance;
    return s_instance;
}

// ============================================================================
// vanilla 1.21.11 biome registry 顺序（与 RegistryDataBuilder.cpp:81 同步条目一致）
// registry id = 下标。此为真 Java 客户端分配 biome registry id 的权威顺序。
// 修改须与 RegistryDataBuilder 的 minecraft:worldgen/biome 条目保持同步。
// ============================================================================
static const std::vector<std::string>& vanillaBiomeNames()
{
    static const std::vector<std::string> kNames = {
        "minecraft:badlands",
        "minecraft:bamboo_jungle",
        "minecraft:basalt_deltas",
        "minecraft:beach",
        "minecraft:birch_forest",
        "minecraft:cherry_grove",
        "minecraft:cold_ocean",
        "minecraft:crimson_forest",
        "minecraft:dark_forest",
        "minecraft:deep_cold_ocean",
        "minecraft:deep_dark",
        "minecraft:deep_frozen_ocean",
        "minecraft:deep_lukewarm_ocean",
        "minecraft:deep_ocean",
        "minecraft:desert",
        "minecraft:dripstone_caves",
        "minecraft:end_barrens",
        "minecraft:end_highlands",
        "minecraft:end_midlands",
        "minecraft:eroded_badlands",
        "minecraft:flower_forest",
        "minecraft:forest",
        "minecraft:frozen_ocean",
        "minecraft:frozen_peaks",
        "minecraft:frozen_river",
        "minecraft:grove",
        "minecraft:ice_spikes",
        "minecraft:jagged_peaks",
        "minecraft:jungle",
        "minecraft:lukewarm_ocean",
        "minecraft:lush_caves",
        "minecraft:mangrove_swamp",
        "minecraft:meadow",
        "minecraft:mushroom_fields",
        "minecraft:nether_wastes",
        "minecraft:ocean",
        "minecraft:old_growth_birch_forest",
        "minecraft:old_growth_pine_taiga",
        "minecraft:old_growth_spruce_taiga",
        "minecraft:pale_garden",
        "minecraft:plains",
        "minecraft:river",
        "minecraft:savanna",
        "minecraft:savanna_plateau",
        "minecraft:small_end_islands",
        "minecraft:snowy_beach",
        "minecraft:snowy_plains",
        "minecraft:snowy_slopes",
        "minecraft:snowy_taiga",
        "minecraft:soul_sand_valley",
        "minecraft:sparse_jungle",
        "minecraft:stony_peaks",
        "minecraft:stony_shore",
        "minecraft:sunflower_plains",
        "minecraft:swamp",
        "minecraft:taiga",
        "minecraft:the_end",
        "minecraft:the_void",
        "minecraft:warm_ocean",
        "minecraft:warped_forest",
        "minecraft:windswept_forest",
        "minecraft:windswept_gravelly_hills",
        "minecraft:windswept_hills",
        "minecraft:windswept_savanna",
        "minecraft:wooded_badlands",
    };
    return kNames;
}

// 1.18 旧名 → 新名 别名表。项目 Biome::m_name 仍是 1.16.5 旧名（如 mountains），
// 须归一化到 1.21.11 vanilla 名（windswept_hills）才能查到 registry id。
static const std::unordered_map<std::string, std::string>& renamedBiomeAliases()
{
    static const std::unordered_map<std::string, std::string> kAliases = {
        {"mountains", "windswept_hills"},
        {"wooded_mountains", "windswept_forest"},
        {"gravelly_mountains", "windswept_gravelly_hills"},
        {"stone_shore", "stony_shore"},
        {"giant_tree_taiga", "old_growth_pine_taiga"},
        {"giant_spruce_taiga", "old_growth_spruce_taiga"},
        {"tall_birch_forest", "old_growth_birch_forest"},
        {"jungle_edge", "sparse_jungle"},
        {"wooded_badlands_plateau", "wooded_badlands"},
        {"shattered_savanna", "windswept_savanna"},
    };
    return kAliases;
}

/// 把 biome name（裸 path，无 minecraft: 前缀）归一化为 vanilla 1.21.11 名（无前缀）。
/// 先查 1.18 别名表；未命中则原样返回（已是新名或 1.16.x 已删变体）。
static std::string normalizeBiomeName(const std::string& rawName)
{
    if (const auto it = renamedBiomeAliases().find(rawName); it != renamedBiomeAliases().end()) {
        return it->second;
    }
    return rawName;
}

// ============================================================================
// 公开接口
// ============================================================================

Result<void> JavaBiomeRegistryIdMap::initialize()
{
    m_initialized = false;
    m_toJava.clear();
    m_fromJava.clear();

    // name（带 minecraft: 前缀）→ registry id
    std::unordered_map<std::string, u32> nameToRegistryId;
    const auto& names = vanillaBiomeNames();
    nameToRegistryId.reserve(names.size());
    for (u32 i = 0; i < names.size(); ++i) {
        nameToRegistryId[names[i]] = i;
    }

    // plains 的 Java registry id（兜底用）
    const auto plainsIt = nameToRegistryId.find("minecraft:plains");
    const u32 plainsRegistryId = (plainsIt != nameToRegistryId.end()) ? plainsIt->second : 40;

    size_t matched = 0;
    size_t fallback = 0;
    for (const auto& biome : BiomeRegistry::instance().allBiomes()) {
        const std::string normalized = normalizeBiomeName(biome.name());
        const std::string fullName = "minecraft:" + normalized;
        u32 registryId = plainsRegistryId;
        if (const auto it = nameToRegistryId.find(fullName); it != nameToRegistryId.end()) {
            registryId = it->second;
            ++matched;
        } else {
            ++fallback;
            spdlog::warn("JavaBiomeRegistryIdMap: biome '{}' (id={}) has no Java registry entry, "
                         "falling back to plains({})",
                biome.name(),
                biome.id(),
                plainsRegistryId);
        }
        m_toJava[biome.id()] = registryId;
        // 反向：后注册的同 registry id 覆盖前者（一般唯一，1.16.x 变体兜底 plains 会覆盖，
        // 但 plains 本身有自己的 BiomeId，兜底条目的反向映射意义不大，保留 plains 真身即可）。
        if (registryId == plainsRegistryId) {
            // 仅当 plains 真身（name==plains）时写反向，避免兜底条目抢占 plains 反向槽。
            if (normalized == "plains") {
                m_fromJava[registryId] = biome.id();
            }
        } else {
            m_fromJava[registryId] = biome.id();
        }
    }

    // 确保 plains 反向映射存在（即便上面的遍历顺序未覆盖）。
    m_fromJava.try_emplace(plainsRegistryId, Biomes::Plains);

    spdlog::info("JavaBiomeRegistryIdMap: matched {} biomes, {} fell back to plains", matched, fallback);

    m_initialized = true;
    return {};
}

u32 JavaBiomeRegistryIdMap::toJavaRegistryId(BiomeId internalBiomeId) const
{
    if (!m_initialized) {
        spdlog::warn("JavaBiomeRegistryIdMap: not initialized, returning plains registry id");
        return 40; // plains registry id
    }
    if (const auto it = m_toJava.find(internalBiomeId); it != m_toJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaBiomeRegistryIdMap: toJavaRegistryId miss for internal BiomeId={}", internalBiomeId);
    return 40; // plains 兜底
}

BiomeId JavaBiomeRegistryIdMap::fromJavaRegistryId(u32 javaRegistryId) const
{
    if (!m_initialized) {
        return Biomes::Plains;
    }
    if (const auto it = m_fromJava.find(javaRegistryId); it != m_fromJava.end()) {
        return it->second;
    }
    spdlog::warn("JavaBiomeRegistryIdMap: fromJavaRegistryId miss for javaRegistryId={}", javaRegistryId);
    return Biomes::Plains; // plains 兜底
}

} // namespace mc::world::biome
