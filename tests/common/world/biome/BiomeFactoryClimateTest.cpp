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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// BiomeFactory Climate Values Test
//
// Verifies that biome climate values (temperature, humidity, hasPrecipitation,
// temperatureModifier, effects like waterColor) match MC 1.21.11 exactly.
//
// Some tests are intentionally written with EXPECTs that will FAIL until the
// corresponding biome values are fixed to match vanilla MC 1.21.11. These
// are clearly marked with "MC 1.21.11" comments showing the correct value.
// ============================================================================

#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeFactory.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

// Helper shortcuts for readability
using TempMod = BiomeClimate::TemperatureModifier;
using GCM = GrassColorModifier;

class BiomeFactoryClimateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

// ============================================================================
// Overworld Biomes - Correct Values (should PASS now)
// ============================================================================

TEST_F(BiomeFactoryClimateTest, PlainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, DesertClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Desert);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, BadlandsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Badlands);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Badlands has special grass/foliage colors and Badlands grass color modifier
    EXPECT_EQ(biome.effects().grassColorModifier(), GCM::Badlands);
    EXPECT_TRUE(biome.effects().grassColor().has_value());
    EXPECT_EQ(biome.effects().grassColor().value(), BiomeEffects::BADLANDS_GRASS_COLOR);
    EXPECT_TRUE(biome.effects().foliageColor().has_value());
    EXPECT_EQ(biome.effects().foliageColor().value(), BiomeEffects::BADLANDS_FOLIAGE_COLOR);
}

TEST_F(BiomeFactoryClimateTest, ForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Forest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, OceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Ocean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, DeepOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DeepOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SwampClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Swamp);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Swamp has special water colors and Swamp grass color modifier
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::SWAMP_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::SWAMP_WATER_FOG_COLOR);
    EXPECT_EQ(biome.effects().fogColor(), BiomeEffects::SWAMP_FOG_COLOR);
    EXPECT_EQ(biome.effects().grassColorModifier(), GCM::Swamp);
}

TEST_F(BiomeFactoryClimateTest, RiverClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::River);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, BeachClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Beach);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SnowyPlainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SnowyPlains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, DarkForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DarkForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Dark forest has DarkForest grass color modifier
    EXPECT_EQ(biome.effects().grassColorModifier(), GCM::DarkForest);
    EXPECT_TRUE(biome.effects().grassColor().has_value());
    EXPECT_EQ(biome.effects().grassColor().value(), BiomeEffects::DARK_FOREST_GRASS_COLOR);
}

TEST_F(BiomeFactoryClimateTest, BirchForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::BirchForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.6f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.6f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, JungleClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Jungle);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.95f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, WarmOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::WarmOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Warm ocean has special water colors
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::WARM_OCEAN_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::WARM_OCEAN_WATER_FOG_COLOR);
}

TEST_F(BiomeFactoryClimateTest, LukewarmOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::LukewarmOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.6f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR);
}

TEST_F(BiomeFactoryClimateTest, ColdOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::ColdOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.3f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::COLD_OCEAN_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR);
}

TEST_F(BiomeFactoryClimateTest, ErodedBadlandsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::ErodedBadlands);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().grassColorModifier(), GCM::Badlands);
}

TEST_F(BiomeFactoryClimateTest, WoodedBadlandsPlateauClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::WoodedBadlandsPlateau);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SnowyBeachClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SnowyBeach);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.05f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, IceSpikesClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::IceSpikes);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// Savanna Biomes - KNOWN WRONG: temperature and hasPrecipitation need fixing
// MC 1.21.11: Savanna should be temperature=2.0, hasPrecipitation=false
// ============================================================================

