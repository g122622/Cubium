#include <gtest/gtest.h>

#include "common/entity/entities/player/SpawnLocationHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/TestWorldHelper.hpp"

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
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
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
        for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
            for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
                for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
                    chunk.getBiomes().setBiome(x, y, z, biomeId);
                }
            }
        }
    }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("SpawnLocationTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("SpawnLocationTestWorld::tickManager not implemented");
    }

private:
    [[nodiscard]] const BlockState* getAirState() const
    {
        return &VanillaBlocks::AIR->defaultState();
    }
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
    BiomeRegistry::instance().registerBiome(customBiome);

    ChunkData& chunk = world.ensureChunk(0, 0);
    world.fillBiome(chunk, CUSTOM_BIOME_ID);

    world.setBlockState(3, 63, 3, &VanillaBlocks::DIRT->defaultState());
    world.setBlockState(3, 64, 3, &VanillaBlocks::GRASS_BLOCK->defaultState());

    const auto spawnPos = SpawnLocationHelper::findSpawnLocation(world, 3, 3, false);

    EXPECT_FALSE(spawnPos.has_value());
}

} // namespace
} // namespace mc
