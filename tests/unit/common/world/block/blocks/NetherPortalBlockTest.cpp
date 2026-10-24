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

#include "common/TestWorldHelper.hpp"
#include <gtest/gtest.h>

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
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
        return BlockRegistry::instance().airState();
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

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override
    {
        return nullptr;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return 64;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 12345;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 0;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Easy;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] bool isUltraWarm() const override
    {
        return false;
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state)
    {
        (void)setBlockState(pos.x, pos.y, pos.z, state);
    }

    void clearBlockAt(const BlockPos& pos)
    {
        m_blocks.erase(pos);
    }

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

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

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
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
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

// 最小的 2×3 门，框架不要求四角；两种轴向共用同一夹具。
void _buildCompletePortal(NetherPortalTestWorld& world, Axis axis)
{
    const Direction widthDir = axis == Axis::X ? Direction::East : Direction::South;
    const BlockPos corner(5, 64, 5);
    const auto& portal =
        VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
    for (i32 width = 0; width < 2; ++width) {
        world.setBlockAt(corner.offset(widthDir, width).down(), &VanillaBlocks::OBSIDIAN->defaultState());
        world.setBlockAt(corner.offset(widthDir, width).up(3), &VanillaBlocks::OBSIDIAN->defaultState());
        for (i32 height = 0; height < 3; ++height) {
            world.setBlockAt(corner.offset(widthDir, width).up(height), &portal);
        }
    }
    for (i32 height = 0; height < 3; ++height) {
        world.setBlockAt(
            corner.offset(Directions::opposite(widthDir)).up(height), &VanillaBlocks::OBSIDIAN->defaultState());
        world.setBlockAt(corner.offset(widthDir, 2).up(height), &VanillaBlocks::OBSIDIAN->defaultState());
    }
}

TEST_F(NetherPortalBlockTest, CompleteFrameAndInteriorAreValidOnBothAxes)
{
    for (const Axis axis : {Axis::X, Axis::Z}) {
        NetherPortalTestWorld world;
        _buildCompletePortal(world, axis);
        const auto& state =
            VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
        EXPECT_TRUE(VanillaBlocks::NETHER_PORTAL->isValidPosition(state, world, BlockPos(5, 65, 5)));
    }
}

TEST_F(NetherPortalBlockTest, MissingInteriorInvalidatesRemainingPortalOnBothAxes)
{
    for (const Axis axis : {Axis::X, Axis::Z}) {
        NetherPortalTestWorld world;
        _buildCompletePortal(world, axis);
        const auto& state =
            VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), axis);
        world.clearBlockAt(BlockPos(5, 64, 5));
        EXPECT_FALSE(VanillaBlocks::NETHER_PORTAL->isValidPosition(state, world, BlockPos(5, 65, 5)));
        const auto result = VanillaBlocks::NETHER_PORTAL->updatePostPlacement(state,
            Direction::Down,
            *BlockRegistry::instance().airState(),
            world,
            BlockPos(5, 65, 5),
            BlockPos(5, 64, 5));
        EXPECT_TRUE(result.isAir());
    }
}

TEST_F(NetherPortalBlockTest, MissingFrameInvalidatesRemainingPortal)
{
    NetherPortalTestWorld world;
    _buildCompletePortal(world, Axis::X);
    world.clearBlockAt(BlockPos(4, 65, 5));
    const auto& state = VanillaBlocks::NETHER_PORTAL->defaultState();
    const auto result = VanillaBlocks::NETHER_PORTAL->updatePostPlacement(
        state, Direction::West, *BlockRegistry::instance().airState(), world, BlockPos(5, 65, 5), BlockPos(4, 65, 5));
    EXPECT_TRUE(result.isAir());
}

TEST_F(NetherPortalBlockTest, SingleObsidianNeighborDoesNotValidatePortal)
{
    NetherPortalTestWorld world;
    world.setBlockAt(BlockPos(5, 63, 5), &VanillaBlocks::OBSIDIAN->defaultState());
    const auto& state = VanillaBlocks::NETHER_PORTAL->defaultState();
    EXPECT_FALSE(VanillaBlocks::NETHER_PORTAL->isValidPosition(state, world, BlockPos(5, 64, 5)));
}

TEST_F(NetherPortalBlockTest, UpdateOutsidePortalPlaneDoesNotRemovePortal)
{
    NetherPortalTestWorld world;
    const auto& state = VanillaBlocks::NETHER_PORTAL->defaultState();
    const auto result = VanillaBlocks::NETHER_PORTAL->updatePostPlacement(
        state, Direction::North, *BlockRegistry::instance().airState(), world, BlockPos(5, 64, 5), BlockPos(5, 64, 4));
    EXPECT_TRUE(result.is(VanillaBlocks::NETHER_PORTAL));
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

} // namespace
