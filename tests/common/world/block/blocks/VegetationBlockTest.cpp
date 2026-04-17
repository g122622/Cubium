#include <gtest/gtest.h>

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/vegetation/CactusBlock.hpp"
#include "world/block/blocks/vegetation/MushroomBlock.hpp"
#include "world/block/blocks/vegetation/SaplingBlock.hpp"
#include "world/block/blocks/vegetation/TallGrassBlock.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 植被方块测试用世界
 */
class VegetationTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }

        return nullptr;
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override {
        return sampleLight(m_blockLight, x, y, z);
    }

    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override {
        return sampleLight(m_skyLight, x, y, z);
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= 0 && y < 256; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return m_seed; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    void setSeed(u64 seed) { m_seed = seed; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) {
        (void)setBlock(pos.x, pos.y, pos.z, state);
    }

    void setSkyLightAt(const BlockPos& pos, u8 light) {
        m_skyLight[pos] = light;
    }

    void setBlockLightAt(const BlockPos& pos, u8 light) {
        m_blockLight[pos] = light;
    }

private:
    [[nodiscard]] const BlockState* airState() const {
        return BlockRegistry::instance().airState();
    }

    [[nodiscard]] static u8 sampleLight(const std::map<BlockPos, u8>& lights, i32 x, i32 y, i32 z) {
        const BlockPos pos(x, y, z);
        const auto it = lights.find(pos);
        if (it != lights.end()) {
            return it->second;
        }
        return 0;
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, u8> m_blockLight;
    std::map<BlockPos, u8> m_skyLight;
    u64 m_seed = 12345;
};

/**
 * @brief 测试用固定随机序列
 */
class SequenceRandom final : public math::IRandom {
public:
    explicit SequenceRandom(std::vector<i32> values)
        : m_values(std::move(values)) {
    }

    void setSeed(u64 seed) override {
        m_seed = seed;
        m_index = 0;
    }

    [[nodiscard]] u64 nextU64() override {
        return static_cast<u64>(nextValue());
    }

    [[nodiscard]] u32 nextU32() override {
        return static_cast<u32>(nextValue());
    }

    [[nodiscard]] i32 nextInt(i32 bound) override {
        return nextValue() % bound;
    }

    [[nodiscard]] i32 nextInt() override {
        return nextValue();
    }

    [[nodiscard]] i32 nextInt(i32 min, i32 max) override {
        return min + (nextValue() % (max - min + 1));
    }

    [[nodiscard]] bool nextBoolean() override {
        return (nextValue() & 1) != 0;
    }

    [[nodiscard]] f32 nextFloat() override {
        return static_cast<f32>(nextValue() & 0x00FFFFFF) / static_cast<f32>(1 << 24);
    }

    [[nodiscard]] f32 nextFloat(f32 min, f32 max) override {
        return min + nextFloat() * (max - min);
    }

    [[nodiscard]] f64 nextDouble() override {
        return static_cast<f64>(nextValue() & 0x001FFFFFFFFFFFFF) / static_cast<f64>(1ULL << 53);
    }

    [[nodiscard]] f64 nextDouble(f64 min, f64 max) override {
        return min + nextDouble() * (max - min);
    }

    [[nodiscard]] f32 nextGaussian(f32 mean, f32 stddev) override {
        return mean + stddev * static_cast<f32>(nextValue());
    }

    [[nodiscard]] i64 nextLong() override {
        return static_cast<i64>(nextValue());
    }

    [[nodiscard]] i64 nextLong(i64 bound) override {
        return static_cast<i64>(nextValue() % bound);
    }

private:
    [[nodiscard]] i32 nextValue() {
        if (m_index < m_values.size()) {
            return m_values[m_index++];
        }
        return 0;
    }

    std::vector<i32> m_values;
    size_t m_index = 0;
    u64 m_seed = 0;
};

/**
 * @brief 测试用生物实体
 */
class TestLivingEntity final : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(LegacyEntityType::Player, 1) {
        setHealth(maxHealth());
    }
};

} // namespace

class VegetationBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(VegetationBlockTest, SaplingCanSustainOnDirtLikeBlocks) {
    SaplingBlock sapling(
        [](IWorld&, const BlockPos&, math::IRandom&) {},
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(8, 1, 8);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    EXPECT_TRUE(sapling.isValidPosition(sapling.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::FARMLAND->defaultState());
    EXPECT_TRUE(sapling.isValidPosition(sapling.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(sapling.isValidPosition(sapling.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, SaplingRandomTickAdvancesStageUnderLight) {
    bool treeCalled = false;
    SaplingBlock sapling(
        [&](IWorld&, const BlockPos&, math::IRandom&) {
            treeCalled = true;
        },
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(4, 64, 4);
    const BlockState& saplingState = sapling.defaultState();

    world.setBlockAt(pos, &saplingState);
    world.setSkyLightAt(pos.up(), 15);
    world.setBlockLightAt(pos.up(), 0);

    SequenceRandom random({0});
    BlockState state = saplingState;
    sapling.randomTick(world, pos, state, random);

    const BlockState* updated = world.getBlockState(pos);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ(sapling.getStage(*updated), 1);
    EXPECT_FALSE(treeCalled);
}

TEST_F(VegetationBlockTest, SaplingGrowUsesWorldSeedAndPosition) {
    std::vector<u64> samples;
    SaplingBlock sapling(
        [&](IWorld&, const BlockPos&, math::IRandom& random) {
            samples.push_back(random.nextU64());
        },
        BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld worldA;
    worldA.setSeed(12345);
    const BlockPos pos(12, 70, -4);
    const BlockState& matureState = sapling.defaultState().with(BlockStateProperties::STAGE_0_1(), 1);

    worldA.setBlockAt(pos, &matureState);
    BlockState stateA = matureState;
    EXPECT_TRUE(sapling.grow(worldA, pos, stateA));

    VegetationTestWorld worldB;
    worldB.setSeed(12345);
    worldB.setBlockAt(pos, &matureState);
    BlockState stateB = matureState;
    EXPECT_TRUE(sapling.grow(worldB, pos, stateB));

    ASSERT_EQ(samples.size(), 2u);
    EXPECT_EQ(samples[0], samples[1]);
}

TEST_F(VegetationBlockTest, TallGrassCanSustainOnDirtLikeBlocks) {
    TallGrassBlock grass(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(2, 10, 2);

    world.setBlockAt(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_TRUE(grass.isValidPosition(grass.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::FARMLAND->defaultState());
    EXPECT_TRUE(grass.isValidPosition(grass.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(grass.isValidPosition(grass.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, MushroomCanSustainInDarkAndOnMycelium) {
    MushroomBlock mushroom(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().lightLevel(1));

    VegetationTestWorld world;
    const BlockPos pos(6, 30, 6);

    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    world.setSkyLightAt(pos, 15);
    EXPECT_FALSE(mushroom.isValidPosition(mushroom.defaultState(), world, pos));

    world.setSkyLightAt(pos, 0);
    EXPECT_TRUE(mushroom.isValidPosition(mushroom.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::MYCELIUM->defaultState());
    world.setSkyLightAt(pos, 15);
    EXPECT_TRUE(mushroom.isValidPosition(mushroom.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, MushroomRandomTickSpreadsWhenDark) {
    MushroomBlock mushroom(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid().lightLevel(1));

    VegetationTestWorld world;
    const BlockPos pos(0, 1, 0);

    world.setBlockAt(pos, &mushroom.defaultState());
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            world.setBlockAt(BlockPos(pos.x + dx, 0, pos.z + dz), &VanillaBlocks::STONE->defaultState());
        }
    }

    SequenceRandom random({0, 2, 1, 1});
    BlockState state = mushroom.defaultState();
    mushroom.randomTick(world, pos, state, random);

    bool spreadFound = false;
    for (i32 dx = -1; dx <= 1 && !spreadFound; ++dx) {
        for (i32 dy = -1; dy <= 1 && !spreadFound; ++dy) {
            for (i32 dz = -1; dz <= 1 && !spreadFound; ++dz) {
                const BlockPos candidate(pos.x + dx, pos.y + dy, pos.z + dz);
                if (candidate == pos) {
                    continue;
                }

                const BlockState* candidateState = world.getBlockState(candidate);
                if (candidateState != nullptr && candidateState->is(&mushroom)) {
                    spreadFound = true;
                }
            }
        }
    }

    EXPECT_TRUE(spreadFound);
}

TEST_F(VegetationBlockTest, CactusCanSustainOnlyOnSandLikeBlocks) {
    CactusBlock cactus(BlockProperties(Material::PLANT).hardness(0.4f));

    VegetationTestWorld world;
    const BlockPos pos(9, 20, 9);

    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    EXPECT_TRUE(cactus.isValidPosition(cactus.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::RED_SAND->defaultState());
    EXPECT_TRUE(cactus.isValidPosition(cactus.defaultState(), world, pos));

    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(cactus.isValidPosition(cactus.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, CactusOnEntityCollisionDamagesLivingEntities) {
    CactusBlock cactus(BlockProperties(Material::PLANT).hardness(0.4f));
    VegetationTestWorld world;
    const BlockPos pos(3, 50, 3);
    TestLivingEntity entity;

    entity.setHealth(10.0f);

    cactus.onEntityCollision(cactus.defaultState(), world, pos, entity);

    EXPECT_FLOAT_EQ(entity.health(), 9.0f);
}