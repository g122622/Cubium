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
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/blocks/nether/MagmaBlock.hpp"
#include "world/block/blocks/ocean/BubbleColumnBlock.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include "world/fluid/FluidTags.hpp"
#include "world/fluid/fluids/WaterFluid.hpp"
#include "world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 气泡柱测试用世界
 *
 * 继承 IWorld，提供完整的测试环境
 */
class BubbleColumnTestWorld final : public IBlockReader {
public:
    using IWorld::getBlockState;

    BubbleColumnTestWorld()
        : m_currentTick(0)
    {}

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

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_fluids.find(pos);
        if (it != m_fluids.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void setFluidState(i32 x, i32 y, i32 z, const fluid::FluidState& state) { m_fluids[BlockPos(x, y, z)] = state; }

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
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] bool isUltraWarm() const override { return false; }
    [[nodiscard]] bool doFireTick() const override { return true; }
    [[nodiscard]] bool isRaining() const override { return false; }
    [[nodiscard]] bool canRainAt(const BlockPos&) const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }
    [[nodiscard]] const BlockState* getBlockAt(const BlockPos& pos) const { return getBlockState(pos.x, pos.y, pos.z); }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<BubbleColumnTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

private:
    void ensureTickManager() const
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(const_cast<BubbleColumnTestWorld&>(*this));
        }
    }

    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, fluid::FluidState> m_fluids;
    mutable std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
    u64 m_currentTick;
};

/**
 * @brief 获取 MagmaBlock 指针
 */
MagmaBlock* getMagmaBlock()
{
    if (VanillaBlocks::MAGMA == nullptr) {
        return nullptr;
    }
    return const_cast<MagmaBlock*>(static_cast<const MagmaBlock*>(VanillaBlocks::MAGMA));
}

/**
 * @brief 获取 BubbleColumnBlock 指针
 */
BubbleColumnBlock* getBubbleColumnBlock()
{
    if (VanillaBlocks::BUBBLE_COLUMN == nullptr) {
        return nullptr;
    }
    return const_cast<BubbleColumnBlock*>(static_cast<const BubbleColumnBlock*>(VanillaBlocks::BUBBLE_COLUMN));
}

class BubbleColumnBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        mc::fluid::FluidRegistry::instance().initialize();
    }

    void TearDown() override {}
};

// ========== BubbleColumnBlock::canHoldBubbleColumn 测试 ==========

TEST_F(BubbleColumnBlockTest, CanHoldBubbleColumn_ReturnsTrueForWaterSource)
{
    BubbleColumnTestWorld world;

    // 设置水源方块
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), waterState);

    // 设置流体状态为水源
    auto* waterFluid = mc::fluid::FluidRegistry::instance().getFluid(mc::fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState& sourceState = waterFluid->defaultState();
    if (sourceState.isSource()) {
        world.setFluidState(0, 0, 0, sourceState);
    }

    // 检查是否可以放置气泡柱
    EXPECT_TRUE(BubbleColumnBlock::canHoldBubbleColumn(world, BlockPos(0, 0, 0)));
}

TEST_F(BubbleColumnBlockTest, CanHoldBubbleColumn_ReturnsFalseForNonWater)
{
    BubbleColumnTestWorld world;

    // 设置石头方块（非水）
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), stoneState);

    // 检查是否可以放置气泡柱
    EXPECT_FALSE(BubbleColumnBlock::canHoldBubbleColumn(world, BlockPos(0, 0, 0)));
}

TEST_F(BubbleColumnBlockTest, CanHoldBubbleColumn_ReturnsFalseForAir)
{
    BubbleColumnTestWorld world;

    // 空气方块
    EXPECT_FALSE(BubbleColumnBlock::canHoldBubbleColumn(world, BlockPos(0, 0, 0)));
}

// ========== BubbleColumnBlock::getDrag 测试 ==========

TEST_F(BubbleColumnBlockTest, GetDrag_ReturnsTrueForMagma)
{
    BubbleColumnTestWorld world;

    // 设置岩浆块
    const BlockState* magmaState = &VanillaBlocks::MAGMA->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), magmaState);

    // 岩浆块上方应该是下拖气泡柱
    EXPECT_TRUE(BubbleColumnBlock::getDrag(world, BlockPos(0, 0, 0)));
}

TEST_F(BubbleColumnBlockTest, GetDrag_ReturnsFalseForSoulSand)
{
    BubbleColumnTestWorld world;

    // 设置灵魂沙
    const BlockState* soulSandState = &VanillaBlocks::SOUL_SAND->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), soulSandState);

    // 灵魂沙上方应该是上升气泡柱
    EXPECT_FALSE(BubbleColumnBlock::getDrag(world, BlockPos(0, 0, 0)));
}

TEST_F(BubbleColumnBlockTest, GetDrag_ReturnsTrueForEmpty)
{
    BubbleColumnTestWorld world;

    // 空位置默认下拖
    EXPECT_TRUE(BubbleColumnBlock::getDrag(world, BlockPos(0, 0, 0)));
}

