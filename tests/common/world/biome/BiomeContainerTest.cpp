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

#include <gtest/gtest.h>

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

namespace mc {
namespace {

using namespace world::chunk;
using namespace world::biome;
using namespace world;

// ============================================================================
// Constants for readability
// ============================================================================

static constexpr i32 H = BiomeContainer::HORIZ_SIZE;                 // 4
static constexpr i32 V = BiomeContainer::VERT_SIZE;                  // 4
static constexpr i32 SEC_BIOME = BiomeContainer::SECTION_BIOME_SIZE; // 64
static constexpr i32 SEC_COUNT = BiomeContainer::SECTION_COUNT;      // 24
static constexpr i32 TOTAL = BiomeContainer::TOTAL_SIZE;             // 1536

// ============================================================================
// Test fixture
// ============================================================================

class BiomeContainerTest : public ::testing::Test {
protected:
    BiomeContainer container;
};

// ============================================================================
// 1. Default construction - all biome IDs are zero
// ============================================================================

TEST_F(BiomeContainerTest, DefaultConstruction_AllBiomesAreZero)
{
    BiomeContainer fresh;
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    EXPECT_EQ(fresh.getBiome(sec, x, y, z), BiomeId(0))
                        << "Default biome at sec=" << sec << " x=" << x << " y=" << y << " z=" << z << " should be 0";
                }
            }
        }
    }
}

TEST_F(BiomeContainerTest, DefaultConstruction_SerializedSizeIsCorrect)
{
    BiomeContainer fresh;
    auto data = fresh.serialize();
    // Each BiomeId (u16) is serialized as 2 bytes
    EXPECT_EQ(data.size(), static_cast<size_t>(TOTAL * 2));
}

// ============================================================================
// 2. setBiome / getBiome round-trip for valid indices
// ============================================================================

TEST_F(BiomeContainerTest, SetGetBiome_RoundTrip_Section0)
{
    container.setBiome(0, 1, 2, 3, Biomes::Plains);
    EXPECT_EQ(container.getBiome(0, 1, 2, 3), Biomes::Plains);
}

TEST_F(BiomeContainerTest, SetGetBiome_RoundTrip_LastSection)
{
    const i32 lastSec = SEC_COUNT - 1;
    container.setBiome(lastSec, 3, 3, 3, Biomes::Desert);
    EXPECT_EQ(container.getBiome(lastSec, 3, 3, 3), Biomes::Desert);
}

TEST_F(BiomeContainerTest, SetGetBiome_RoundTrip_Origin)
{
    container.setBiome(0, 0, 0, 0, Biomes::Forest);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Forest);
}

TEST_F(BiomeContainerTest, SetGetBiome_RoundTrip_MultiplePositions)
{
    // Write different biomes to several positions and verify each
    container.setBiome(0, 0, 0, 0, Biomes::Ocean);
    container.setBiome(5, 2, 1, 3, Biomes::Taiga);
    container.setBiome(12, 3, 2, 1, Biomes::Jungle);
    container.setBiome(23, 1, 3, 2, Biomes::DeepOcean);

    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Ocean);
    EXPECT_EQ(container.getBiome(5, 2, 1, 3), Biomes::Taiga);
    EXPECT_EQ(container.getBiome(12, 3, 2, 1), Biomes::Jungle);
    EXPECT_EQ(container.getBiome(23, 1, 3, 2), Biomes::DeepOcean);
}

TEST_F(BiomeContainerTest, SetGetBiome_HighBiomeIdPreserved)
{
    // BiomeId is u16; verify high IDs like PaleGarden (185) are stored correctly
    container.setBiome(0, 0, 0, 0, Biomes::PaleGarden);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::PaleGarden);
}

TEST_F(BiomeContainerTest, SetGetBiome_AllPositionsInSection)
{
    // Fill every position in section 0 with a distinct value
    BiomeId id = 1;
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                container.setBiome(0, x, y, z, id++);
            }
        }
    }
    // Verify round-trip
    id = 1;
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                EXPECT_EQ(container.getBiome(0, x, y, z), id) << "Mismatch at x=" << x << " y=" << y << " z=" << z;
                ++id;
            }
        }
    }
}

// ============================================================================
// 3. getBiomeAtBlock coordinate mapping (quart conversion)
// ============================================================================

