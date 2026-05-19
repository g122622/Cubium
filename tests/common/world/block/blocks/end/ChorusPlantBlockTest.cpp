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

#include "core/Constants.hpp"
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/VanillaBlocks.hpp"
#include "world/block/blocks/end/ChorusFlowerBlock.hpp"
#include "world/block/blocks/end/ChorusPlantBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::blocks;

namespace {

// 测试用 World Mock，实现 IBlockReader 接口
// 参考 IceBlockTest.cpp 中的 IceTestWorld 实现
class ChorusPlantTestWorld final : public IBlockReader {
public:
    ChorusPlantTestWorld() = default;

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

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return true; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(1); } // 末地维度
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    // TickManager interface
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<ChorusPlantTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    // Random interface
    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        throw std::runtime_error("ChorusPlantTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        throw std::runtime_error("ChorusPlantTestWorld::worldBorder not implemented");
    }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    u64 m_currentTick = 0;
};

} // namespace

// ============================================================================
// ChorusPlantBlock 基础测试
// ============================================================================

class ChorusPlantBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(ChorusPlantBlockTest, Create_HasCorrectProperties)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();

    // 默认状态应该是无连接
    EXPECT_FALSE(state.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(state.get(BlockStateProperties::EAST()));
    EXPECT_FALSE(state.get(BlockStateProperties::WEST()));
    EXPECT_FALSE(state.get(BlockStateProperties::UP()));
    EXPECT_FALSE(state.get(BlockStateProperties::DOWN()));
}

TEST_F(ChorusPlantBlockTest, GetShape_ReturnsValidShape)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();
    const CollisionShape& shape = block.getShape(state);

    // 形状不应该为空（即使没有连接，也有中心柱）
    EXPECT_FALSE(shape.isEmpty());
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_NoConnections)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& state = block.defaultState();

    // 无连接时索引应为 0
    size_t index = ChorusPlantBlock::getShapeIndex(state);
    EXPECT_EQ(index, 0ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_AllConnections)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 设置所有连接为 true
    const BlockState& state = block.defaultState()
                                  .with(BlockStateProperties::DOWN(), true)
                                  .with(BlockStateProperties::UP(), true)
                                  .with(BlockStateProperties::NORTH(), true)
                                  .with(BlockStateProperties::SOUTH(), true)
                                  .with(BlockStateProperties::WEST(), true)
                                  .with(BlockStateProperties::EAST(), true);

    // 所有连接时索引应为 63 (所有位都设置)
    size_t index = ChorusPlantBlock::getShapeIndex(state);
    EXPECT_EQ(index, 63ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_SingleConnection)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 测试各个方向的单独连接
    // Down = bit 0 = 1
    const BlockState& downState = block.defaultState().with(BlockStateProperties::DOWN(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(downState), 1ULL);

    // Up = bit 1 = 2
    const BlockState& upState = block.defaultState().with(BlockStateProperties::UP(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(upState), 2ULL);

    // North = bit 2 = 4
    const BlockState& northState = block.defaultState().with(BlockStateProperties::NORTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(northState), 4ULL);

    // South = bit 3 = 8
    const BlockState& southState = block.defaultState().with(BlockStateProperties::SOUTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(southState), 8ULL);

    // West = bit 4 = 16
    const BlockState& westState = block.defaultState().with(BlockStateProperties::WEST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(westState), 16ULL);

    // East = bit 5 = 32
    const BlockState& eastState = block.defaultState().with(BlockStateProperties::EAST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(eastState), 32ULL);
}

TEST_F(ChorusPlantBlockTest, GetShapeIndex_MultipleConnections)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // Up + Down = bit 0 + bit 1 = 3
    const BlockState& verticalState =
        block.defaultState().with(BlockStateProperties::DOWN(), true).with(BlockStateProperties::UP(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(verticalState), 3ULL);

    // North + South = bit 2 + bit 3 = 12
    const BlockState& northSouthState =
        block.defaultState().with(BlockStateProperties::NORTH(), true).with(BlockStateProperties::SOUTH(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(northSouthState), 12ULL);

    // East + West = bit 4 + bit 5 = 48
    const BlockState& eastWestState =
        block.defaultState().with(BlockStateProperties::WEST(), true).with(BlockStateProperties::EAST(), true);
    EXPECT_EQ(ChorusPlantBlock::getShapeIndex(eastWestState), 48ULL);
}

TEST_F(ChorusPlantBlockTest, Shape_ChangesWithConnections)
{
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));

    // 无连接的形状
    const BlockState& noConnection = block.defaultState();
    const CollisionShape& shapeNoConnection = block.getShape(noConnection);

    // 所有连接的形状
    const BlockState& allConnections = block.defaultState()
                                           .with(BlockStateProperties::DOWN(), true)
                                           .with(BlockStateProperties::UP(), true)
                                           .with(BlockStateProperties::NORTH(), true)
                                           .with(BlockStateProperties::SOUTH(), true)
                                           .with(BlockStateProperties::WEST(), true)
                                           .with(BlockStateProperties::EAST(), true);
    const CollisionShape& shapeAllConnections = block.getShape(allConnections);

    // 两个形状的引用应该不同（因为索引不同）
    EXPECT_NE(&shapeNoConnection, &shapeAllConnections);
}

