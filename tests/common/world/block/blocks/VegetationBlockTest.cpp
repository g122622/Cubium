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
#include "world/block/blocks/vegetation/BambooBlock.hpp"
#include "core/Constants.hpp"

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
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
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

// ============================================================================
// BambooBlock Tests
// ============================================================================

TEST_F(VegetationBlockTest, BambooCanSustainOnBambooPlantableBlocks) {
    BambooBlock bamboo(BlockProperties(Material::BAMBOO).hardness(1.0f).notSolid());

    VegetationTestWorld world;
    const BlockPos pos(10, 5, 10);

    // 可以放在草方块上
    world.setBlockAt(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_TRUE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));

    // 可以放在泥土上
    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    EXPECT_TRUE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));

    // 可以放在沙子上
    world.setBlockAt(pos.down(), &VanillaBlocks::SAND->defaultState());
    EXPECT_TRUE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));

    // 可以放在沙砾上
    world.setBlockAt(pos.down(), &VanillaBlocks::GRAVEL->defaultState());
    EXPECT_TRUE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));

    // 可以放在竹子上
    world.setBlockAt(pos.down(), &VanillaBlocks::BAMBOO->defaultState());
    EXPECT_TRUE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));

    // 不可以放在石头上
    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(bamboo.isValidPosition(bamboo.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, BambooGrowthLimitedTo16Blocks) {
    // 使用 VanillaBlocks 中注册的竹子
    ASSERT_NE(VanillaBlocks::BAMBOO, nullptr);
    BambooBlock* bamboo = dynamic_cast<BambooBlock*>(VanillaBlocks::BAMBOO);
    ASSERT_NE(bamboo, nullptr);

    VegetationTestWorld world;
    const BlockPos basePos(5, 20, 5);

    // 设置基座
    world.setBlockAt(basePos.down(), &VanillaBlocks::DIRT->defaultState());

    // 设置连续16格竹子（从 basePos 向下15格 + basePos 本身 = 16格）
    for (i32 i = 0; i < 16; ++i) {
        world.setBlockAt(basePos.down(i), &VanillaBlocks::BAMBOO->defaultState());
    }

    // 检查竹子已放置
    const BlockState* checkState = world.getBlockState(basePos);
    ASSERT_NE(checkState, nullptr);
    EXPECT_TRUE(checkState->is(VanillaBlocks::BAMBOO));

    // 竹子高度已达16格，不应该再生长
    SequenceRandom random({0});  // nextInt(3) == 0 触发生长
    BlockState state = checkState->with(BlockStateProperties::STAGE_0_1(), 0);
    bamboo->randomTick(world, basePos, state, random);

    // 上方应该没有新竹子（因为已达最大高度）
    const BlockState* above = world.getBlockState(basePos.up());
    EXPECT_TRUE(above == nullptr || above->isAir());
}

TEST_F(VegetationBlockTest, BambooRandomTickCanGrow) {
    // 使用 VanillaBlocks 中注册的竹子
    ASSERT_NE(VanillaBlocks::BAMBOO, nullptr);
    BambooBlock* bamboo = dynamic_cast<BambooBlock*>(VanillaBlocks::BAMBOO);
    ASSERT_NE(bamboo, nullptr);

    VegetationTestWorld world;
    const BlockPos pos(7, 20, 7);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    const BlockState& bambooState = VanillaBlocks::BAMBOO->defaultState();
    world.setBlockAt(pos, &bambooState);

    // 验证方块已设置
    const BlockState* checkState = world.getBlockState(pos);
    ASSERT_NE(checkState, nullptr);

    // 使用随机数触发生长 (nextInt(3) == 0, nextInt(3) for leaves)
    SequenceRandom random({0, 1});  // 0 -> 触发生长, 1 -> 小叶子
    BlockState state = bambooState;
    bamboo->randomTick(world, pos, state, random);

    // 上方应该有新竹子
    const BlockState* above = world.getBlockState(pos.up());
    ASSERT_NE(above, nullptr) << "No block was placed above the bamboo";
    EXPECT_TRUE(above->is(VanillaBlocks::BAMBOO)) << "Block above is not bamboo";
}

TEST_F(VegetationBlockTest, BambooBoneMealGrowsMultipleBlocks) {
    BambooBlock bamboo(BlockProperties(Material::BAMBOO).hardness(1.0f).notSolid());

    VegetationTestWorld world;
    const BlockPos pos(8, 30, 8);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    const BlockState& bambooState = VanillaBlocks::BAMBOO->defaultState();
    world.setBlockAt(pos, &bambooState);

    // 使用随机数让 canUseBonemeal 返回 true 并生长
    SequenceRandom random({0, 1, 0, 0});  // float < 0.45, nextInt(2) == 1
    EXPECT_TRUE(bamboo.canUseBonemeal(world, random, pos, bambooState));

    bamboo.grow(world, random, pos, bambooState);

    // 检查是否生长了多格
    int growthCount = 0;
    for (int i = 1; i <= 3; ++i) {
        const BlockState* above = world.getBlockState(pos.up(i));
        if (above != nullptr && above->is(VanillaBlocks::BAMBOO)) {
            growthCount++;
        }
    }
    EXPECT_GE(growthCount, 1);
}

// ============================================================================
// BambooSaplingBlock Tests
// ============================================================================

TEST_F(VegetationBlockTest, BambooSaplingCanSustainOnBambooPlantableBlocks) {
    BambooSaplingBlock sapling(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(12, 5, 12);

    // 可以放在草方块上
    world.setBlockAt(pos.down(), &VanillaBlocks::GRASS_BLOCK->defaultState());
    EXPECT_TRUE(sapling.isValidPosition(sapling.defaultState(), world, pos));

    // 可以放在泥土上
    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    EXPECT_TRUE(sapling.isValidPosition(sapling.defaultState(), world, pos));

    // 不可以放在竹子上
    world.setBlockAt(pos.down(), &VanillaBlocks::BAMBOO->defaultState());
    EXPECT_FALSE(sapling.isValidPosition(sapling.defaultState(), world, pos));

    // 不可以放在石头上
    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    EXPECT_FALSE(sapling.isValidPosition(sapling.defaultState(), world, pos));
}

TEST_F(VegetationBlockTest, BambooSaplingRandomTickCanGrow) {
    BambooSaplingBlock sapling(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(15, 40, 15);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    const BlockState& saplingState = VanillaBlocks::BAMBOO_SAPLING->defaultState();
    world.setBlockAt(pos, &saplingState);

    // 使用随机数触发生长 (nextInt(8) == 0)
    SequenceRandom random({0});
    BlockState state = saplingState;
    sapling.randomTick(world, pos, state, random);

    // 幼苗应该变成竹子
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_TRUE(newState->is(VanillaBlocks::BAMBOO));
}

TEST_F(VegetationBlockTest, BambooSaplingGrowMethodReplacesWithBamboo) {
    BambooSaplingBlock sapling(BlockProperties(Material::REPLACEABLE_PLANT).noCollision().notSolid());

    VegetationTestWorld world;
    const BlockPos pos(20, 50, 20);

    world.setBlockAt(pos.down(), &VanillaBlocks::DIRT->defaultState());
    const BlockState& saplingState = VanillaBlocks::BAMBOO_SAPLING->defaultState();
    world.setBlockAt(pos, &saplingState);

    SequenceRandom random({0});
    sapling.grow(world, random, pos, saplingState);

    // 幼苗应该变成竹子
    const BlockState* newState = world.getBlockState(pos);
    ASSERT_NE(newState, nullptr);
    EXPECT_TRUE(newState->is(VanillaBlocks::BAMBOO));
}