TEST_F(BiomeContainerTest, GetBiomeAtBlock_Block0_0_0_MapsToQuart0_0_0)
{
    // Block (0, -64, 0) is at the bottom of section 0, quart (0,0,0)
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT, 0), Biomes::Plains);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_Block4_0_4_MapsToQuart1_0_1)
{
    // X=4, Z=4 -> quart X=1, Z=1; Y=-64 -> section 0, quart Y=0
    container.setBiome(0, 1, 0, 1, Biomes::Desert);
    EXPECT_EQ(container.getBiomeAtBlock(4, MIN_BUILD_HEIGHT, 4), Biomes::Desert);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_Block15_0_15_ClampsToQuart3)
{
    // X=15 -> 15>>2 = 3, Z=15 -> 15>>2 = 3 (clamped by HORIZ_SIZE-1=3)
    container.setBiome(0, 3, 0, 3, Biomes::Forest);
    EXPECT_EQ(container.getBiomeAtBlock(15, MIN_BUILD_HEIGHT, 15), Biomes::Forest);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_Block7_0_7_MapsToQuart1)
{
    // 7>>2 = 1
    container.setBiome(0, 1, 0, 1, Biomes::Taiga);
    EXPECT_EQ(container.getBiomeAtBlock(7, MIN_BUILD_HEIGHT, 7), Biomes::Taiga);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_YQuartMapping)
{
    // Y = -64 + 4 = -60 -> yOffset = 4, section = 0, biomeY = 4>>2 = 1
    container.setBiome(0, 0, 1, 0, Biomes::Swamp);
    EXPECT_EQ(container.getBiomeAtBlock(0, -60, 0), Biomes::Swamp);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_YCrossesSectionBoundary)
{
    // Y = -64 + 16 = -48 -> yOffset = 16, section = 1, biomeY = 0
    container.setBiome(1, 0, 0, 0, Biomes::River);
    EXPECT_EQ(container.getBiomeAtBlock(0, -48, 0), Biomes::River);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_YMidSection)
{
    // Y = -64 + 24 = -40 -> yOffset = 24, section = 1, biomeY = (24%16)>>2 = 8>>2 = 2
    container.setBiome(1, 0, 2, 0, Biomes::NetherWastes);
    EXPECT_EQ(container.getBiomeAtBlock(0, -40, 0), Biomes::NetherWastes);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_SectionIndexCalculation)
{
    // Verify section index for Y values across the range
    // sectionIndex = (y - MIN_BUILD_HEIGHT) / CHUNK_SECTION_HEIGHT
    // Y = 0 -> yOffset = 64, section = 64/16 = 4
    container.setBiome(4, 0, 0, 0, Biomes::TheEnd);
    EXPECT_EQ(container.getBiomeAtBlock(0, 0, 0), Biomes::TheEnd);

    // Y = 64 -> yOffset = 128, section = 128/16 = 8
    container.setBiome(8, 0, 0, 0, Biomes::Beach);
    EXPECT_EQ(container.getBiomeAtBlock(0, 64, 0), Biomes::Beach);

    // Y = 256 -> yOffset = 320, section = 320/16 = 20
    container.setBiome(20, 0, 0, 0, Biomes::Meadow);
    EXPECT_EQ(container.getBiomeAtBlock(0, 256, 0), Biomes::Meadow);
}

// ============================================================================
// 4. Section index calculation for various Y values
// ============================================================================

TEST_F(BiomeContainerTest, SectionIndex_MinBuildHeight_IsSection0)
{
    // Y = MIN_BUILD_HEIGHT -> section 0
    container.setBiome(0, 0, 0, 0, Biomes::FrozenOcean);
    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT, 0), Biomes::FrozenOcean);
}

TEST_F(BiomeContainerTest, SectionIndex_TopOfSection0)
{
    // Y = MIN_BUILD_HEIGHT + 15 = -49 -> still section 0
    container.setBiome(0, 0, 3, 0, Biomes::FrozenRiver);
    EXPECT_EQ(container.getBiomeAtBlock(0, -49, 0), Biomes::FrozenRiver);
}