TEST_F(ChorusPlantBlockTest, VanillaBlocks_Registered)
{
    // 确保 VanillaBlocks 中的相关方块已注册
    EXPECT_NE(VanillaBlocks::CHORUS_PLANT, nullptr);
    EXPECT_NE(VanillaBlocks::CHORUS_FLOWER, nullptr);
    EXPECT_NE(VanillaBlocks::END_STONE, nullptr);
}

TEST_F(ChorusPlantBlockTest, ShapeCenterSize)
{
    // 参考 MC 1.16.5 SixWayBlock
    // apothem = 0.3125 (5像素)
    // 中心形状：(0.1875, 0.1875, 0.1875) -> (0.8125, 0.8125, 0.8125)
    // 即 (3, 3, 3) -> (13, 13, 13) 像素，尺寸为 10x10x10 像素
    ChorusPlantBlock block(BlockProperties(Material::PLANT).noCollision().hardness(0.0f));
    const BlockState& noConnection = block.defaultState();
    const CollisionShape& shape = block.getShape(noConnection);

    // 中心柱形状不为空
    EXPECT_FALSE(shape.isEmpty());
}

// ============================================================================
// canConnect 测试
// 使用 VanillaBlocks::CHORUS_PLANT 进行测试，因为 canConnect 使用指针比较
// ============================================================================

TEST_F(ChorusPlantBlockTest, CanConnect_ToChorusPlant)
{
    ChorusPlantTestWorld world;

    // 在原位置放置紫颂植物
    const BlockPos centerPos(0, 64, 0);
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    world.setBlockAt(centerPos, &plantState);

    // 使用 VanillaBlocks::CHORUS_PLANT 的 canConnect 方法
    // 因为 canConnect 使用 adjState->is(this) 进行指针比较

    // 检查从相邻位置向紫颂植物方向连接
    // BlockPos(0, 65, 0) 向 Down 方向检查，会检查 BlockPos(0, 64, 0) 的方块
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 65, 0), Direction::Down));
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 63, 0), Direction::Up));
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 1), Direction::North));
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, -1), Direction::South));
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(-1, 64, 0), Direction::East));
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(1, 64, 0), Direction::West));
}

TEST_F(ChorusPlantBlockTest, CanConnect_ToChorusFlower)
{
    ChorusPlantTestWorld world;

    // 在各方向放置紫颂花
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 上方放置紫颂花
    world.setBlockAt(BlockPos(0, 65, 0), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::Up));

    // 下方放置紫颂花
    ChorusPlantTestWorld world2;
    world2.setBlockAt(BlockPos(0, 63, 0), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world2, BlockPos(0, 64, 0), Direction::Down));

    // 北方放置紫颂花
    ChorusPlantTestWorld world3;
    world3.setBlockAt(BlockPos(0, 64, -1), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world3, BlockPos(0, 64, 0), Direction::North));

    // 南方放置紫颂花
    ChorusPlantTestWorld world4;
    world4.setBlockAt(BlockPos(0, 64, 1), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world4, BlockPos(0, 64, 0), Direction::South));

    // 东方放置紫颂花
    ChorusPlantTestWorld world5;
    world5.setBlockAt(BlockPos(1, 64, 0), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world5, BlockPos(0, 64, 0), Direction::East));

    // 西方放置紫颂花
    ChorusPlantTestWorld world6;
    world6.setBlockAt(BlockPos(-1, 64, 0), &flowerState);
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world6, BlockPos(0, 64, 0), Direction::West));
}

