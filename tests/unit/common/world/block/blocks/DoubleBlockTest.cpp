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
 * IMPLIED, NONINFRINGEMENT OF THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/cave/SmallDripleafBlock.hpp"
#include "common/world/block/blocks/ocean/TallSeagrassBlock.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/blocks/vegetation/FlowerBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "core/Constants.hpp"

#include <map>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 双格方块 updatePostPlacement 测试用世界桩
 */
class DoubleBlockTestWorld final : public IBlockReader {
public:
    DoubleBlockTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        if (it != m_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        (void)flags;
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<DoubleBlockTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    world::border::WorldBorder m_worldBorder;
    math::Random m_random{12345};
};

/**
 * @brief 双格方块测试夹具
 */
class DoubleBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    DoubleBlockTestWorld world;
};

// ============================================================================
// DoublePlantBlock updatePostPlacement 测试
// ============================================================================

TEST_F(DoubleBlockTest, DoublePlant_LowerHalfLostUpperHalf_ReturnsAir)
{
    // 当下半部分失去上半部分时，下半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    // 放置向日葵（DoublePlantBlock 子类）的下半和上半部分
    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);

    // 下方有支撑（泥土）
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::DIRT->defaultState());

    // 上方变为空气（模拟上半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result =
        VanillaBlocks::SUNFLOWER->updatePostPlacement(*lowerState, Direction::Up, airState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.isAir()) << "Lower half should become air when upper half is removed";
}

TEST_F(DoubleBlockTest, DoublePlant_UpperHalfLostLowerHalf_ReturnsAir)
{
    // 当上半部分失去下半部分时，上半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::DIRT->defaultState());

    // 下方变为空气（模拟下半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SUNFLOWER->updatePostPlacement(
        *upperState, Direction::Down, airState, world, upperPos, lowerPos);

    EXPECT_TRUE(result.isAir()) << "Upper half should become air when lower half is removed";
}

TEST_F(DoubleBlockTest, DoublePlant_BothHalvesIntact_SameState)
{
    // 当上下两半都完好时，updatePostPlacement 应返回原状态
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::DIRT->defaultState());

    // 下半部分收到上方邻居（上半部分）的更新
    BlockState result = VanillaBlocks::SUNFLOWER->updatePostPlacement(
        *lowerState, Direction::Up, *upperState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.is(VanillaBlocks::SUNFLOWER)) << "Lower half should remain when upper half exists";
    EXPECT_EQ(result.get(BlockStateProperties::DOUBLE_BLOCK_HALF()), BlockStateProperties::DoubleBlockHalf::Lower);
}

TEST_F(DoubleBlockTest, DoublePlant_HorizontalUpdate_NoEffect)
{
    // 水平方向的邻居变化不应触发断裂
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::STONE->defaultState());

    // 北侧邻居变化
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SUNFLOWER->updatePostPlacement(
        *lowerState, Direction::North, airState, world, lowerPos, lowerPos.north());

    EXPECT_TRUE(result.is(VanillaBlocks::SUNFLOWER)) << "Horizontal update should not break lower half";
}

TEST_F(DoubleBlockTest, DoublePlant_LowerHalfLostGround_ReturnsAir)
{
    // 当下半部分失去地面支撑时，下半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);

    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    world.setBlockAt(lowerPos, lowerState);
    // 不设置地面支撑（下方为空）

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SUNFLOWER->updatePostPlacement(
        *lowerState, Direction::Down, airState, world, lowerPos, lowerPos.down());

    EXPECT_TRUE(result.isAir()) << "Lower half should become air when ground support is lost";
}

TEST_F(DoubleBlockTest, DoublePlant_LowerHalfWithGround_DownUpdate_NoEffect)
{
    // 当下半部分有地面支撑时，下方邻居变化不应影响
    const BlockPos lowerPos(5, 10, 5);

    const BlockState* lowerState = &VanillaBlocks::SUNFLOWER->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    world.setBlockAt(lowerPos, lowerState);
    // 使用泥土（DIRT）作为支撑，因为向日葵需要泥土类方块
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::DIRT->defaultState());

    // 下方仍然是泥土
    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    BlockState result = VanillaBlocks::SUNFLOWER->updatePostPlacement(
        *lowerState, Direction::Down, *dirtState, world, lowerPos, lowerPos.down());

    EXPECT_TRUE(result.is(VanillaBlocks::SUNFLOWER)) << "Lower half should remain when ground still supports";
}

// ============================================================================
// TallSeagrassBlock updatePostPlacement 测试
// ============================================================================

TEST_F(DoubleBlockTest, TallSeagrass_LowerHalfLostUpperHalf_ReturnsAir)
{
    // 当下半部分失去上半部分时，下半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::STONE->defaultState());

    // 上方变为空气（模拟上半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::TALL_SEAGRASS->updatePostPlacement(
        *lowerState, Direction::Up, airState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.isAir()) << "TallSeagrass lower half should become air when upper half is removed";
}