TEST_F(BiomeContainerTest, SectionIndex_BottomOfSection1)
{
    // Y = MIN_BUILD_HEIGHT + 16 = -48 -> section 1
    container.setBiome(1, 0, 0, 0, Biomes::SnowyPlains);
    EXPECT_EQ(container.getBiomeAtBlock(0, -48, 0), Biomes::SnowyPlains);
}

TEST_F(BiomeContainerTest, SectionIndex_LastValidSection)
{
    // Last valid Y = MAX_BUILD_HEIGHT - 1 = 319
    // yOffset = 319 - (-64) = 383, section = 383/16 = 23 (integer division)
    container.setBiome(23, 0, 3, 0, Biomes::CherryGrove);
    EXPECT_EQ(container.getBiomeAtBlock(0, 319, 0), Biomes::CherryGrove);
}

// ============================================================================
// 5. Negative Y coordinates handling
// ============================================================================

TEST_F(BiomeContainerTest, NegativeY_MinBuildHeight)
{
    container.setBiome(0, 0, 0, 0, Biomes::DeepDark);
    EXPECT_EQ(container.getBiomeAtBlock(0, -64, 0), Biomes::DeepDark);
}

TEST_F(BiomeContainerTest, NegativeY_NegativeOne)
{
    // Y = -1 -> yOffset = 63, section = 63/16 = 3, biomeY = (63%16)>>2 = 15>>2 = 3
    container.setBiome(3, 0, 3, 0, Biomes::DripstoneCaves);
    EXPECT_EQ(container.getBiomeAtBlock(0, -1, 0), Biomes::DripstoneCaves);
}

TEST_F(BiomeContainerTest, NegativeY_Negative32)
{
    // Y = -32 -> yOffset = 32, section = 32/16 = 2, biomeY = 0
    container.setBiome(2, 0, 0, 0, Biomes::LushCaves);
    EXPECT_EQ(container.getBiomeAtBlock(0, -32, 0), Biomes::LushCaves);
}

// ============================================================================
// 6. Boundary conditions
// ============================================================================

TEST_F(BiomeContainerTest, Boundary_MIN_BUILD_HEIGHT)
{
    container.setBiome(0, 0, 0, 0, Biomes::Ocean);
    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT, 0), Biomes::Ocean);
}

TEST_F(BiomeContainerTest, Boundary_MAX_BUILD_HEIGHT_MinusOne)
{
    // Y = 319 is the last valid Y coordinate
    // yOffset = 383, section = 23, biomeY = (383%16)>>2 = 15>>2 = 3
    container.setBiome(23, 0, 3, 0, Biomes::Ocean);
    EXPECT_EQ(container.getBiomeAtBlock(0, MAX_BUILD_HEIGHT - 1, 0), Biomes::Ocean);
}

TEST_F(BiomeContainerTest, Boundary_AboveMaxBuildHeight_ReturnsZero)
{
    // Y = 320 is out of range, should return 0
    EXPECT_EQ(container.getBiomeAtBlock(0, MAX_BUILD_HEIGHT, 0), BiomeId(0));
}

TEST_F(BiomeContainerTest, Boundary_BelowMinBuildHeight_ReturnsZero)
{
    // Y = -65 is below MIN_BUILD_HEIGHT, should return 0
    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT - 1, 0), BiomeId(0));
}

TEST_F(BiomeContainerTest, Boundary_MaxBlockXZ_ClampsCorrectly)
{
    // X=15, Z=15 -> quart 3,3
    container.setBiome(0, 3, 0, 3, Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(15, MIN_BUILD_HEIGHT, 15), Biomes::Plains);
}

TEST_F(BiomeContainerTest, Boundary_OutOfRangeXZ_ClampsToValidQuart)
{
    // X=16 -> 16>>2 = 4, clamped to 3; Z=16 -> clamped to 3
    container.setBiome(0, 3, 0, 3, Biomes::Desert);
    EXPECT_EQ(container.getBiomeAtBlock(16, MIN_BUILD_HEIGHT, 16), Biomes::Desert);
}

TEST_F(BiomeContainerTest, Boundary_NegativeXZ_ClampsToZero)
{
    // X=-1 -> -1>>2 = -1 (arithmetic shift), clamped to 0
    container.setBiome(0, 0, 0, 0, Biomes::Forest);
    EXPECT_EQ(container.getBiomeAtBlock(-1, MIN_BUILD_HEIGHT, 0), Biomes::Forest);
    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT, -1), Biomes::Forest);
}