TEST_F(ChorusPlantBlockTest, CanConnect_ToEndStone_OnlyDownward)
{
    ChorusPlantTestWorld world;

    // 在下方放置末地石（应该能连接）
    const BlockState& endStoneState = VanillaBlocks::END_STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &endStoneState);

    // 只有向下方向能连接到末地石
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::Down));

    // 其他方向不能连接到末地石
    ChorusPlantTestWorld world2;
    world2.setBlockAt(BlockPos(0, 65, 0), &endStoneState); // 上方末地石
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world2, BlockPos(0, 64, 0), Direction::Up));

    ChorusPlantTestWorld world3;
    world3.setBlockAt(BlockPos(0, 64, -1), &endStoneState); // 北方末地石
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world3, BlockPos(0, 64, 0), Direction::North));

    ChorusPlantTestWorld world4;
    world4.setBlockAt(BlockPos(0, 64, 1), &endStoneState); // 南方末地石
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world4, BlockPos(0, 64, 0), Direction::South));

    ChorusPlantTestWorld world5;
    world5.setBlockAt(BlockPos(1, 64, 0), &endStoneState); // 东方末地石
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world5, BlockPos(0, 64, 0), Direction::East));

    ChorusPlantTestWorld world6;
    world6.setBlockAt(BlockPos(-1, 64, 0), &endStoneState); // 西方末地石
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world6, BlockPos(0, 64, 0), Direction::West));
}

TEST_F(ChorusPlantBlockTest, CanConnect_NotToRegularBlocks)
{
    ChorusPlantTestWorld world;

    // 放置普通方块（石头）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &stoneState);

    // 不能连接到普通方块（即使是在下方）
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::Down));

    // 其他方向测试
    ChorusPlantTestWorld world2;
    world2.setBlockAt(BlockPos(0, 65, 0), &stoneState);
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world2, BlockPos(0, 64, 0), Direction::Up));
}

TEST_F(ChorusPlantBlockTest, CanConnect_NotToAir)
{
    ChorusPlantTestWorld world;

    // 空世界（所有位置都是空气）

    // 不能连接到空气
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::Down));
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::Up));
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::North));
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::South));
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::East));
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->canConnect(world, BlockPos(0, 64, 0), Direction::West));
}