TEST_F(BiomeFactoryClimateTest, SavannaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Savanna);
    // MC 1.21.11: temperature should be 2.0, hasPrecipitation should be false
    // Current code has temperature=1.2 and default hasPrecipitation=true
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, ShatteredSavannaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::ShatteredSavanna);
    // MC 1.21.11: temperature should be 2.0, hasPrecipitation should be false
    // Current code has temperature=1.1 and default hasPrecipitation=true
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SavannaPlateauClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SavannaPlateau);
    // MC 1.21.11: temperature should be 2.0, hasPrecipitation should be false
    // Current code has temperature=1.0 and default hasPrecipitation=true
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// MushroomFields - KNOWN WRONG: hasPrecipitation should be true
// MC 1.21.11: has_precipitation=true
// ============================================================================

TEST_F(BiomeFactoryClimateTest, MushroomFieldsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::MushroomFields);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.9f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 1.0f);
    // MC 1.21.11: has_precipitation=true
    // Current code incorrectly sets hasPrecipitation=false
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// FrozenOcean and FrozenRiver - KNOWN WRONG: temperatureModifier should be Frozen
// MC 1.21.11: temperature_modifier="frozen", hasPrecipitation=true
// ============================================================================

TEST_F(BiomeFactoryClimateTest, FrozenOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::FrozenOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    // MC 1.21.11: hasPrecipitation should be true (snow)
    EXPECT_TRUE(biome.hasPrecipitation());
    // MC 1.21.11: temperatureModifier should be Frozen
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::Frozen);
    // Frozen ocean has special water color
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::FROZEN_OCEAN_WATER_COLOR);
}

TEST_F(BiomeFactoryClimateTest, FrozenRiverClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::FrozenRiver);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    // MC 1.21.11: hasPrecipitation should be true (snow)
    EXPECT_TRUE(biome.hasPrecipitation());
    // MC 1.21.11: temperatureModifier should be Frozen
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::Frozen);
    // Frozen river uses frozen ocean water color
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::FROZEN_OCEAN_WATER_COLOR);
}

TEST_F(BiomeFactoryClimateTest, DeepFrozenOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DeepFrozenOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    // MC 1.21.11: hasPrecipitation should be true (snow)
    EXPECT_TRUE(biome.hasPrecipitation());
    // MC 1.21.11: temperatureModifier should be Frozen
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::Frozen);
    // Deep frozen ocean uses frozen ocean water color
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::FROZEN_OCEAN_WATER_COLOR);
}

// ============================================================================
// DeepDark - KNOWN WRONG: temperature and hasPrecipitation need fixing
// MC 1.21.11: temperature=0.8, hasPrecipitation=true
// ============================================================================

TEST_F(BiomeFactoryClimateTest, DeepDarkClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DeepDark);
    // MC 1.21.11: temperature should be 0.8
    // Current code has temperature=0.0
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    // MC 1.21.11: hasPrecipitation should be true
    // Current code has hasPrecipitation=false
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// Taiga - KNOWN WRONG: temperature should be 0.25
// MC 1.21.11: temperature=0.25 (non-snowy taiga)
// ============================================================================

TEST_F(BiomeFactoryClimateTest, TaigaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Taiga);
    // MC 1.21.11: temperature should be 0.25 (non-snowy taiga has temp 0.25, not -0.5)
    // Current code has temperature=-0.5
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.25f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// JungleEdge/SparseJungle - KNOWN WRONG: temperature should be 0.8
// MC 1.21.11: temperature=0.8 (not 0.95)
// ============================================================================

TEST_F(BiomeFactoryClimateTest, JungleEdgeClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::JungleEdge);
    // MC 1.21.11: temperature should be 0.8
    // Current code has temperature=0.95
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// SnowyTaiga - KNOWN WRONG: waterColor should be 0x3D57E6
// MC 1.21.11: water_color=4020182 (0x3D57E6)
// ============================================================================

TEST_F(BiomeFactoryClimateTest, SnowyTaigaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SnowyTaiga);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // MC 1.21.11: waterColor should be 0x3D57E6 (4020182)
    // Current code uses default water color
    EXPECT_EQ(biome.effects().waterColor(), 0x3D57E6u);
}

// ============================================================================
// Additional overworld biomes - verifying current values
// ============================================================================