// ============================================================================
// 7. setBiome at out-of-range local coordinates does not crash
// ============================================================================

TEST_F(BiomeContainerTest, SetBiome_OutOfRangeX_SilentlyIgnored)
{
    // x out of range [0,4) should be silently ignored per the bounds check
    container.setBiome(0, 4, 0, 0, Biomes::Plains);
    // The biome at (0,0,0) should still be default 0 since the set was ignored
    // and the out-of-range position was not written
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), BiomeId(0));
}

TEST_F(BiomeContainerTest, SetBiome_OutOfRangeY_SilentlyIgnored)
{
    container.setBiome(0, 0, 4, 0, Biomes::Desert);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), BiomeId(0));
}

TEST_F(BiomeContainerTest, SetBiome_OutOfRangeZ_SilentlyIgnored)
{
    container.setBiome(0, 0, 0, 4, Biomes::Forest);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), BiomeId(0));
}

TEST_F(BiomeContainerTest, GetBiome_OutOfRangeLocalCoords_ReturnsZero)
{
    EXPECT_EQ(container.getBiome(0, 4, 0, 0), BiomeId(0));
    EXPECT_EQ(container.getBiome(0, 0, 4, 0), BiomeId(0));
    EXPECT_EQ(container.getBiome(0, 0, 0, 4), BiomeId(0));
    EXPECT_EQ(container.getBiome(0, -1, 0, 0), BiomeId(0));
    EXPECT_EQ(container.getBiome(0, 0, -1, 0), BiomeId(0));
    EXPECT_EQ(container.getBiome(0, 0, 0, -1), BiomeId(0));
}

// ============================================================================
// 8. Overwriting a biome replaces the previous value
// ============================================================================

TEST_F(BiomeContainerTest, OverwriteBiome_ReplacesPreviousValue)
{
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Plains);

    container.setBiome(0, 0, 0, 0, Biomes::Desert);
    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Desert);
}

TEST_F(BiomeContainerTest, OverwriteBiome_DoesNotAffectOtherPositions)
{
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    container.setBiome(0, 1, 0, 0, Biomes::Desert);

    container.setBiome(0, 0, 0, 0, Biomes::Forest);

    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Forest);
    EXPECT_EQ(container.getBiome(0, 1, 0, 0), Biomes::Desert);
}

TEST_F(BiomeContainerTest, OverwriteBiome_AcrossSections)
{
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    container.setBiome(1, 0, 0, 0, Biomes::Desert);

    container.setBiome(0, 0, 0, 0, Biomes::Forest);

    EXPECT_EQ(container.getBiome(0, 0, 0, 0), Biomes::Forest);
    EXPECT_EQ(container.getBiome(1, 0, 0, 0), Biomes::Desert);
}

// ============================================================================
// 9. Serialization / deserialization round-trip
// ============================================================================

TEST_F(BiomeContainerTest, SerializeDeserialize_EmptyContainer)
{
    BiomeContainer original;
    auto data = original.serialize();

    auto result = BiomeContainer::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());

    const auto& restored = result.value();
    // All biomes should be 0 (default)
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    EXPECT_EQ(restored.getBiome(sec, x, y, z), BiomeId(0));
                }
            }
        }
    }
}

TEST_F(BiomeContainerTest, SerializeDeserialize_PopulatedContainer)
{
    // Populate with a pattern: each section gets a different biome
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        BiomeId biome = static_cast<BiomeId>(sec + 1); // IDs 1..24
        container.setBiome(sec, 0, 0, 0, biome);
        container.setBiome(sec, 3, 3, 3, biome);
    }

    auto data = container.serialize();
    auto result = BiomeContainer::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());

    const auto& restored = result.value();
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        BiomeId expected = static_cast<BiomeId>(sec + 1);
        EXPECT_EQ(restored.getBiome(sec, 0, 0, 0), expected);
        EXPECT_EQ(restored.getBiome(sec, 3, 3, 3), expected);
    }
}