// ============================================================================
// isValidPosition 测试
// 使用 VanillaBlocks::CHORUS_PLANT 进行测试
// ============================================================================

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithChorusPlantBelow)
{
    ChorusPlantTestWorld world;

    // 在下方放置紫颂植物
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &plantState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该有效（有紫颂植物在下方可以连接）
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithChorusFlowerBelow)
{
    ChorusPlantTestWorld world;

    // 在下方放置紫颂花
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &flowerState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该有效（有紫颂花在下方可以连接）
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithEndStoneBelow)
{
    ChorusPlantTestWorld world;

    // 在下方放置末地石
    const BlockState& endStoneState = VanillaBlocks::END_STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &endStoneState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该有效（有末地石在下方可以连接）
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithChorusPlantOnSide)
{
    ChorusPlantTestWorld world;

    // 在侧面放置紫颂植物（北方）
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    world.setBlockAt(BlockPos(0, 64, -1), &plantState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该有效（有紫颂植物在侧面可以连接）
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithChorusFlowerAbove)
{
    ChorusPlantTestWorld world;

    // 在上方放置紫颂花
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();
    world.setBlockAt(BlockPos(0, 65, 0), &flowerState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该有效（有紫颂花在上方可以连接）
    EXPECT_TRUE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_NoConnections)
{
    ChorusPlantTestWorld world;

    // 空世界（没有可连接的方块）
    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该无效（没有可连接的方块）
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, IsValidPosition_WithRegularBlockBelow)
{
    ChorusPlantTestWorld world;

    // 在下方放置普通方块（石头）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &stoneState);

    const BlockState& state = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 应该无效（石头不是可连接的方块）
    EXPECT_FALSE(static_cast<const ChorusPlantBlock*>(VanillaBlocks::CHORUS_PLANT)
            ->isValidPosition(state, world, BlockPos(0, 64, 0)));
}

// ============================================================================
// updatePostPlacement 测试
// 使用 VanillaBlocks::CHORUS_PLANT 进行测试（非 const 方法）
// ============================================================================

TEST_F(ChorusPlantBlockTest, UpdatePostPlacement_UpdatesConnectionState)
{
    ChorusPlantTestWorld world;

    // 初始状态：无连接
    const BlockState& initialState = VanillaBlocks::CHORUS_PLANT->defaultState();
    EXPECT_FALSE(initialState.get(BlockStateProperties::DOWN()));
    EXPECT_FALSE(initialState.get(BlockStateProperties::UP()));
    EXPECT_FALSE(initialState.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(initialState.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(initialState.get(BlockStateProperties::WEST()));
    EXPECT_FALSE(initialState.get(BlockStateProperties::EAST()));

    // 在下方放置末地石
    const BlockState& endStoneState = VanillaBlocks::END_STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &endStoneState);

    // 更新下方邻居 - updatePostPlacement 是非 const 方法，直接使用 VanillaBlocks 指针
    BlockState updatedState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::Down, endStoneState, world, BlockPos(0, 64, 0), BlockPos(0, 63, 0));

    // 应该更新向下连接为 true
    EXPECT_TRUE(updatedState.get(BlockStateProperties::DOWN()));
    EXPECT_FALSE(updatedState.get(BlockStateProperties::UP()));
    EXPECT_FALSE(updatedState.get(BlockStateProperties::NORTH()));
    EXPECT_FALSE(updatedState.get(BlockStateProperties::SOUTH()));
    EXPECT_FALSE(updatedState.get(BlockStateProperties::WEST()));
    EXPECT_FALSE(updatedState.get(BlockStateProperties::EAST()));
}

TEST_F(ChorusPlantBlockTest, UpdatePostPlacement_UpdatesAllDirections)
{
    ChorusPlantTestWorld world;
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    const BlockState& initialState = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 测试各个方向的更新
    // 向上
    world.setBlockAt(BlockPos(0, 65, 0), &plantState);
    BlockState upState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::Up, plantState, world, BlockPos(0, 64, 0), BlockPos(0, 65, 0));
    EXPECT_TRUE(upState.get(BlockStateProperties::UP()));

    // 向下
    ChorusPlantTestWorld world2;
    world2.setBlockAt(BlockPos(0, 63, 0), &plantState);
    BlockState downState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::Down, plantState, world2, BlockPos(0, 64, 0), BlockPos(0, 63, 0));
    EXPECT_TRUE(downState.get(BlockStateProperties::DOWN()));

    // 向北
    ChorusPlantTestWorld world3;
    world3.setBlockAt(BlockPos(0, 64, -1), &plantState);
    BlockState northState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::North, plantState, world3, BlockPos(0, 64, 0), BlockPos(0, 64, -1));
    EXPECT_TRUE(northState.get(BlockStateProperties::NORTH()));

    // 向南
    ChorusPlantTestWorld world4;
    world4.setBlockAt(BlockPos(0, 64, 1), &plantState);
    BlockState southState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::South, plantState, world4, BlockPos(0, 64, 0), BlockPos(0, 64, 1));
    EXPECT_TRUE(southState.get(BlockStateProperties::SOUTH()));

    // 向东
    ChorusPlantTestWorld world5;
    world5.setBlockAt(BlockPos(1, 64, 0), &plantState);
    BlockState eastState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::East, plantState, world5, BlockPos(0, 64, 0), BlockPos(1, 64, 0));
    EXPECT_TRUE(eastState.get(BlockStateProperties::EAST()));

    // 向西
    ChorusPlantTestWorld world6;
    world6.setBlockAt(BlockPos(-1, 64, 0), &plantState);
    BlockState westState = VanillaBlocks::CHORUS_PLANT->updatePostPlacement(
        initialState, Direction::West, plantState, world6, BlockPos(0, 64, 0), BlockPos(-1, 64, 0));
    EXPECT_TRUE(westState.get(BlockStateProperties::WEST()));
}

