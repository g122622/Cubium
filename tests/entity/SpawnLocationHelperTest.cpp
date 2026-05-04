#include <gtest/gtest.h>

#include "common/entity/entities/player/SpawnLocationHelper.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"

#include <unordered_map>

namespace mc {
namespace {

class SpawnLocationTestWorld : public IWorld {
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

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override
    {
        const auto it = m_chunks.find(ChunkPos(x, z));
        return it != m_chunks.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override
    {
        return m_chunks.find(ChunkPos(x, z)) != m_chunks.end();
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

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    ChunkData& ensureChunk(ChunkCoord x, ChunkCoord z)
    {
        const ChunkPos chunkPos(x, z);
        auto it = m_chunks.find(chunkPos);
        if (it == m_chunks.end()) {
            it = m_chunks.emplace(chunkPos, std::make_unique<ChunkData>(x, z)).first;
        }
        return *it->second;
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

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("SpawnLocationTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("SpawnLocationTestWorld::getRandom not implemented");
    }

private:
    [[nodiscard]] const BlockState* getAirState() const
    {
        return &VanillaBlocks::AIR->defaultState();
    }

    std::unordered_map<ChunkPos, std::unique_ptr<ChunkData>> m_chunks;
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