TEST_F(BiomeFactoryClimateTest, MountainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Mountains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.2f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, StoneShoreClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::StoneShore);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.2f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, GiantTreeTaigaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::GiantTreeTaiga);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.3f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, WoodedMountainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::WoodedMountains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.2f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, BambooJungleClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::BambooJungle);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.95f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, FlowerForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::FlowerForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, TallBirchForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::TallBirchForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.6f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.6f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, GiantSpruceTaigaClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::GiantSpruceTaiga);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.25f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SunflowerPlainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SunflowerPlains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, GravellyMountainsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::GravellyMountains);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.2f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// Cave & Cliffs biomes - verifying values
// ============================================================================

TEST_F(BiomeFactoryClimateTest, MeadowClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Meadow);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, GroveClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Grove);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.2f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SnowySlopesClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SnowySlopes);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.3f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, JaggedPeaksClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::JaggedPeaks);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, FrozenPeaksClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::FrozenPeaks);
    EXPECT_FLOAT_EQ(biome.climate().temperature, -0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, StonyPeaksClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::StonyPeaks);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 1.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.3f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, DripstoneCavesClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DripstoneCaves);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.4f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, LushCavesClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::LushCaves);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, MangroveSwampClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::MangroveSwamp);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.8f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.9f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Mangrove swamp has special water colors and Swamp grass color modifier
    EXPECT_EQ(biome.effects().waterColor(), 0x3A7F3Eu);
    EXPECT_EQ(biome.effects().waterFogColor(), 0x4E6D51u);
    EXPECT_EQ(biome.effects().grassColorModifier(), GCM::Swamp);
}

TEST_F(BiomeFactoryClimateTest, CherryGroveClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::CherryGrove);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Cherry grove has special grass and foliage colors
    EXPECT_TRUE(biome.effects().grassColor().has_value());
    EXPECT_EQ(biome.effects().grassColor().value(), 0xB69FE1u);
    EXPECT_TRUE(biome.effects().foliageColor().has_value());
    EXPECT_EQ(biome.effects().foliageColor().value(), 0xB69FE1u);
}

TEST_F(BiomeFactoryClimateTest, PaleGardenClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::PaleGarden);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.7f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.8f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    // Pale garden has dry foliage color (MC 1.21.11: 0xA0C486)
    EXPECT_TRUE(biome.effects().dryFoliageColor().has_value());
    EXPECT_EQ(biome.effects().dryFoliageColor().value(), 0xA0C486u);
}

// ============================================================================
// Deep ocean variants - verifying current values
// ============================================================================

TEST_F(BiomeFactoryClimateTest, DeepLukewarmOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DeepLukewarmOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.6f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::LUKEWARM_OCEAN_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::LUKEWARM_OCEAN_WATER_FOG_COLOR);
}

TEST_F(BiomeFactoryClimateTest, DeepColdOceanClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::DeepColdOcean);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.3f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_TRUE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::COLD_OCEAN_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::COLD_OCEAN_WATER_FOG_COLOR);
}

// ============================================================================
// Nether Biomes - all share same climate: temp=2.0, humidity=0.0, hasPrecipitation=false
// ============================================================================

TEST_F(BiomeFactoryClimateTest, NetherWastesClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::NetherWastes);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, SoulSandValleyClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SoulSandValley);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, CrimsonForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::CrimsonForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, WarpedForestClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::WarpedForest);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, BasaltDeltasClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::BasaltDeltas);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.0f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// Nether biome fog colors (each has a unique fog color)
TEST_F(BiomeFactoryClimateTest, NetherFogColors)
{
    // Nether Wastes: fogColor=3344392 (0x33071D... actually just decimal)
    EXPECT_EQ(BiomeRegistry::instance().get(Biomes::NetherWastes).effects().fogColor(), 3344392u);
    // Soul Sand Valley: fogColor=1787717
    EXPECT_EQ(BiomeRegistry::instance().get(Biomes::SoulSandValley).effects().fogColor(), 1787717u);
    // Crimson Forest: fogColor=3343107
    EXPECT_EQ(BiomeRegistry::instance().get(Biomes::CrimsonForest).effects().fogColor(), 3343107u);
    // Warped Forest: fogColor=1705242
    EXPECT_EQ(BiomeRegistry::instance().get(Biomes::WarpedForest).effects().fogColor(), 1705242u);
    // Basalt Deltas: fogColor=6840176
    EXPECT_EQ(BiomeRegistry::instance().get(Biomes::BasaltDeltas).effects().fogColor(), 6840176u);
}