TEST_F(BubbleColumnBlockTest, GetDrag_InheritsFromBubbleColumn)
{
    BubbleColumnTestWorld world;

    // 设置下拖气泡柱
    BubbleColumnBlock* bubbleBlock = getBubbleColumnBlock();
    ASSERT_NE(bubbleBlock, nullptr);

    const BlockState& dragState = VanillaBlocks::BUBBLE_COLUMN->defaultState().with(BlockStateProperties::DRAG(), true);
    world.setBlockAt(BlockPos(0, 0, 0), &dragState);

    // 继承气泡柱的 DRAG 状态
    EXPECT_TRUE(BubbleColumnBlock::getDrag(world, BlockPos(0, 0, 0)));

    // 设置上升气泡柱
    const BlockState& upState = VanillaBlocks::BUBBLE_COLUMN->defaultState().with(BlockStateProperties::DRAG(), false);
    world.setBlockAt(BlockPos(0, 0, 0), &upState);

    EXPECT_FALSE(BubbleColumnBlock::getDrag(world, BlockPos(0, 0, 0)));
}

// ========== BubbleColumnBlock::placeBubbleColumn 测试 ==========

TEST_F(BubbleColumnBlockTest, PlaceBubbleColumn_SetsBlockWhenValid)
{
    BubbleColumnTestWorld world;

    // 设置水源方块
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockAt(BlockPos(0, 1, 0), waterState);

    // 设置流体状态为水源
    auto* waterFluid = mc::fluid::FluidRegistry::instance().getFluid(mc::fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState& sourceState = waterFluid->defaultState();
    if (sourceState.isSource()) {
        world.setFluidState(0, 1, 0, sourceState);
    }

    // 放置气泡柱
    BubbleColumnBlock::placeBubbleColumn(world, BlockPos(0, 1, 0), true);

    // 验证气泡柱已放置
    const BlockState* result = world.getBlockAt(BlockPos(0, 1, 0));
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is(VanillaBlocks::BUBBLE_COLUMN));
    EXPECT_TRUE(result->get(BlockStateProperties::DRAG()));
}

TEST_F(BubbleColumnBlockTest, PlaceBubbleColumn_DoesNotSetWhenInvalid)
{
    BubbleColumnTestWorld world;

    // 设置石头方块（非水源）
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), stoneState);

    // 尝试放置气泡柱
    BubbleColumnBlock::placeBubbleColumn(world, BlockPos(0, 0, 0), true);

    // 验证气泡柱未放置（仍然是石头）
    const BlockState* result = world.getBlockAt(BlockPos(0, 0, 0));
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is(VanillaBlocks::STONE));
}

// ========== BubbleColumnBlock::isDrag 测试 ==========

TEST_F(BubbleColumnBlockTest, IsDrag_ReturnsCorrectValue)
{
    BubbleColumnBlock* block = getBubbleColumnBlock();
    ASSERT_NE(block, nullptr);

    // 下拖气泡柱
    const BlockState& dragState = VanillaBlocks::BUBBLE_COLUMN->defaultState().with(BlockStateProperties::DRAG(), true);
    EXPECT_TRUE(block->isDrag(dragState));

    // 上升气泡柱
    const BlockState& upState = VanillaBlocks::BUBBLE_COLUMN->defaultState().with(BlockStateProperties::DRAG(), false);
    EXPECT_FALSE(block->isDrag(upState));
}

// ========== 默认状态测试 ==========

TEST_F(BubbleColumnBlockTest, DefaultState_IsNotDrag)
{
    const BlockState& defaultState = VanillaBlocks::BUBBLE_COLUMN->defaultState();
    EXPECT_FALSE(defaultState.get(BlockStateProperties::DRAG()));
}

// ========== MagmaBlock 测试 ==========

class MagmaBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        mc::fluid::FluidRegistry::instance().initialize();
    }

    void TearDown() override {}
};

TEST_F(MagmaBlockTest, MagmaBlock_Exists)
{
    EXPECT_NE(VanillaBlocks::MAGMA, nullptr);
}

TEST_F(MagmaBlockTest, MagmaBlock_TickCreatesBubbleColumn)
{
    BubbleColumnTestWorld world;

    // 设置岩浆块
    const BlockState* magmaState = &VanillaBlocks::MAGMA->defaultState();
    world.setBlockAt(BlockPos(0, 0, 0), magmaState);

    // 设置水源方块在上方
    const BlockState* waterState = &VanillaBlocks::WATER->defaultState();
    world.setBlockAt(BlockPos(0, 1, 0), waterState);

    // 设置流体状态为水源
    auto* waterFluid = mc::fluid::FluidRegistry::instance().getFluid(mc::fluid::FluidRegistry::WATER_ID);
    ASSERT_NE(waterFluid, nullptr);
    const fluid::FluidState& sourceState = waterFluid->defaultState();
    if (sourceState.isSource()) {
        world.setFluidState(0, 1, 0, sourceState);
    }

    // 获取 MagmaBlock 并执行 tick
    MagmaBlock* magmaBlock = getMagmaBlock();
    ASSERT_NE(magmaBlock, nullptr);

    math::Random random(12345);
    BlockState mutableState = *magmaState;
    magmaBlock->tick(world, BlockPos(0, 0, 0), mutableState, random);

    // 验证上方变为气泡柱
    const BlockState* result = world.getBlockAt(BlockPos(0, 1, 0));
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->is(VanillaBlocks::BUBBLE_COLUMN));
    EXPECT_TRUE(result->get(BlockStateProperties::DRAG())); // 岩浆块产生下拖气泡柱
}

} // namespace
