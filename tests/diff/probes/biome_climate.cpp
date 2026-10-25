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

/**
 * @file biome_climate.cpp
 * @brief 差分能力：生物群系的气候与视觉参数（原版 ClimateSettings + BiomeSpecialEffects）。
 *
 * `list`：已注册的全部生物群系（`minecraft:<name>`）
 * `run` ：{ "biome": "minecraft:plains" }
 *         → 温度、是否降水、水色、雾色、天空色、草色修改器
 *
 * 温度决定雪线、水面结冰、雨雪切换；颜色决定水下与天空的观感。全部是手抄常量，
 * 错 0.05 就足以让同一个群系在边界高度上「下雪 vs 下雨」翻转。
 *
 * 草/树叶的最终颜色还依赖客户端 colormap（尚未实现），不在本能力范围。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeRegistry.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using world::biome::Biome;
using world::biome::BiomeClimate;
using world::biome::BiomeEffects;
using world::biome::BiomeRegistry;

void ensureBiomes()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        BiomeRegistry::instance().initialize();
        s_initialized = true;
    }
}

/**
 * Biome::m_name 沿用 1.16.5 的名字（mountains / wooded_badlands_plateau ……），
 * 1.18 起原版改了名。这里按与 JavaBiomeRegistryIdMap::renamedBiomeAliases 相同的
 * 别名表归一化，保证两侧比的是同一个群系。
 */
const std::string& normalizeBiomeName(const std::string& raw)
{
    // 表是静态的、只含字面量，返回引用安全。
    static const std::unordered_map<std::string, std::string> kAliases = {
        // 1.16.5 → 1.18 改名
        {"mountains", "windswept_hills"},
        {"wooded_mountains", "windswept_forest"},
        {"gravelly_mountains", "windswept_gravelly_hills"},
        {"shattered_savanna", "windswept_savanna"},
        {"wooded_badlands_plateau", "wooded_badlands"},
        {"stone_shore", "stony_shore"},
        {"giant_tree_taiga", "old_growth_pine_taiga"},
        {"giant_spruce_taiga", "old_growth_spruce_taiga"},
        {"tall_birch_forest", "old_growth_birch_forest"},
        {"jungle_edge", "sparse_jungle"},
        // 1.16.5 时代的等价旧名
        {"extreme_hills", "windswept_hills"},
        {"extreme_hills_with_trees", "windswept_forest"},
        {"snowy_tundra", "snowy_plains"},
    };
    const auto it = kAliases.find(raw);
    return it == kAliases.end() ? raw : it->second;
}

std::string qualifiedName(const Biome& biome)
{
    static const std::string kPrefix = "minecraft:";
    const std::string& name = biome.name();
    const std::string bare = name.starts_with(kPrefix) ? name.substr(kPrefix.size()) : name;
    return kPrefix + normalizeBiomeName(bare);
}

/// 与 oracle 的 roundToFloat5 同格式：把 f32 打印成 5 位小数（两侧比的都是同一个 f32）。
f64 roundToFloat5(f32 value)
{
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.5f", static_cast<double>(value));
    return std::stod(buffer);
}

void writeList(FILE* out)
{
    ensureBiomes();
    std::vector<std::string> names;
    for (const Biome& biome : BiomeRegistry::instance().allBiomes()) {
        names.push_back(qualifiedName(biome));
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

const char* grassColorModifierName(world::biome::GrassColorModifier modifier)
{
    switch (modifier) {
        case world::biome::GrassColorModifier::DarkForest:
            return "dark_forest";
        case world::biome::GrassColorModifier::Swamp:
            return "swamp";
        case world::biome::GrassColorModifier::None:
        default:
            return "none";
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureBiomes();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("biome").get<std::string>();

    for (const Biome& biome : BiomeRegistry::instance().allBiomes()) {
        if (qualifiedName(biome) != requested) {
            continue;
        }
        const BiomeClimate& climate = biome.climate();
        const BiomeEffects& effects = biome.effects();
        return json{{"biome", requested},
            {"temperature", roundToFloat5(climate.temperature)},
            {"hasPrecipitation", climate.hasPrecipitation},
            {"waterColor", effects.waterColor() & 0xFFFFFFu},
            {"waterFogColor", effects.waterFogColor() & 0xFFFFFFu},
            {"fogColor", effects.fogColor() & 0xFFFFFFu},
            {"skyColor", effects.skyColor() & 0xFFFFFFu},
            {"grassColorModifier", grassColorModifierName(effects.grassColorModifier())}}
            .dump();
    }
    return json{{"biome", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("biome_climate", &writeList, &runLine)

} // namespace mc::diff