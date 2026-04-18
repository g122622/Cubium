#include <gtest/gtest.h>

#include "common/util/math/random/IRandom.hpp"
#include "world/IWorld.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/ice/IceBlock.hpp"

#include <map>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

class IceTestWorld final : public IBlockReader {
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
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isUltraWarm() const override { return m_isUltraWarm; }

    void setUltraWarm(bool value) { m_isUltraWarm = value; }

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
    bool m_isUltraWarm = false;
};

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

class IceBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(IceBlockTest, RandomTickTurnsIceIntoWaterInNormalDimension) {
    IceTestWorld world;
    IceBlock ice(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(4, 64, 4);
    BlockState state = ice.defaultState();

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    ice.randomTick(world, pos, state, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTickTurnsIceIntoAirInUltraWarmDimension) {
    IceTestWorld world;
    IceBlock ice(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(6, 64, 6);
    BlockState state = ice.defaultState();

    world.setUltraWarm(true);
    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    ice.randomTick(world, pos, state, random);

    EXPECT_EQ(world.getBlockState(pos), nullptr);
}

TEST_F(IceBlockTest, RandomTickTurnsFrostedIceIntoWaterInNormalDimension) {
    IceTestWorld world;
    FrostedIceBlock frostedIce(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(8, 64, 8);
    BlockState state = frostedIce.defaultState();

    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    frostedIce.randomTick(world, pos, state, random);

    const BlockState* finalState = world.getBlockState(pos);
    ASSERT_NE(finalState, nullptr);
    EXPECT_EQ(finalState->stateId(), VanillaBlocks::WATER->defaultState().stateId());
}

TEST_F(IceBlockTest, RandomTickTurnsFrostedIceIntoAirInUltraWarmDimension) {
    IceTestWorld world;
    FrostedIceBlock frostedIce(BlockProperties(Material::ICE).hardness(0.5f));
    SequenceRandom random({0});
    const BlockPos pos(10, 64, 10);
    BlockState state = frostedIce.defaultState();

    world.setUltraWarm(true);
    world.setBlockAt(pos, &state);
    world.setSkyLightAt(pos, 15);
    world.setBlockLightAt(pos, 15);

    frostedIce.randomTick(world, pos, state, random);

    EXPECT_EQ(world.getBlockState(pos), nullptr);
}

} // namespace