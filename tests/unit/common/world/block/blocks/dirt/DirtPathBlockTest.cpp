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

/**
 * @file DirtPathBlockTest.cpp
 * @brief DirtPathBlock 单元测试
 *
 * 测试草径方块的核心行为：
 * - 方块注册为 DirtPathBlock 类型（非 SimpleBlock）
 * - 渲染形状与碰撞形状高度均为 15/16 格（0.9375），顶部比完整方块矮 1 像素
 * - 形状为 SimpleBox（非 FullBlock），boxCount==1
 * - 上方存在固体方块（栅栏门除外）时无法存活：放置时改放泥土、运行中经 1 tick 计划刻转回泥土
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/dirt/DirtPathBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <utility>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief DirtPathBlock 测试用世界桩
 *
 * 提供方块状态读写与真实的 TickManager（用于验证计划刻驱动的地径→泥土转变）。
 */
class DirtPathTestWorld final : public mc::test::BaseTestWorld {
public:
    DirtPathTestWorld() = default;

    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        return it != m_blocks.end() ? it->second : nullptr;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr || state->isAir()) {
            m_blocks.erase(pos);
            m_ownedStates.erase(pos);
        } else {
            // 存储状态副本：传入的 state 可能是栈上临时对象（如 pushEntitiesUp 的返回值）
            auto [it, inserted] = m_ownedStates.insert_or_assign(pos, *state);
            m_blocks[pos] = &it->second;
        }
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        MC_UNUSED(flags);
        return setBlockState(x, y, z, state);
    }

    [[nodiscard]] mc::world::chunk::IChunkManager* chunkManager() override { return &m_stubChunks; }
    [[nodiscard]] const mc::world::chunk::IChunkManager* chunkManager() const override { return &m_stubChunks; }
    mc::test::StubChunkManager m_stubChunks{nullptr, true};

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
        return *m_tickManagerPtr;
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return const_cast<DirtPathTestWorld*>(this)->tickManager();
    }

    void setBlockAt(const BlockPos& pos, const BlockState* state) { (void)setBlockState(pos.x, pos.y, pos.z, state); }

    void clearBlockAt(const BlockPos& pos)
    {
        m_blocks.erase(pos);
        m_ownedStates.erase(pos);
    }

    /// 推进一个计划刻周期（计划刻 delay=1，故需两次 tick 使 scheduledTick 达到）
    void advanceTicks(u64 count)
    {
        for (u64 i = 0; i < count; ++i) {
            tickManager().tick(m_currentTick);
            ++m_currentTick;
        }
    }

private:
    std::map<BlockPos, const BlockState*> m_blocks;
    std::map<BlockPos, BlockState> m_ownedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_currentTick = 0;
};

} // namespace

class DirtPathBlockTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// 方块注册与类型测试
// ============================================================================

TEST_F(DirtPathBlockTest, DirtPathIsRegistered)
{
    ASSERT_NE(VanillaBlocks::GRASS_PATH, nullptr);
}

TEST_F(DirtPathBlockTest, DirtPathIsDirtPathBlockType)
{
    // 草径应注册为 DirtPathBlock 类型（非 SimpleBlock），因其有自定义形状
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    auto* dirtPathBlock = dynamic_cast<const DirtPathBlock*>(dirtPath);
    EXPECT_NE(dirtPathBlock, nullptr) << "GRASS_PATH should be registered as DirtPathBlock, not SimpleBlock";
}

// ============================================================================
// 渲染形状测试
// ============================================================================

TEST_F(DirtPathBlockTest, ShapeHeightIs15Over16)
{
    // 草径渲染形状高度应为 15/16 格（0.9375），顶部比完整方块矮 1 像素
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);

    ASSERT_EQ(shape.boxCount(), 1u) << "DirtPath shape should be a single box";

    const AxisAlignedBB& box = shape.boxes().front();
    EXPECT_FLOAT_EQ(box.minX, 0.0f);
    EXPECT_FLOAT_EQ(box.minY, 0.0f);
    EXPECT_FLOAT_EQ(box.minZ, 0.0f);
    EXPECT_FLOAT_EQ(box.maxX, 1.0f);
    EXPECT_FLOAT_EQ(box.maxY, 15.0f / 16.0f) << "DirtPath shape maxY should be 15/16 (0.9375)";
    EXPECT_FLOAT_EQ(box.maxZ, 1.0f);
}