TEST_F(BiomeContainerTest, SerializeDeserialize_HighBiomeId)
{
    // PaleGarden = 185, stored as 2-byte little-endian: 0xB9, 0x00
    container.setBiome(0, 0, 0, 0, Biomes::PaleGarden);
    auto data = container.serialize();

    // Verify raw bytes for the first biome entry
    ASSERT_GE(data.size(), 2u);
    EXPECT_EQ(data[0], 0xB9); // low byte of 185
    EXPECT_EQ(data[1], 0x00); // high byte of 185

    auto result = BiomeContainer::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getBiome(0, 0, 0, 0), Biomes::PaleGarden);
}

TEST_F(BiomeContainerTest, SerializeDeserialize_FullContainer)
{
    // Fill every position with a unique value based on index
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    // Use a value that fits in u16, cycling through biome IDs
                    i32 idx = sec * SEC_BIOME + y * H * H + z * H + x;
                    container.setBiome(sec, x, y, z, static_cast<BiomeId>(idx % 256));
                }
            }
        }
    }

    auto data = container.serialize();
    auto result = BiomeContainer::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());

    const auto& restored = result.value();
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    i32 idx = sec * SEC_BIOME + y * H * H + z * H + x;
                    BiomeId expected = static_cast<BiomeId>(idx % 256);
                    EXPECT_EQ(restored.getBiome(sec, x, y, z), expected)
                        << "Mismatch at sec=" << sec << " x=" << x << " y=" << y << " z=" << z;
                }
            }
        }
    }
}

TEST_F(BiomeContainerTest, Deserialize_DataTooSmall_ReturnsError)
{
    std::vector<u8> smallData(TOTAL * 2 - 1, 0); // One byte too small
    auto result = BiomeContainer::deserialize(smallData.data(), smallData.size());
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

TEST_F(BiomeContainerTest, Deserialize_DataTooSmall_EmptyData)
{
    auto result = BiomeContainer::deserialize(nullptr, 0);
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::InvalidArgument);
}

TEST_F(BiomeContainerTest, Deserialize_LargerData_Success)
{
    // Extra data beyond the expected size should still succeed
    BiomeContainer original;
    original.setBiome(0, 0, 0, 0, Biomes::Plains);
    auto data = original.serialize();
    // Append extra bytes
    data.push_back(0xFF);
    data.push_back(0xFF);

    auto result = BiomeContainer::deserialize(data.data(), data.size());
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value().getBiome(0, 0, 0, 0), Biomes::Plains);
}

// ============================================================================
// 10. Collecting all unique biome IDs in the container
// ============================================================================

TEST_F(BiomeContainerTest, GetAllBiomes_EmptyContainer_ReturnsZeroOnly)
{
    BiomeContainer fresh;
    std::set<BiomeId> biomes;
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    biomes.insert(fresh.getBiome(sec, x, y, z));
                }
            }
        }
    }
    ASSERT_EQ(biomes.size(), 1u);
    EXPECT_EQ(*biomes.begin(), BiomeId(0));
}

TEST_F(BiomeContainerTest, GetAllBiomes_MultipleBiomes_ReturnsAllUnique)
{
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    container.setBiome(1, 0, 0, 0, Biomes::Desert);
    container.setBiome(2, 0, 0, 0, Biomes::Forest);
    container.setBiome(3, 0, 0, 0, Biomes::Plains); // duplicate

    std::set<BiomeId> biomes;
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        for (i32 y = 0; y < V; ++y) {
            for (i32 z = 0; z < H; ++z) {
                for (i32 x = 0; x < H; ++x) {
                    biomes.insert(container.getBiome(sec, x, y, z));
                }
            }
        }
    }
    // Should contain 0 (default), Plains(1), Desert(2), Forest(4)
    EXPECT_EQ(biomes.count(BiomeId(0)), 1u);
    EXPECT_EQ(biomes.count(Biomes::Plains), 1u);
    EXPECT_EQ(biomes.count(Biomes::Desert), 1u);
    EXPECT_EQ(biomes.count(Biomes::Forest), 1u);
    EXPECT_EQ(biomes.size(), 4u);
}

// ============================================================================
// 11. getBiomeAtBlock comprehensive coordinate mapping
// ============================================================================