// ============================================================================
// ChorusFlowerBlock 测试
// 测试 isValidPosition 方法的各种场景
// ============================================================================

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnChorusPlant)
{
    // 情况1：下方是紫颂植物 - 应该可以放置
    ChorusPlantTestWorld world;
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &plantState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnEndStone)
{
    // 情况2：下方是末地石 - 应该可以放置（作为生长基底）
    ChorusPlantTestWorld world;
    const BlockState& endStoneState = VanillaBlocks::END_STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &endStoneState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnChorusFlower)
{
    // 情况3：下方是紫颂花 - 应该可以放置（水平分支生长）
    ChorusPlantTestWorld world;
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &flowerState);

    EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnRegularBlock)
{
    // 情况4：下方是普通方块（非空气） - 应该无法放置
    ChorusPlantTestWorld world;
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 63, 0), &stoneState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnAirWithHorizontalSupport)
{
    // 情况5：下方是空气，恰好有一个水平方向的紫颂植物支撑
    ChorusPlantTestWorld world;
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 北方有一个紫颂植物
    world.setBlockAt(BlockPos(0, 64, -1), &plantState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 位置 (0, 64, 0) 下方是空气，北方有紫颂植物，应该可以放置
    EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnAirWithMultipleHorizontalSupports)
{
    // 情况6：下方是空气，有多个水平方向的紫颂植物 - 应该无法放置
    ChorusPlantTestWorld world;
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();

    // 北方和南方都有紫颂植物
    world.setBlockAt(BlockPos(0, 64, -1), &plantState);
    world.setBlockAt(BlockPos(0, 64, 1), &plantState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 位置 (0, 64, 0) 有两个水平支撑，应该无法放置
    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnAirWithNoHorizontalSupport)
{
    // 情况7：下方是空气，没有水平方向的紫颂植物支撑 - 应该无法放置
    ChorusPlantTestWorld world;
    // 空世界，没有任何方块

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_OnAirWithBlockingBlock)
{
    // 情况8：下方是空气，有紫颂植物支撑，但其他方向有非空气方块阻挡
    ChorusPlantTestWorld world;
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 北方有紫颂植物
    world.setBlockAt(BlockPos(0, 64, -1), &plantState);
    // 南方有石头（阻挡）
    world.setBlockAt(BlockPos(0, 64, 1), &stoneState);

    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 位置 (0, 64, 0) 有非空气非紫颂植物的方块阻挡，应该无法放置
    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_AllHorizontalDirections)
{
    // 测试所有水平方向都可以作为支撑
    const BlockState& plantState = VanillaBlocks::CHORUS_PLANT->defaultState();
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 北方支撑
    {
        ChorusPlantTestWorld world;
        world.setBlockAt(BlockPos(0, 64, -1), &plantState);
        EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
                ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
    }

    // 南方支撑
    {
        ChorusPlantTestWorld world;
        world.setBlockAt(BlockPos(0, 64, 1), &plantState);
        EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
                ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
    }

    // 东方支撑
    {
        ChorusPlantTestWorld world;
        world.setBlockAt(BlockPos(1, 64, 0), &plantState);
        EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
                ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
    }

    // 西方支撑
    {
        ChorusPlantTestWorld world;
        world.setBlockAt(BlockPos(-1, 64, 0), &plantState);
        EXPECT_TRUE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
                ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
    }
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_ChainsOfFlowersNotAllowedHorizontally)
{
    // 紫颂花不能作为水平支撑（只有紫颂植物可以）
    ChorusPlantTestWorld world;
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 水平方向放置紫颂花（不是紫颂植物）
    world.setBlockAt(BlockPos(0, 64, -1), &flowerState);

    // 下方是空气，水平方向只有紫颂花，不是有效的支撑
    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}

TEST_F(ChorusPlantBlockTest, ChorusFlower_IsValidPosition_EndStoneNotHorizontalSupport)
{
    // 末地石不能作为水平支撑（只能作为底部支撑）
    ChorusPlantTestWorld world;
    const BlockState& endStoneState = VanillaBlocks::END_STONE->defaultState();
    const BlockState& flowerState = VanillaBlocks::CHORUS_FLOWER->defaultState();

    // 水平方向放置末地石
    world.setBlockAt(BlockPos(0, 64, -1), &endStoneState);

    // 下方是空气，水平方向的末地石不是有效支撑
    EXPECT_FALSE(static_cast<const ChorusFlowerBlock*>(VanillaBlocks::CHORUS_FLOWER)
            ->isValidPosition(flowerState, world, BlockPos(0, 64, 0)));
}
