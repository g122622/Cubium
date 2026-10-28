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

#include "common/command/StringReader.hpp"
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "probe.hpp"
#include <algorithm>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {
namespace {
using nlohmann::json;
using namespace world::spawn;

class _columnWorld final : public ISpawnWorldReader {
public:
    explicit _columnWorld(const json& scenario)
        : m_below(_parse(scenario.at("below").get<std::string>()))
        , m_at(_parse(scenario.at("at").get<std::string>()))
        , m_above(_parse(scenario.at("above").get<std::string>()))
        , m_insideBorder(scenario.at("insideBorder").get<bool>())
        , m_y(scenario.at("y").get<i32>())
    {}

    const BlockState* getBlockState(i32, i32 y, i32) const override
    {
        return y == m_y - 1 ? m_below : y == m_y ? m_at : y == m_y + 1 ? m_above : &VanillaBlocks::AIR->defaultState();
    }

    bool isWithinWorldBorder(i32, i32, i32) const override
    {
        return m_insideBorder;
    }

    i32 getHeight(HeightmapType, i32, i32) const override
    {
        return m_y;
    }

    BiomeId getBiome(i32, i32, i32) const override
    {
        return BiomeId{0};
    }

    u64 seed() const override
    {
        return 0;
    }

    Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    SpawnEnvironment environment(const Vector3i&) const override
    {
        return SpawnEnvironment{0, 0, 0, false, 63, 0, 0, 7, 0};
    }

    bool hasNearbyPlayer(const Vector3d&, f64) const override
    {
        return false;
    }

private:
    static const BlockState* _parse(const std::string& text)
    {
        command::StringReader reader(text);
        return command::BlockStateArgumentType::blockState()->parse(reader).state();
    }

    const BlockState* m_below;
    const BlockState* m_at;
    const BlockState* m_above;
    bool m_insideBorder;
    i32 m_y;
};

void _initialize()
{
    static const bool initialized = [] {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        entity::VanillaEntities::registerAll();
        EntitySpawnPlacementRegistry::initializeDefaults();
        return true;
    }();
    MC_UNUSED(initialized);
}

std::string _runLine(std::string_view line)
{
    _initialize();
    const auto input = json::parse(line);
    const auto entity = input.at("entity").get<std::string>();
    const auto placement = EntitySpawnPlacementRegistry::getPlacementType(entity);
    const char* name = placement == PlacementType::OnGround ? "on_ground"
        : placement == PlacementType::InWater               ? "in_water"
        : placement == PlacementType::InLava                ? "in_lava"
                                                            : "no_restrictions";
    json cases = json::array();
    for (const auto& scenario : input.at("cases")) {
        _columnWorld world(scenario);
        cases.push_back(json{{"name", scenario.at("name")},
            {"positionOk",
                EntitySpawnPlacementRegistry::canSpawnAtLocation(
                    placement, world, Vector3i(0, scenario.at("y").get<i32>(), 0), entity)}});
    }
    const auto heightmap = EntitySpawnPlacementRegistry::getHeightmapType(entity);
    return json{{"entity", entity},
        {"placement", name},
        {"heightmap", heightmap == HeightmapType::MotionBlocking ? "MOTION_BLOCKING" : "MOTION_BLOCKING_NO_LEAVES"},
        {"cases", std::move(cases)}}
        .dump();
}

void _writeList(FILE* out)
{
    _initialize();
    std::vector<std::string> names;
    for (const auto* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}
} // namespace
MC_DIFF_DEFINE_PROBE("spawn_placement", &_writeList, &_runLine)
} // namespace mc::diff