// ============================================================================
// End Biomes - all share same climate: temp=0.5, humidity=0.5, hasPrecipitation=false
// ============================================================================

TEST_F(BiomeFactoryClimateTest, TheEndClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::TheEnd);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
    EXPECT_EQ(biome.effects().fogColor(), 10518688u);
}

TEST_F(BiomeFactoryClimateTest, SmallEndIslandsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::SmallEndIslands);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, EndMidlandsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::EndMidlands);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, EndHighlandsClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::EndHighlands);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

TEST_F(BiomeFactoryClimateTest, EndBarrensClimate)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::EndBarrens);
    EXPECT_FLOAT_EQ(biome.climate().temperature, 0.5f);
    EXPECT_FLOAT_EQ(biome.climate().humidity, 0.5f);
    EXPECT_FALSE(biome.hasPrecipitation());
    EXPECT_EQ(biome.climate().temperatureModifier, TempMod::None);
}

// ============================================================================
// Precipitation consistency checks
// Biomes with temperature >= 0.15 should have hasPrecipitation=true (Rain via getPrecipitationAt)
// Biomes with temperature < 0.15 should have hasPrecipitation=true (Snow via getPrecipitationAt)
// Biomes with temperature >= 1.0 and downfall=0.0 in hot biomes should have hasPrecipitation=false
// ============================================================================

TEST_F(BiomeFactoryClimateTest, HotBiomesHaveNoPrecipitation)
{
    // All biomes with temperature >= 2.0 should have hasPrecipitation=false
    // because MC 1.21.11 sets has_precipitation=false for these hot biomes.
    // 注：仅含 1.21.11 仍存在的 hot biome（DesertHills/DesertLakes/BadlandsPlateau/
    // ModifiedBadlandsPlateau/ModifiedWoodedBadlandsPlateau 等为 1.16.5 已删除变体，
    // BiomeRegistry 不注册，已从列表移除）。
    const BiomeId hotBiomes[] = {
        Biomes::Desert,
        Biomes::Badlands,
        Biomes::ErodedBadlands,
        Biomes::WoodedBadlandsPlateau,
    };

    for (BiomeId id : hotBiomes) {
        SCOPED_TRACE("Biome ID " + std::to_string(id));
        const auto& biome = BiomeRegistry::instance().get(id);
        EXPECT_FALSE(biome.hasPrecipitation())
            << "Biome " << biome.name() << " (temp=" << biome.climate().temperature << ") should have no precipitation";
    }
}

TEST_F(BiomeFactoryClimateTest, ColdBiomesHaveSnowPrecipitation)
{
    // Biomes with temperature < 0.15 should have precipitation (Snow via getPrecipitationAt)
    const BiomeId coldBiomes[] = {
        Biomes::SnowyPlains,
        Biomes::SnowyBeach,
        Biomes::SnowyMountains,
        Biomes::IceSpikes,
        Biomes::SnowyTaiga,
        Biomes::SnowyTaigaHills,
        Biomes::SnowyTaigaMountains,
        Biomes::Grove,
        Biomes::SnowySlopes,
        Biomes::JaggedPeaks,
        Biomes::FrozenPeaks,
    };

    for (BiomeId id : coldBiomes) {
        SCOPED_TRACE("Biome ID " + std::to_string(id));
        const auto& biome = BiomeRegistry::instance().get(id);
        EXPECT_TRUE(biome.hasPrecipitation()) << "Biome " << biome.name() << " (temp=" << biome.climate().temperature
                                              << ") should have precipitation (snow)";
    }
}

