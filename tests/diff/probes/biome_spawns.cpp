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
 * @file biome_spawns.cpp
 * @brief 差分能力：生物群系的自然生成配置。
 *
 * `list`：服务端加载到的生物群系 ID
 * `run` ：{ "biome": "minecraft:plains" } → 每个 `MobCategory` 的生成条目
 *
 * 数据包里的 biome JSON 是 spawners / creature_spawn_probability / spawn_costs 的**唯一来源**，
 * 服务端由 `BiomeLoader::loadFromJson` 解码到 `MobSpawnInfo`。探针走同一条路径（同一个
 * `MobSpawnInfoJson` 解码器），产出与原版 `MobSpawnSettings` 逐字段比对——因此这条
 * harness 同时覆盖「数据包解码是否正确」和「群系名 → BiomeId 映射是否完整」。
 *
 * 生成算法（`NaturalSpawner` 的位置挑选、生物上限统计、怪物密度检查）属于运行时行为，
 * 不在静态数据这一层比对。
 */

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Result.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/spawn/MobSpawnInfo.hpp"
#include "common/world/spawn/MobSpawnInfoJson.hpp"
#include "biome_name_mapping.hpp"
#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using entity::EntityClassification;
using world::spawn::MobSpawnInfo;
using world::spawn::SpawnEntry;

/// 数据包 `spawners` 里出现的全部分类（顺序与输出无关，写 JSON 时按名字排序）
constexpr EntityClassification ALL_CATEGORIES[] = {
    EntityClassification::Monster,
    EntityClassification::Creature,
    EntityClassification::Ambient,
    EntityClassification::Axolotls,
    EntityClassification::UndergroundWaterCreature,
    EntityClassification::WaterCreature,
    EntityClassification::WaterAmbient,
    EntityClassification::Misc,
};

/// 加载后的群系生成表
struct LoadedBiome {
    MobSpawnInfo info;
};

std::vector<std::pair<std::string, LoadedBiome>>& biomes()
{
    static std::vector<std::pair<std::string, LoadedBiome>> s_biomes;
    return s_biomes;
}

[[nodiscard]] json summariseSpawns(const std::vector<SpawnEntry>& spawns)
{
    json result = json::array();
    for (const auto& spawn : spawns) {
        result.push_back(json{
            {"type", spawn.entityTypeId},
            {"weight", spawn.weight},
            {"minCount", spawn.minCount},
            {"maxCount", spawn.maxCount},
        });
    }
    std::sort(result.begin(), result.end(), [](const json& left, const json& right) {
        const auto key = [](const json& entry) {
            return std::make_tuple(entry.at("type").get<std::string>(),
                entry.at("weight").get<i32>(),
                entry.at("minCount").get<i32>(),
                entry.at("maxCount").get<i32>());
        };
        return key(left) < key(right);
    });
    return result;
}

[[nodiscard]] json summariseSpawnCosts(const MobSpawnInfo& info)
{
    // 生成成本没有公开的遍历接口，按内置实体注册表逐个查询（与原版 oracle 同一份候选集合）。
    json costs = json::object();
    for (const auto* type : entity::EntityRegistry::instance().getAllTypes()) {
        if (type == nullptr) {
            continue;
        }
        const auto* cost = info.getSpawnCost(type->name());
        if (cost == nullptr) {
            continue;
        }
        costs[type->name()] = json{{"charge", cost->charge}, {"energy_budget", cost->energyBudget}};
    }
    return costs;
}

void loadAll()
{
    static bool s_initialised = false;
    if (s_initialised) {
        return;
    }
    s_initialised = true;

    datapack::initializeRegistries();
    world::biome::BiomeRegistry::instance().initialize();

    const auto biomeDir = datapack::root() / "data" / "minecraft" / "worldgen" / "biome";

    for (const auto& file : datapack::jsonFiles(biomeDir)) {
        const std::string name = file.stem().string();
        const auto biomeId = biome::biomeIdByDataPackName(name);
        if (!biomeId.has_value()) {
            std::fprintf(stderr, "biome_spawns: no BiomeId for '%s'\n", name.c_str());
            continue;
        }

        std::ifstream stream(file);
        if (!stream) {
            continue;
        }
        json contents;
        try {
            stream >> contents;
        }
        catch (const json::parse_error& error) {
            std::fprintf(stderr, "biome_spawns: %s: %s\n", name.c_str(), error.what());
            continue;
        }

        // 与 BiomeLoader::loadFromJson 同一条解码路径。probability 缺省时不能用本项目工厂的
        // 默认值（未被数据包覆盖的群系还带着手抄的旧值），取原版 `MobSpawnSettings.CODEC`
        // 的字段默认值 0.1f（此处即 mobSpawnInfoFromJson 的调用参数）。
        constexpr f32 DEFAULT_CREATURE_SPAWN_PROBABILITY = 0.1f;
        std::optional<json> probability;
        if (contents.contains("creature_spawn_probability")) {
            probability = contents["creature_spawn_probability"];
        }
        const json spawners = contents.contains("spawners") ? contents["spawners"] : json();
        auto parsed =
            world::spawn::mobSpawnInfoFromJson(spawners, probability, DEFAULT_CREATURE_SPAWN_PROBABILITY);
        if (parsed.failed()) {
            std::fprintf(stderr, "biome_spawns: %s: %s\n", name.c_str(), parsed.error().message().c_str());
            continue;
        }
        LoadedBiome loaded{std::move(parsed).value()};
        if (contents.contains("spawn_costs")) {
            auto costs = world::spawn::applySpawnCosts(loaded.info, contents["spawn_costs"]);
            if (costs.failed()) {
                std::fprintf(stderr, "biome_spawns: %s: %s\n", name.c_str(), costs.error().message().c_str());
                continue;
            }
        }
        biomes().emplace_back("minecraft:" + name, std::move(loaded));
    }
    std::sort(biomes().begin(), biomes().end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
}

void writeList(FILE* out)
{
    loadAll();
    for (const auto& [id, unused] : biomes()) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    loadAll();
    const json input = json::parse(inputLine);
    const std::string id = input.at("biome").get<std::string>();

    const auto found = std::lower_bound(
        biomes().begin(), biomes().end(), id, [](const auto& entry, const std::string& value) {
            return entry.first < value;
        });
    if (found == biomes().end() || found->first != id) {
        return json{{"biome", id}, {"missing", true}}.dump();
    }

    const MobSpawnInfo& info = found->second.info;
    json categories = json::object();
    for (const auto category : ALL_CATEGORIES) {
        categories[std::string(world::spawn::spawnCategoryName(category))] = json{
            {"maxInstances", entity::getMaxCount(category)},
            {"spawns", summariseSpawns(info.getSpawns(category))},
        };
    }

    json result{
        {"biome", id},
        {"categories", std::move(categories)},
        {"creature_spawn_probability", info.getCreatureSpawnProbability()},
        {"spawn_costs", summariseSpawnCosts(info)},
    };
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("biome_spawns", &writeList, &runLine)

} // namespace mc::diff