TEST_F(DoubleBlockTest, TallSeagrass_UpperHalfLostLowerHalf_ReturnsAir)
{
    // 当上半部分失去下半部分时，上半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::STONE->defaultState());

    // 下方变为空气（模拟下半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::TALL_SEAGRASS->updatePostPlacement(
        *upperState, Direction::Down, airState, world, upperPos, lowerPos);

    EXPECT_TRUE(result.isAir()) << "TallSeagrass upper half should become air when lower half is removed";
}

TEST_F(DoubleBlockTest, TallSeagrass_BothHalvesIntact_SameState)
{
    // 当上下两半都完好时，updatePostPlacement 应返回原状态
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
    const BlockState* upperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::STONE->defaultState());

    // 下半部分收到上方邻居（上半部分）的更新
    BlockState result = VanillaBlocks::TALL_SEAGRASS->updatePostPlacement(
        *lowerState, Direction::Up, *upperState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.is(VanillaBlocks::TALL_SEAGRASS))
        << "TallSeagrass lower half should remain when upper half exists";
}

TEST_F(DoubleBlockTest, TallSeagrass_HorizontalUpdate_NoEffect)
{
    // 水平方向的邻居变化不应触发断裂
    const BlockPos lowerPos(5, 10, 5);

    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(lowerPos.down(), &VanillaBlocks::STONE->defaultState());

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::TALL_SEAGRASS->updatePostPlacement(
        *lowerState, Direction::East, airState, world, lowerPos, lowerPos.east());

    EXPECT_TRUE(result.is(VanillaBlocks::TALL_SEAGRASS)) << "Horizontal update should not break TallSeagrass";
}

TEST_F(DoubleBlockTest, TallSeagrass_LowerHalfLostGround_ReturnsAir)
{
    // 当下半部分失去地面支撑时，下半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);

    const BlockState* lowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
        BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);

    world.setBlockAt(lowerPos, lowerState);
    // 不设置地面支撑

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::TALL_SEAGRASS->updatePostPlacement(
        *lowerState, Direction::Down, airState, world, lowerPos, lowerPos.down());

    EXPECT_TRUE(result.isAir()) << "TallSeagrass lower half should become air when ground support is lost";
}

// ============================================================================
// SmallDripleafBlock updatePostPlacement 测试
// ============================================================================

TEST_F(DoubleBlockTest, SmallDripleaf_LowerHalfLostUpperHalf_ReturnsAir)
{
    // 当下半部分失去上半部分时，下半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState* upperState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);

    // 上方变为空气（模拟上半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SMALL_DRIPLEAF->updatePostPlacement(
        *lowerState, Direction::Up, airState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.isAir()) << "SmallDripleaf lower half should become air when upper half is removed";
}

TEST_F(DoubleBlockTest, SmallDripleaf_UpperHalfLostLowerHalf_ReturnsAir)
{
    // 当上半部分失去下半部分时，上半部分应变为空气
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState* upperState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);

    // 下方变为空气（模拟下半部分被破坏）
    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SMALL_DRIPLEAF->updatePostPlacement(
        *upperState, Direction::Down, airState, world, upperPos, lowerPos);

    EXPECT_TRUE(result.isAir()) << "SmallDripleaf upper half should become air when lower half is removed";
}

TEST_F(DoubleBlockTest, SmallDripleaf_BothHalvesIntact_SameState)
{
    // 当上下两半都完好时，updatePostPlacement 应返回原状态
    const BlockPos lowerPos(5, 10, 5);
    const BlockPos upperPos(5, 11, 5);

    const BlockState* lowerState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
    const BlockState* upperState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    world.setBlockAt(lowerPos, lowerState);
    world.setBlockAt(upperPos, upperState);

    // 下半部分收到上方邻居（上半部分）的更新
    BlockState result = VanillaBlocks::SMALL_DRIPLEAF->updatePostPlacement(
        *lowerState, Direction::Up, *upperState, world, lowerPos, upperPos);

    EXPECT_TRUE(result.is(VanillaBlocks::SMALL_DRIPLEAF))
        << "SmallDripleaf lower half should remain when upper half exists";
}

TEST_F(DoubleBlockTest, SmallDripleaf_HorizontalUpdate_NoEffect)
{
    // 水平方向的邻居变化不应触发断裂
    const BlockPos lowerPos(5, 10, 5);

    const BlockState* lowerState =
        &VanillaBlocks::SMALL_DRIPLEAF->defaultState()
             .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower)
             .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);

    world.setBlockAt(lowerPos, lowerState);

    const BlockState& airState = VanillaBlocks::AIR->defaultState();
    BlockState result = VanillaBlocks::SMALL_DRIPLEAF->updatePostPlacement(
        *lowerState, Direction::South, airState, world, lowerPos, lowerPos.south());

    EXPECT_TRUE(result.is(VanillaBlocks::SMALL_DRIPLEAF)) << "Horizontal update should not break SmallDripleaf";
}

} // namespace
