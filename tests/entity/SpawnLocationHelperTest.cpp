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

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/SpawnLocationHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <unordered_map>

namespace mc {
namespace {

class SpawnLocationTestWorld : public test::BaseChunkBackedTestWorld {
public:
    SpawnLocationTestWorld() = default;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return getAirState();
        }

        const BlockState* state = chunk->getBlockState(x & 15, y, z & 15);
        return state != nullptr ? state : getAirState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        ChunkData& chunk = ensureChunk(x >> 4, z >> 4);
        chunk.setBlockState(x & 15, y, z & 15, state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override
    {
        const ChunkData* chunk = getChunk(x >> 4, z >> 4);
        if (chunk == nullptr) {
            return 0;
        }

        for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT; --y) {
            const BlockState* state = chunk->getBlockState(x & 15, y, z & 15);
            if (state != nullptr && !state->isAir()) {
                return y;
            }
        }

        return 0;
    }

    void fillBiome(ChunkData& chunk, BiomeId biomeId)
    {
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                    for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                        chunk.getBiomes().setBiome(section, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpawnLocationTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpawnLocationTestWorld::tickManager not implemented");
    }

private:
    [[nodiscard]] const BlockState* getAirState() const { return &VanillaBlocks::AIR->defaultState(); }
};

class SpawnLocationHelperTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(SpawnLocationHelperTest, FindsPlainsSurfaceSpawnLikeVanilla)
{
    SpawnLocationTestWorld world;
    ChunkData& chunk = world.ensureChunk(0, 0);
    world.fillBiome(chunk, Biomes::Plains);

    world.setBlockState(1, 63, 1, &VanillaBlocks::DIRT->defaultState());
    world.setBlockState(1, 64, 1, &VanillaBlocks::GRASS_BLOCK->defaultState());

    const auto spawnPos = SpawnLocationHelper::findSpawnLocation(world, 1, 1, true);

    ASSERT_TRUE(spawnPos.has_value());
    EXPECT_EQ(*spawnPos, BlockPos(1, 65, 1));
}

TEST_F(SpawnLocationHelperTest, RejectsOceanColumnsCoveredByWater)
{
    SpawnLocationTestWorld world;
    ChunkData& chunk = world.ensureChunk(0, 0);
    world.fillBiome(chunk, Biomes::Plains);

    world.setBlockState(2, 62, 2, &VanillaBlocks::DIRT->defaultState());
    world.setBlockState(2, 63, 2, &VanillaBlocks::GRASS_BLOCK->defaultState());
    world.setBlockState(2, 64, 2, &VanillaBlocks::WATER->defaultState());

    const auto spawnPos = SpawnLocationHelper::findSpawnLocation(world, 2, 2, true);

    EXPECT_FALSE(spawnPos.has_value());
}

TEST_F(SpawnLocationHelperTest, ScansChunkInVanillaOrder)
{
    SpawnLocationTestWorld world;
    ChunkData& chunk = world.ensureChunk(0, 0);
    world.fillBiome(chunk, Biomes::Plains);

    world.setBlockState(5, 70, 7, &VanillaBlocks::GRASS_BLOCK->defaultState());

    const auto spawnPos = SpawnLocationHelper::findSpawnLocationInChunk(world, ChunkPos(0, 0), true);

    ASSERT_TRUE(spawnPos.has_value());
    EXPECT_EQ(*spawnPos, BlockPos(5, 71, 7));
}

TEST_F(SpawnLocationHelperTest, HonorsBiomeSurfaceBlockMatch)
{
    constexpr BiomeId CUSTOM_BIOME_ID = 250;

    SpawnLocationTestWorld world;
    Biome customBiome(CUSTOM_BIOME_ID, "spawn_helper_test");
    customBiome.setSurfaceBlock(&VanillaBlocks::SAND->defaultState());
    BiomeRegistry::instance().registerBiome(std::move(customBiome));

    ChunkData& chunk = world.ensureChunk(0, 0);
    world.fillBiome(chunk, CUSTOM_BIOME_ID);

    world.setBlockState(3, 63, 3, &VanillaBlocks::DIRT->defaultState());
    world.setBlockState(3, 64, 3, &VanillaBlocks::GRASS_BLOCK->defaultState());

    const auto spawnPos = SpawnLocationHelper::findSpawnLocation(world, 3, 3, false);

    EXPECT_FALSE(spawnPos.has_value());
}

} // namespace
} // namespace mc
