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

#include "../src/common/world/gen/feature/DecorationStage.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// DecorationStage Tests
// ============================================================================

class DecorationStageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DecorationStageTest, GetAllReturnsCorrectOrder)
{
    const auto& stages = DecorationStages::getAll();

    EXPECT_EQ(stages.size(), static_cast<size_t>(DecorationStage::Count));
    EXPECT_EQ(stages[0], DecorationStage::RawGeneration);
    EXPECT_EQ(stages[1], DecorationStage::Lakes);
    EXPECT_EQ(stages[2], DecorationStage::LocalModifications);
    EXPECT_EQ(stages[3], DecorationStage::UndergroundStructures);
    EXPECT_EQ(stages[4], DecorationStage::SurfaceStructures);
    EXPECT_EQ(stages[5], DecorationStage::Strongholds);
    EXPECT_EQ(stages[6], DecorationStage::UndergroundOres);
    EXPECT_EQ(stages[7], DecorationStage::UndergroundDecoration);
    EXPECT_EQ(stages[8], DecorationStage::FluidSprings);
    EXPECT_EQ(stages[9], DecorationStage::VegetalDecoration);
    EXPECT_EQ(stages[10], DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, GetNameReturnsCorrectStrings)
{
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::RawGeneration), "raw_generation");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::Lakes), "lakes");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::LocalModifications), "local_modifications");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundStructures), "underground_structures");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::SurfaceStructures), "surface_structures");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::Strongholds), "strongholds");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundOres), "underground_ores");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundDecoration), "underground_decoration");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::FluidSprings), "fluid_springs");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::VegetalDecoration), "vegetal_decoration");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::TopLayerModification), "top_layer_modification");
}

TEST_F(DecorationStageTest, GetIndexReturnsCorrectValues)
{
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::RawGeneration), 0);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::Lakes), 1);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::UndergroundOres), 6);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::FluidSprings), 8);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::VegetalDecoration), 9);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::TopLayerModification), 10);
}

TEST_F(DecorationStageTest, FromIndexReturnsCorrectStage)
{
    EXPECT_EQ(DecorationStages::fromIndex(0), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromIndex(6), DecorationStage::UndergroundOres);
    EXPECT_EQ(DecorationStages::fromIndex(8), DecorationStage::FluidSprings);
    EXPECT_EQ(DecorationStages::fromIndex(10), DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, FromIndexInvalidReturnsRawGeneration)
{
    EXPECT_EQ(DecorationStages::fromIndex(100), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromIndex(255), DecorationStage::RawGeneration);
}

TEST_F(DecorationStageTest, FromNameReturnsCorrectStage)
{
    EXPECT_EQ(DecorationStages::fromName("raw_generation"), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromName("lakes"), DecorationStage::Lakes);
    EXPECT_EQ(DecorationStages::fromName("underground_ores"), DecorationStage::UndergroundOres);
    EXPECT_EQ(DecorationStages::fromName("vegetal_decoration"), DecorationStage::VegetalDecoration);
    EXPECT_EQ(DecorationStages::fromName("top_layer_modification"), DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, FromNameInvalidReturnsRawGeneration)
{
    EXPECT_EQ(DecorationStages::fromName("invalid"), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromName(""), DecorationStage::RawGeneration);
}