// ============================================================================
// 碰撞形状测试
// ============================================================================

TEST_F(DirtPathBlockTest, CollisionShapeEqualsShape)
{
    // 草径碰撞形状与渲染形状一致（均为 15/16 高）
    // 注意：与 FarmlandBlock 不同，FarmlandBlock 的碰撞形状是完整方块
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);
    const CollisionShape& collisionShape = dirtPath->getCollisionShape(state);

    ASSERT_EQ(shape.boxCount(), 1u);
    ASSERT_EQ(collisionShape.boxCount(), 1u);

    const AxisAlignedBB& shapeBox = shape.boxes().front();
    const AxisAlignedBB& collisionBox = collisionShape.boxes().front();
    EXPECT_FLOAT_EQ(shapeBox.maxY, collisionBox.maxY)
        << "DirtPath collision shape should equal render shape (15/16 high)";
}

// ============================================================================
// 形状类型测试
// ============================================================================

TEST_F(DirtPathBlockTest, ShapeIsSimpleBoxNotFullBlock)
{
    // 草径形状应为 SimpleBox（非 FullBlock），且仅 1 个 box
    // 这样 ChunkMesher 才会按 shape 的 AABB 渲染非完整高度
    const Block* dirtPath = VanillaBlocks::GRASS_PATH;
    ASSERT_NE(dirtPath, nullptr);

    const BlockState& state = dirtPath->defaultState();
    const CollisionShape& shape = dirtPath->getShape(state);

    EXPECT_FALSE(shape.isEmpty());
    EXPECT_FALSE(shape.isFullBlock())
        << "DirtPath shape should NOT be FullBlock (must be SimpleBox for short-height rendering)";
    EXPECT_EQ(shape.boxCount(), 1u);
}

// ============================================================================
// 存活判定（isValidPosition / canSurvive）
// ============================================================================

TEST_F(DirtPathBlockTest, CanSurviveWhenAboveIsAir)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);

    EXPECT_TRUE(VanillaBlocks::GRASS_PATH->isValidPosition(VanillaBlocks::GRASS_PATH->defaultState(), world, pos));
}

TEST_F(DirtPathBlockTest, CannotSurviveWhenAboveIsSolidBlock)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());

    EXPECT_FALSE(VanillaBlocks::GRASS_PATH->isValidPosition(VanillaBlocks::GRASS_PATH->defaultState(), world, pos));
}

TEST_F(DirtPathBlockTest, CanSurviveWhenAboveIsFenceGate)
{
    // 栅栏门非固体，但 vanilla 例外允许土径位于其下方
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    ASSERT_NE(VanillaBlocks::OAK_FENCE_GATE, nullptr);
    world.setBlockAt(pos.up(), &VanillaBlocks::OAK_FENCE_GATE->defaultState());

    EXPECT_TRUE(VanillaBlocks::GRASS_PATH->isValidPosition(VanillaBlocks::GRASS_PATH->defaultState(), world, pos));
}

TEST_F(DirtPathBlockTest, CanSurviveWhenAboveIsNonSolidBlock)
{
    // 花草等非固体方块不影响土径存活
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    ASSERT_NE(VanillaBlocks::SHORT_GRASS, nullptr);
    world.setBlockAt(pos.up(), &VanillaBlocks::SHORT_GRASS->defaultState());

    EXPECT_TRUE(VanillaBlocks::GRASS_PATH->isValidPosition(VanillaBlocks::GRASS_PATH->defaultState(), world, pos));
}

// ============================================================================
// 放置状态（getStateForPlacement）
// ============================================================================

TEST_F(DirtPathBlockTest, PlacementReturnsDirtPathWhenAboveIsAir)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    // 点击下方石头方块的顶面 → 放置位置为 pos
    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());

    ItemStack stack;
    BlockItemUseContext context(
        world, nullptr, stack, Vector3(0.5f, 64.0f, 0.5f), pos.down(), Direction::Up, 0.0f, 0.0f);
    ASSERT_TRUE(context.placementPos() == pos);

    const BlockState result = VanillaBlocks::GRASS_PATH->getStateForPlacement(context);
    EXPECT_TRUE(result.is(VanillaBlocks::GRASS_PATH));
}