TEST_F(BiomeContainerTest, GetBiomeAtBlock_AllQuartPositionsInFirstSection)
{
    // Test every quart position in the first section via getBiomeAtBlock
    for (i32 qy = 0; qy < V; ++qy) {
        for (i32 qz = 0; qz < H; ++qz) {
            for (i32 qx = 0; qx < H; ++qx) {
                BiomeId id = static_cast<BiomeId>(qy * H * H + qz * H + qx + 1);
                container.setBiome(0, qx, qy, qz, id);
            }
        }
    }

    // Verify via block coordinates: block X = qx*4, Y = MIN_BUILD_HEIGHT + qy*4, Z = qz*4
    for (i32 qy = 0; qy < V; ++qy) {
        for (i32 qz = 0; qz < H; ++qz) {
            for (i32 qx = 0; qx < H; ++qx) {
                i32 blockX = qx * 4;
                i32 blockY = MIN_BUILD_HEIGHT + qy * 4;
                i32 blockZ = qz * 4;
                BiomeId expected = static_cast<BiomeId>(qy * H * H + qz * H + qx + 1);
                EXPECT_EQ(container.getBiomeAtBlock(blockX, blockY, blockZ), expected)
                    << "Quart(" << qx << "," << qy << "," << qz << ") block(" << blockX << "," << blockY << ","
                    << blockZ << ")";
            }
        }
    }
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_IntermediateBlockCoordinates)
{
    // Blocks 0-3 all map to quart 0, blocks 4-7 to quart 1, etc.
    container.setBiome(0, 0, 0, 0, Biomes::Plains);
    container.setBiome(0, 1, 0, 0, Biomes::Desert);
    container.setBiome(0, 2, 0, 0, Biomes::Forest);
    container.setBiome(0, 3, 0, 0, Biomes::Taiga);

    EXPECT_EQ(container.getBiomeAtBlock(0, MIN_BUILD_HEIGHT, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(1, MIN_BUILD_HEIGHT, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(2, MIN_BUILD_HEIGHT, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(3, MIN_BUILD_HEIGHT, 0), Biomes::Plains);

    EXPECT_EQ(container.getBiomeAtBlock(4, MIN_BUILD_HEIGHT, 0), Biomes::Desert);
    EXPECT_EQ(container.getBiomeAtBlock(5, MIN_BUILD_HEIGHT, 0), Biomes::Desert);
    EXPECT_EQ(container.getBiomeAtBlock(6, MIN_BUILD_HEIGHT, 0), Biomes::Desert);
    EXPECT_EQ(container.getBiomeAtBlock(7, MIN_BUILD_HEIGHT, 0), Biomes::Desert);

    EXPECT_EQ(container.getBiomeAtBlock(8, MIN_BUILD_HEIGHT, 0), Biomes::Forest);
    EXPECT_EQ(container.getBiomeAtBlock(12, MIN_BUILD_HEIGHT, 0), Biomes::Taiga);
}

TEST_F(BiomeContainerTest, GetBiomeAtBlock_YIntermediateCoordinates)
{
    // Y values within the same quart should return the same biome
    container.setBiome(0, 0, 0, 0, Biomes::Ocean);
    container.setBiome(0, 0, 1, 0, Biomes::Plains);
    container.setBiome(0, 0, 2, 0, Biomes::Desert);
    container.setBiome(0, 0, 3, 0, Biomes::Forest);

    // Y = -64,-63,-62,-61 -> biomeY = 0
    EXPECT_EQ(container.getBiomeAtBlock(0, -64, 0), Biomes::Ocean);
    EXPECT_EQ(container.getBiomeAtBlock(0, -63, 0), Biomes::Ocean);
    EXPECT_EQ(container.getBiomeAtBlock(0, -62, 0), Biomes::Ocean);
    EXPECT_EQ(container.getBiomeAtBlock(0, -61, 0), Biomes::Ocean);

    // Y = -60,-59,-58,-57 -> biomeY = 1
    EXPECT_EQ(container.getBiomeAtBlock(0, -60, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(0, -59, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(0, -58, 0), Biomes::Plains);
    EXPECT_EQ(container.getBiomeAtBlock(0, -57, 0), Biomes::Plains);

    // Y = -56,-55,-54,-53 -> biomeY = 2
    EXPECT_EQ(container.getBiomeAtBlock(0, -56, 0), Biomes::Desert);

    // Y = -52,-51,-50,-49 -> biomeY = 3
    EXPECT_EQ(container.getBiomeAtBlock(0, -52, 0), Biomes::Forest);
    EXPECT_EQ(container.getBiomeAtBlock(0, -49, 0), Biomes::Forest);
}

// ============================================================================
// 12. Constant verification
// ============================================================================

TEST_F(BiomeContainerTest, Constants_HaveExpectedValues)
{
    EXPECT_EQ(BiomeContainer::HORIZ_SIZE, 4);
    EXPECT_EQ(BiomeContainer::VERT_SIZE, 4);
    EXPECT_EQ(BiomeContainer::SECTION_BIOME_SIZE, 64);
    EXPECT_EQ(BiomeContainer::SECTION_COUNT, CHUNK_SECTIONS);
    EXPECT_EQ(BiomeContainer::TOTAL_SIZE, 64 * CHUNK_SECTIONS);

    // Verify CHUNK_SECTIONS = 384 / 16 = 24
    EXPECT_EQ(CHUNK_SECTIONS, 24);
    EXPECT_EQ(BiomeContainer::TOTAL_SIZE, 1536);
}

// ============================================================================
// 13. Section isolation: writing one section does not affect another
// ============================================================================

TEST_F(BiomeContainerTest, SectionIsolation)
{
    // Fill section 0 with Plains and section 1 with Desert
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                container.setBiome(0, x, y, z, Biomes::Plains);
                container.setBiome(1, x, y, z, Biomes::Desert);
            }
        }
    }

    // Overwrite section 0 entirely with Forest
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                container.setBiome(0, x, y, z, Biomes::Forest);
            }
        }
    }

    // Section 1 should still be Desert
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                EXPECT_EQ(container.getBiome(1, x, y, z), Biomes::Desert)
                    << "Section 1 contaminated at x=" << x << " y=" << y << " z=" << z;
            }
        }
    }
}