TEST_F(BiomeFactoryClimateTest, FrozenBiomesHaveFrozenTemperatureModifier)
{
    // MC 1.21.11: frozen_ocean, frozen_river, and deep_frozen_ocean should
    // have TemperatureModifier::Frozen
    const BiomeId frozenBiomes[] = {
        Biomes::FrozenOcean,
        Biomes::FrozenRiver,
        Biomes::DeepFrozenOcean,
    };

    for (BiomeId id : frozenBiomes) {
        SCOPED_TRACE("Biome ID " + std::to_string(id));
        const auto& biome = BiomeRegistry::instance().get(id);
        EXPECT_EQ(biome.climate().temperatureModifier, TempMod::Frozen)
            << "Biome " << biome.name() << " should have Frozen temperatureModifier";
        EXPECT_TRUE(biome.hasPrecipitation()) << "Biome " << biome.name() << " should have precipitation (snow)";
    }
}

// ============================================================================
// Savanna family - comprehensive check for known issues
// All savanna biomes should be temperature=2.0, hasPrecipitation=false in MC 1.21.11
// ============================================================================

TEST_F(BiomeFactoryClimateTest, AllSavannaBiomesHaveHotDryClimate)
{
    // MC 1.21.11: All savanna biomes have temperature=2.0 and hasPrecipitation=false
    const BiomeId savannaBiomes[] = {
        Biomes::Savanna, Biomes::ShatteredSavanna, Biomes::SavannaPlateau,
        // 注：ShatteredSavannaPlateau 是 1.16.5 已删除变体，1.21.11 已移除，
        // BiomeRegistry 有意不注册（对齐 vanilla），故不纳入本聚合断言。
    };

    for (BiomeId id : savannaBiomes) {
        SCOPED_TRACE("Biome ID " + std::to_string(id));
        const auto& biome = BiomeRegistry::instance().get(id);
        EXPECT_FLOAT_EQ(biome.climate().temperature, 2.0f)
            << "Biome " << biome.name() << " should have temperature 2.0";
        EXPECT_FALSE(biome.hasPrecipitation()) << "Biome " << biome.name() << " should have no precipitation";
    }
}

// ============================================================================
// Water color verification for biomes with special water colors
// ============================================================================

TEST_F(BiomeFactoryClimateTest, DefaultWaterColors)
{
    // Biomes without special water color should use the default
    const BiomeId defaultWaterBiomes[] = {
        Biomes::Plains,
        Biomes::Desert,
        Biomes::Forest,
        Biomes::Mountains,
        Biomes::Taiga,
        Biomes::Beach,
    };

    for (BiomeId id : defaultWaterBiomes) {
        SCOPED_TRACE("Biome ID " + std::to_string(id));
        const auto& biome = BiomeRegistry::instance().get(id);
        EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::DEFAULT_WATER_COLOR)
            << "Biome " << biome.name() << " should have default water color";
        EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::DEFAULT_WATER_FOG_COLOR)
            << "Biome " << biome.name() << " should have default water fog color";
    }
}

TEST_F(BiomeFactoryClimateTest, SwampWaterColors)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::Swamp);
    EXPECT_EQ(biome.effects().waterColor(), BiomeEffects::SWAMP_WATER_COLOR);
    EXPECT_EQ(biome.effects().waterFogColor(), BiomeEffects::SWAMP_WATER_FOG_COLOR);
}

TEST_F(BiomeFactoryClimateTest, CherryGroveWaterColor)
{
    const auto& biome = BiomeRegistry::instance().get(Biomes::CherryGrove);
    // Cherry grove has a unique water color: 0x5D93DF
    EXPECT_EQ(biome.effects().waterColor(), 0x5D93DFu);
}

} // namespace
} // namespace mc