TEST_F(DirtPathBlockTest, PlacementReturnsDirtWhenAboveIsSolidBlock)
{
    // 上方为固体方块时无法放置土径，vanilla 改放泥土（Block.pushEntitiesUp）
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    world.setBlockAt(pos.down(), &VanillaBlocks::STONE->defaultState());
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());

    ItemStack stack;
    BlockItemUseContext context(
        world, nullptr, stack, Vector3(0.5f, 64.0f, 0.5f), pos.down(), Direction::Up, 0.0f, 0.0f);
    ASSERT_TRUE(context.placementPos() == pos);

    const BlockState result = VanillaBlocks::GRASS_PATH->getStateForPlacement(context);
    EXPECT_TRUE(result.is(VanillaBlocks::DIRT));
}

// ============================================================================
// 邻居更新与计划刻转变（updatePostPlacement / tick）
// ============================================================================

TEST_F(DirtPathBlockTest, UpdatePostPlacementSchedulesTickWhenAboveBecomesSolid)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& pathState = VanillaBlocks::GRASS_PATH->defaultState();
    world.setBlockAt(pos, &pathState);

    // 上方出现石头 → 安排 1 tick 后的计划刻
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());
    const BlockState updated = VanillaBlocks::GRASS_PATH->updatePostPlacement(
        pathState, Direction::Up, VanillaBlocks::STONE->defaultState(), world, pos, pos.up());

    // 状态本身不变（仅安排计划刻）
    EXPECT_TRUE(updated.is(VanillaBlocks::GRASS_PATH));
    EXPECT_TRUE(world.tickManager().isBlockTickScheduled(pos, *VanillaBlocks::GRASS_PATH));
}

TEST_F(DirtPathBlockTest, TickTurnsToDirtWhenAboveIsSolid)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& pathState = VanillaBlocks::GRASS_PATH->defaultState();
    world.setBlockAt(pos, &pathState);
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());

    BlockState mutableState = pathState;
    math::Random random(12345);
    VanillaBlocks::GRASS_PATH->tick(world, pos, mutableState, random);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->is(VanillaBlocks::DIRT));
}

TEST_F(DirtPathBlockTest, TickKeepsDirtPathWhenAboveIsAir)
{
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& pathState = VanillaBlocks::GRASS_PATH->defaultState();
    world.setBlockAt(pos, &pathState);

    BlockState mutableState = pathState;
    math::Random random(12345);
    VanillaBlocks::GRASS_PATH->tick(world, pos, mutableState, random);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->is(VanillaBlocks::GRASS_PATH));
}

TEST_F(DirtPathBlockTest, ScheduledTickConvertsDirtPathToDirtAfterPlacement)
{
    // 端到端：上方放固体方块触发计划刻 → 推进 2 tick → 土径变为泥土
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& pathState = VanillaBlocks::GRASS_PATH->defaultState();
    world.setBlockAt(pos, &pathState);

    // 模拟 setBlockState 的形状更新：上方放置石头后对下方土径调用 updatePostPlacement
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());
    static_cast<void>(VanillaBlocks::GRASS_PATH->updatePostPlacement(
        pathState, Direction::Up, VanillaBlocks::STONE->defaultState(), world, pos, pos.up()));

    world.advanceTicks(2);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->is(VanillaBlocks::DIRT));
}

TEST_F(DirtPathBlockTest, RemovesSolidAboveKeepsDirtPath)
{
    // 上方固体被移除后土径不应转变泥土（updatePostPlacement 只在上方变化时判定存活）
    DirtPathTestWorld world;
    const BlockPos pos(0, 64, 0);
    const BlockState& pathState = VanillaBlocks::GRASS_PATH->defaultState();
    world.setBlockAt(pos, &pathState);

    // 模拟上方石头的放置与移除：最终上方为空气
    world.setBlockAt(pos.up(), &VanillaBlocks::STONE->defaultState());
    world.clearBlockAt(pos.up());

    static_cast<void>(VanillaBlocks::GRASS_PATH->updatePostPlacement(
        pathState, Direction::Up, VanillaBlocks::AIR->defaultState(), world, pos, pos.up()));

    world.advanceTicks(2);

    const BlockState* after = world.getBlockState(pos.x, pos.y, pos.z);
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->is(VanillaBlocks::GRASS_PATH));
}
