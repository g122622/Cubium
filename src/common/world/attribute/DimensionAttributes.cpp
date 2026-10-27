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

#include "common/world/attribute/DimensionAttributes.hpp"

#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/util/TriState.hpp"
#include "common/world/attribute/EnvironmentAttributes.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/level/MoonPhase.hpp"
#include "common/world/timeline/TimelineRegistry.hpp"

#include <spdlog/spdlog.h>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace attribute {
namespace {} // namespace

namespace detail {

bool anyBiomeDeclaresAttribute(const void* attribute)
{
    for (const biome::Biome& biome : biome::BiomeRegistry::instance().allBiomes()) {
        if (biome.attributes().contains(attribute)) {
            return true;
        }
    }
    return false;
}

} // namespace detail

std::unique_ptr<EnvironmentAttributeSystem> DimensionAttributes::createSystem(const DimensionType& dimensionType,
    std::function<i64()> dayTimeGetter,
    std::function<i64()> gameTimeGetter,
    const BiomeResolver& biomeResolver)
{
    EnvironmentAttributeSystemBuilder builder;
    const EnvironmentAttributeMap& constants = dimensionType.attributes();

    // 维度声明的时间线（按声明顺序，即层序）
    std::vector<const timeline::Timeline*> timelines;
    timelines.reserve(dimensionType.timelines().size());
    for (const std::string& name : dimensionType.timelines()) {
        const timeline::TimelineRegistry::Entry* entry = timeline::TimelineRegistry::find(name);
        if (entry == nullptr) {
            spdlog::warn("Dimension declares unknown timeline: {}", name);
            continue;
        }
        timelines.push_back(entry->timeline);
    }

    // 预定义属性集合：显式列举保证顺序稳定
    const auto add = [&](const auto& attribute) {
        detail::appendAttributeLayers(
            builder, attribute, constants, timelines, dayTimeGetter, gameTimeGetter, biomeResolver);
    };

    add(EnvironmentAttributes::FOG_COLOR());
    add(EnvironmentAttributes::FOG_START_DISTANCE());
    add(EnvironmentAttributes::FOG_END_DISTANCE());
    add(EnvironmentAttributes::SKY_FOG_END_DISTANCE());
    add(EnvironmentAttributes::CLOUD_FOG_END_DISTANCE());
    add(EnvironmentAttributes::WATER_FOG_COLOR());
    add(EnvironmentAttributes::WATER_FOG_START_DISTANCE());
    add(EnvironmentAttributes::WATER_FOG_END_DISTANCE());
    add(EnvironmentAttributes::SKY_COLOR());
    add(EnvironmentAttributes::SUNRISE_SUNSET_COLOR());
    add(EnvironmentAttributes::CLOUD_COLOR());
    add(EnvironmentAttributes::CLOUD_HEIGHT());
    add(EnvironmentAttributes::SUN_ANGLE());
    add(EnvironmentAttributes::MOON_ANGLE());
    add(EnvironmentAttributes::STAR_ANGLE());
    add(EnvironmentAttributes::MOON_PHASE());
    add(EnvironmentAttributes::STAR_BRIGHTNESS());
    add(EnvironmentAttributes::SKY_LIGHT_COLOR());
    add(EnvironmentAttributes::SKY_LIGHT_FACTOR());
    add(EnvironmentAttributes::DEFAULT_DRIPSTONE_PARTICLE());
    add(EnvironmentAttributes::AMBIENT_PARTICLES());
    add(EnvironmentAttributes::BACKGROUND_MUSIC());
    add(EnvironmentAttributes::MUSIC_VOLUME());
    add(EnvironmentAttributes::AMBIENT_SOUNDS());
    add(EnvironmentAttributes::FIREFLY_BUSH_SOUNDS());
    add(EnvironmentAttributes::SKY_LIGHT_LEVEL());
    add(EnvironmentAttributes::CAN_START_RAID());
    add(EnvironmentAttributes::WATER_EVAPORATES());
    add(EnvironmentAttributes::RESPAWN_ANCHOR_WORKS());
    add(EnvironmentAttributes::NETHER_PORTAL_SPAWNS_PIGLINS());
    add(EnvironmentAttributes::FAST_LAVA());
    add(EnvironmentAttributes::INCREASED_FIRE_BURNOUT());
    add(EnvironmentAttributes::EYEBLOSSOM_OPEN());
    add(EnvironmentAttributes::TURTLE_EGG_HATCH_CHANCE());
    add(EnvironmentAttributes::PIGLINS_ZOMBIFY());
    add(EnvironmentAttributes::SNOW_GOLEM_MELTS());
    add(EnvironmentAttributes::CREAKING_ACTIVE());
    add(EnvironmentAttributes::SURFACE_SLIME_SPAWN_CHANCE());
    add(EnvironmentAttributes::CAT_WAKING_UP_GIFT_CHANCE());
    add(EnvironmentAttributes::BEES_STAY_IN_HIVE());
    add(EnvironmentAttributes::NAUTILUS_ENTERS_WATER_WHEN_RIDDEN());
    add(EnvironmentAttributes::ARMADILLO_SCUTE_DROP_INTERVAL());
    add(EnvironmentAttributes::MONSTERS_BURN());
    add(EnvironmentAttributes::CAN_PILLAGER_PATROL_SPAWN());
    add(EnvironmentAttributes::VILLAGER_ACTIVITY());
    add(EnvironmentAttributes::BABY_VILLAGER_ACTIVITY());

    return std::make_unique<EnvironmentAttributeSystem>(builder.build());
}

} // namespace attribute
} // namespace world
} // namespace mc
