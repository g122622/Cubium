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

#include "biome_name_mapping.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "probe.hpp"
#include "vanilla_datapack.hpp"
#include <algorithm>
#include <fstream>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace mc::diff {
namespace {
using nlohmann::json;
using namespace world::spawn;

void _loadBiomeTags()
{
    // 群系成员是场景数据，来自两侧共用的数据包；谓词本身仍读取真实注册表。
    const auto directory = datapack::root() / "data/minecraft/tags/worldgen/biome";
    std::unordered_map<std::string, json> definitions;
    for (const auto& file : datapack::jsonFiles(directory)) {
        auto path = std::filesystem::relative(file, directory).generic_string();
        path.resize(path.size() - 5);
        std::ifstream stream(file);
        definitions.emplace("minecraft:" + path, json::parse(stream));
    }
    std::unordered_map<std::string, std::vector<BiomeId>> resolved;
    std::function<const std::vector<BiomeId>&(const std::string&)> resolve =
        [&](const std::string& name) -> const std::vector<BiomeId>& {
        if (const auto found = resolved.find(name); found != resolved.end()) {
            return found->second;
        }
        std::vector<BiomeId> members;
        for (const auto& value : definitions.at(name).at("values")) {
            const auto id = value.is_string() ? value.get<std::string>() : value.at("id").get<std::string>();
            if (id.starts_with('#')) {
                const auto& referenced = resolve(id.substr(1));
                members.insert(members.end(), referenced.begin(), referenced.end());
            } else {
                const auto mapped = biome::biomeIdByDataPackName(id.substr(id.find(':') + 1));
                MC_ASSERT_RELEASE(mapped.has_value());
                members.push_back(*mapped);
            }
        }
        return resolved.emplace(name, std::move(members)).first->second;
    };
    for (const auto& [name, unused] : definitions) {
        auto& tag = world::biome::BiomeTags::getOrCreateTag(ResourceLocation(name));
        tag.clear();
        tag.addAll(resolve(name));
    }
}

class _ruleWorld final : public ISpawnWorldReader {
public:
    explicit _ruleWorld(const json& scenario)
        : m_case(scenario)
        , m_x(scenario.at("x").get<i32>())
        , m_y(scenario.at("y").get<i32>())
        , m_z(scenario.at("z").get<i32>())
    {
        for (const auto& [offset, block] : scenario.at("blocks").items()) {
            const auto text = block.get<std::string>();
            command::StringReader reader(text);
            m_blocks.emplace(
                m_y + std::stoi(offset), command::BlockStateArgumentType::blockState()->parse(reader).state());
        }
    }

    const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto found = m_blocks.find(y);
        return x == m_x && z == m_z && found != m_blocks.end() ? found->second : &VanillaBlocks::AIR->defaultState();
    }

    bool isWithinWorldBorder(i32, i32, i32) const override
    {
        return true;
    }

    i32 getHeight(HeightmapType, i32, i32) const override
    {
        return m_case.at("surface").get<i32>();
    }

    BiomeId getBiome(i32, i32, i32) const override
    {
        const auto name = m_case.at("biome").get<std::string>();
        return biome::biomeIdByDataPackName(name.substr(name.find(':') + 1)).value();
    }

    u64 seed() const override
    {
        return static_cast<u64>(std::stoll(m_case.at("worldSeed").get<std::string>()));
    }

    Difficulty difficulty() const override
    {
        const auto name = m_case.at("difficulty").get<std::string>();
        return name == "peaceful" ? Difficulty::Peaceful
            : name == "easy"      ? Difficulty::Easy
            : name == "hard"      ? Difficulty::Hard
                                  : Difficulty::Normal;
    }

    SpawnEnvironment environment(const Vector3i&) const override
    {
        const auto dimension = m_case.at("dimension").get<std::string>();
        return SpawnEnvironment{m_case.at("sky").get<i32>(),
            m_case.at("light").get<i32>(),
            m_case.at("darken").get<i32>(),
            m_case.at("thunder").get<bool>(),
            m_case.at("sea").get<i32>(),
            dimension == "minecraft:the_nether" ? 15 : 0,
            dimension == "minecraft:the_nether"    ? 7
                : dimension == "minecraft:the_end" ? 15
                                                   : 0,
            dimension == "minecraft:the_end" ? 15 : 7,
            m_case.at("slimeChance").get<f32>()};
    }

    bool hasNearbyPlayer(const Vector3d& center, f64 distance) const override
    {
        const auto& player = m_case.at("player");
        if (player.is_null() || player.at("mode") == "creative" || player.at("mode") == "spectator") {
            return false;
        }
        const f64 dx = m_x + player.at("x").get<f64>() - center.x;
        const f64 dy = m_y + player.at("y").get<f64>() - center.y;
        const f64 dz = m_z + player.at("z").get<f64>() - center.z;
        return dx * dx + dy * dy + dz * dz < distance * distance;
    }

private:
    const json& m_case;
    i32 m_x;
    i32 m_y;
    i32 m_z;
    std::unordered_map<i32, const BlockState*> m_blocks;
};

std::string _runLine(std::string_view line)
{
    static const bool initialized = [] {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        entity::VanillaEntities::registerAll();
        world::biome::BiomeRegistry::instance().initialize();
        world::biome::BiomeTags::initialize();
        _loadBiomeTags();
        EntitySpawnPlacementRegistry::initializeDefaults();
        return true;
    }();
    MC_UNUSED(initialized);
    const auto input = json::parse(line);
    const auto id = input.at("entity").get<std::string>();
    json cases = json::array();
    for (const auto& scenario : input.at("cases")) {
        _ruleWorld world(scenario);
        const auto reason = getSpawnReasonByName(scenario.at("reason").get<std::string>()).value();
        math::JavaLegacyRandom random(static_cast<u64>(scenario.at("seed").get<i64>()));
        const Vector3i pos(scenario.at("x").get<i32>(), scenario.at("y").get<i32>(), scenario.at("z").get<i32>());
        const bool position = EntitySpawnPlacementRegistry::canSpawnAtLocation(
            EntitySpawnPlacementRegistry::getPlacementType(id), world, pos, id);
        const bool rules = EntitySpawnPlacementRegistry::checkSpawnRules(id, world, reason, pos, random);
        cases.push_back(json{{"name", scenario.at("name")},
            {"positionOk", position},
            {"rulesOk", rules},
            {"nextRandom", random.nextInt(1000)}});
    }
    return json{{"entity", id}, {"cases", std::move(cases)}}.dump();
}

void _writeList(FILE* out)
{
    VanillaBlocks::initialize();
    entity::VanillaEntities::registerAll();
    std::vector<std::string> ids;
    for (const auto* type : entity::EntityRegistry::instance().getAllTypes()) {
        ids.push_back(type->name());
    }
    std::sort(ids.begin(), ids.end());
    for (const auto& id : ids) {
        std::fprintf(out, "%s\n", id.c_str());
    }
}
} // namespace
MC_DIFF_DEFINE_PROBE("spawn_rules", &_writeList, &_runLine)
} // namespace mc::diff
