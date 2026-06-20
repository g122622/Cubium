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
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/blocks/nether/NetherPortalBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 下界传送门测试用世界
 *
 * 继承 IBlockReader，提供最小化测试环境
 */
class NetherPortalTestWorld final : public IBlockReader {
public:
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
        } else {
            m_blocks[pos] = state;
        }
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
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
    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() const override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void clearBlockAt(const BlockPos& pos) { m_blocks.erase(pos); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<NetherPortalTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

private:
    void ensureTickManager() const
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<NetherPortalTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

class NetherPortalBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(NetherPortalBlockTest, VanillaBlocksInitialized)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);
}

TEST_F(NetherPortalBlockTest, DefaultStateIsXAxis)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    const BlockState& defaultState = VanillaBlocks::NETHER_PORTAL->defaultState();
    EXPECT_EQ(defaultState.get(BlockStateProperties::HORIZONTAL_AXIS()), Axis::X);
}

TEST_F(NetherPortalBlockTest, GetAxisReturnsCorrectValue)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    const NetherPortalBlock* portal = static_cast<const NetherPortalBlock*>(VanillaBlocks::NETHER_PORTAL);

    const BlockState& xState = portal->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::X);
    EXPECT_EQ(portal->getAxis(xState), Axis::X);

    const BlockState& zState = portal->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::Z);
    EXPECT_EQ(portal->getAxis(zState), Axis::Z);
}

TEST_F(NetherPortalBlockTest, IsValidPositionWithObsidianAbove)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);

    const BlockPos portalPos(5, 64, 5);
    const BlockPos obsidianPos(5, 65, 5);

    world.setBlockAt(obsidianPos, &VanillaBlocks::OBSIDIAN->defaultState());

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsValidPositionWithObsidianBelow)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);

    const BlockPos portalPos(5, 64, 5);
    const BlockPos obsidianPos(5, 63, 5);

    world.setBlockAt(obsidianPos, &VanillaBlocks::OBSIDIAN->defaultState());

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsValidPositionWithPortalAbove)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);

    const BlockPos portalPos(5, 64, 5);
    const BlockPos otherPortalPos(5, 65, 5);

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    world.setBlockAt(otherPortalPos, &portalState);

    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsValidPositionWithObsidianOnSide)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);

    // X轴传送门：宽度方向是东西，深度方向是南北
    // 测试深度方向的框架
    const BlockPos portalPos(5, 64, 5);
    const BlockPos obsidianPos(5, 64, 6); // 南方

    world.setBlockAt(obsidianPos, &VanillaBlocks::OBSIDIAN->defaultState());

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsValidPositionWithPortalOnSide)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);

    // 测试宽度方向有其他传送门方块
    const BlockPos portalPos(5, 64, 5);
    const BlockPos otherPortalPos(6, 64, 5); // 东方

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();
    world.setBlockAt(otherPortalPos, &portalState);

    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsNotValidPositionWithNoConnection)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);

    // 没有任何相邻的传送门或黑曜石
    const BlockPos portalPos(5, 64, 5);
    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();

    EXPECT_FALSE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, IsNotValidPositionWithStoneAround)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    // 四周都是石头，没有传送门或黑曜石
    const BlockPos portalPos(5, 64, 5);
    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();

    // 放置石头在六个方向
    world.setBlockAt(BlockPos(5, 65, 5), &VanillaBlocks::STONE->defaultState()); // 上
    world.setBlockAt(BlockPos(5, 63, 5), &VanillaBlocks::STONE->defaultState()); // 下
    world.setBlockAt(BlockPos(6, 64, 5), &VanillaBlocks::STONE->defaultState()); // 东
    world.setBlockAt(BlockPos(4, 64, 5), &VanillaBlocks::STONE->defaultState()); // 西
    world.setBlockAt(BlockPos(5, 64, 6), &VanillaBlocks::STONE->defaultState()); // 南
    world.setBlockAt(BlockPos(5, 64, 4), &VanillaBlocks::STONE->defaultState()); // 北

    EXPECT_FALSE(VanillaBlocks::NETHER_PORTAL->isValidPosition(portalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, ZAxisPortalValidWithObsidianOnEastWest)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);

    // Z轴传送门：宽度方向是南北，深度方向是东西
    // 测试深度方向（东西）的框架
    const BlockPos portalPos(5, 64, 5);
    const BlockPos obsidianPos(6, 64, 5); // 东方

    world.setBlockAt(obsidianPos, &VanillaBlocks::OBSIDIAN->defaultState());

    const BlockState& zPortalState =
        VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::Z);
    EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(zPortalState, world, portalPos));
}

TEST_F(NetherPortalBlockTest, PortalShapeIsCorrect)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    const NetherPortalBlock* portal = static_cast<const NetherPortalBlock*>(VanillaBlocks::NETHER_PORTAL);

    const BlockState& xState = portal->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::X);
    const BlockState& zState = portal->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), Axis::Z);

    // 传送门没有碰撞
    const CollisionShape& xCollision = portal->getCollisionShape(xState);
    const CollisionShape& zCollision = portal->getCollisionShape(zState);
    EXPECT_TRUE(xCollision.isEmpty());
    EXPECT_TRUE(zCollision.isEmpty());

    // 传送门有渲染形状
    const CollisionShape& xShape = portal->getShape(xState);
    const CollisionShape& zShape = portal->getShape(zState);
    EXPECT_FALSE(xShape.isEmpty());
    EXPECT_FALSE(zShape.isEmpty());
}

TEST_F(NetherPortalBlockTest, PortalIsNotOpaque)
{
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    const NetherPortalBlock* portal = static_cast<const NetherPortalBlock*>(VanillaBlocks::NETHER_PORTAL);
    const BlockState& state = portal->defaultState();

    EXPECT_FALSE(portal->isOpaque(state));
}

TEST_F(NetherPortalBlockTest, UpdatePostPlacementRemovesInvalidPortal)
{
    NetherPortalTestWorld world;
    ASSERT_NE(VanillaBlocks::NETHER_PORTAL, nullptr);
    ASSERT_NE(VanillaBlocks::OBSIDIAN, nullptr);

    const BlockPos portalPos(5, 64, 5);
    const BlockPos obsidianPos(5, 63, 5);

    // 放置黑曜石作为支撑
    world.setBlockAt(obsidianPos, &VanillaBlocks::OBSIDIAN->defaultState());

    const BlockState& portalState = VanillaBlocks::NETHER_PORTAL->defaultState();

    // 有支撑时应该有效
    BlockState result = VanillaBlocks::NETHER_PORTAL->updatePostPlacement(
        portalState, Direction::Down, portalState, world, portalPos, obsidianPos);
    EXPECT_TRUE(result.is(VanillaBlocks::NETHER_PORTAL));

    // 移除支撑
    world.clearBlockAt(obsidianPos);

    // 无支撑时应该返回空气
    result = VanillaBlocks::NETHER_PORTAL->updatePostPlacement(
        portalState, Direction::Down, portalState, world, portalPos, obsidianPos);

    // 应该返回空气状态
    EXPECT_TRUE(result.isAir());
}

} // namespace