// ============================================================================
// 14. Index layout verification: verify the internal index calculation
// ============================================================================

TEST_F(BiomeContainerTest, IndexLayout_YMajorOrder)
{
    // The index is: sectionIndex * 64 + y * 16 + z * 4 + x
    // Write a unique biome at each (x,y,z) in section 0, verify no collisions
    std::set<BiomeId> writtenIds;
    BiomeId id = 1;
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                container.setBiome(0, x, y, z, id);
                writtenIds.insert(id);
                ++id;
            }
        }
    }
    // 64 unique IDs written
    EXPECT_EQ(writtenIds.size(), 64u);

    // Read back and verify each position still has its unique ID
    id = 1;
    for (i32 y = 0; y < V; ++y) {
        for (i32 z = 0; z < H; ++z) {
            for (i32 x = 0; x < H; ++x) {
                EXPECT_EQ(container.getBiome(0, x, y, z), id)
                    << "Index layout mismatch at x=" << x << " y=" << y << " z=" << z;
                ++id;
            }
        }
    }
}

TEST_F(BiomeContainerTest, IndexLayout_SectionsDoNotOverlap)
{
    // Write a different biome at the same (x,y,z) in each section
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        container.setBiome(sec, 0, 0, 0, static_cast<BiomeId>(sec + 100));
    }

    // Verify each section still has its own value
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        EXPECT_EQ(container.getBiome(sec, 0, 0, 0), static_cast<BiomeId>(sec + 100))
            << "Section overlap at section " << sec;
    }
}

// ============================================================================
// 15. Edge case: Y coordinates at section boundaries
// ============================================================================

TEST_F(BiomeContainerTest, YSectionBoundaries)
{
    // Test every section boundary Y coordinate
    for (i32 sec = 0; sec < SEC_COUNT; ++sec) {
        i32 yBottom = MIN_BUILD_HEIGHT + sec * CHUNK_SECTION_HEIGHT;
        // The bottom of each section should map to biomeY=0 in that section
        container.setBiome(sec, 0, 0, 0, static_cast<BiomeId>(sec + 1));
        EXPECT_EQ(container.getBiomeAtBlock(0, yBottom, 0), static_cast<BiomeId>(sec + 1))
            << "Y=" << yBottom << " should be in section " << sec;
    }
}

} // namespace
} // namespace mc
