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

#include "common/TestWorldHelper.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/cave/BigDripleafStemBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

using namespace mc;
using namespace mc::blocks;
using namespace mc::test;

namespace {

class BigDripleafStemTestWorld final : public BaseTestWorld {
public:
    BigDripleafStemTestWorld() = default;

    void ensureTickManager()
    {
        if (!m_tickManagerPtr) {
            m_tickManagerPtr = std::make_unique<world::tick::TickManager>(*this);
        }
    }

    // 存储 BlockState 的副本并返回指针
    const BlockState* storeBlockState(const BlockState& state)
    {
        m_storedStates.push_back(std::make_unique<BlockState>(state));
        return m_storedStates.back().get();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(packPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second;
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        if (state == nullptr) {
            m_blocks.erase(packPos(x, y, z));
        } else {
            // 存储 BlockState 的副本
            m_storedStates.push_back(std::make_unique<BlockState>(*state));
            m_blocks[packPos(x, y, z)] = m_storedStates.back().get();
        }
        return true;
    }

    bool setBlockState(const BlockPos& pos, const BlockState* state)
    {
        return setBlockState(pos.x, pos.y, pos.z, state);
    }

    // 存储 BlockState 并设置
    bool setBlockStateCopy(const BlockPos& pos, const BlockState& state)
    {
        const BlockState* stored = storeBlockState(state);
        return setBlockState(pos, stored);
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        if (state != nullptr) {
            const fluid::FluidState* fluidState = state->getFluidState();
            if (fluidState != nullptr) {
                return fluidState;
            }
        }
        return &fluid::Fluids::EMPTY()->defaultState();
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        const_cast<BigDripleafStemTestWorld*>(this)->ensureTickManager();
        return *m_tickManagerPtr;
    }

    [[nodiscard]] u64 seed() const override { return m_seed; }

    void setSeed(u64 seed) { m_seed = seed; }

    // 判断 setBlockState 是否被调用过（用于验证方块被移除）
    [[nodiscard]] bool blockWasSetToAir(const BlockPos& pos) const
    {
        const auto it = m_blocks.find(packPos(pos.x, pos.y, pos.z));
        if (it != m_blocks.end()) {
            return it->second->isAir();
        }
        return true; // 不存在视为空气
    }

private:
    [[nodiscard]] static i64 packPos(i32 x, i32 y, i32 z)
    {
        return (static_cast<i64>(x) << 42) ^ (static_cast<i64>(y) << 21) ^ static_cast<i64>(z & 0x1FFFFF);
    }

    std::unordered_map<i64, const BlockState*> m_blocks;
    std::vector<std::unique_ptr<BlockState>> m_storedStates;
    std::unique_ptr<world::tick::TickManager> m_tickManagerPtr;
    u64 m_seed = 12345;
};

// 用于测试的实心方块（提供 BIG_DRIPLEAF_PLACEABLE 标签支撑）
class TestSolidBlock final : public Block {
public:
    explicit TestSolidBlock(const BlockProperties& properties)
        : Block(properties)
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block,
                std::vector<size_t> values,
                const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                const std::vector<BlockState*>* allStates,
                u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    [[nodiscard]] bool isSolidSide(
        const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(side);
        return true;
    }
};

} // namespace

// ============================================================================
// 基本属性测试
// ============================================================================

class BigDripleafStemBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        stemBlock_ = std::make_unique<BigDripleafStemBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(0.1f));
    }

    std::unique_ptr<BigDripleafStemBlock> stemBlock_;
};

TEST_F(BigDripleafStemBlockTest, Create_HasCorrectProperties)
{
    EXPECT_NE(stemBlock_, nullptr);
}

TEST_F(BigDripleafStemBlockTest, DefaultState_HasCorrectFacing)
{
    const BlockState& state = stemBlock_->defaultState();
    EXPECT_EQ(state.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(BigDripleafStemBlockTest, DefaultState_NotWaterlogged)
{
    const BlockState& state = stemBlock_->defaultState();
    EXPECT_FALSE(state.get(BlockStateProperties::WATERLOGGED()));
}

TEST_F(BigDripleafStemBlockTest, UseShapeForLightOcclusion_AlwaysTrue)
{
    const BlockState& state = stemBlock_->defaultState();
    EXPECT_TRUE(stemBlock_->useShapeForLightOcclusion(state));
}

// ============================================================================
// 集成测试 - updatePostPlacement 和 tick 交互
// ============================================================================

class BigDripleafStemIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        stemBlock_ = std::make_unique<BigDripleafStemBlock>(
            BlockProperties(Material::PLANT).noCollision().hardness(0.1f).resistance(0.1f));
        world_.ensureTickManager();
    }

    // 设置有效的三层结构：实心支撑 -> 大滴叶茎 -> 大滴叶
    void setupValidStructure(const BlockPos& stemPos, const BlockPos& leafPos)
    {
        BlockPos belowPos(stemPos.x, stemPos.y - 1, stemPos.z);
        // 下方：BIG_DRIPLEAF_PLACEABLE 标签方块（泥土在标签中）
        world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
        // 中间：大滴叶茎
        world_.setBlockStateCopy(stemPos, stemBlock_->defaultState());
        // 上方：大滴叶
        world_.setBlockState(leafPos, &VanillaBlocks::BIG_DRIPLEAF->defaultState());
    }

    std::unique_ptr<BigDripleafStemBlock> stemBlock_;
    BigDripleafStemTestWorld world_;
};

// ============================================================================
// updatePostPlacement 测试
// ============================================================================

TEST_F(BigDripleafStemIntegrationTest, UpdatePostPlacement_SupportLost_SchedulesTick)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);
    BlockPos belowPos(0, 64, 0);

    // 设置有效结构
    setupValidStructure(stemPos, leafPos);

    // 先移除下方支撑方块（模拟方块变化已经发生）
    world_.setBlockState(belowPos, &VanillaBlocks::AIR->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);

    // 通知茎方块：下方邻居变为空气
    BlockState state = *stemState;
    auto result = stemBlock_->updatePostPlacement(
        state, Direction::Down, VanillaBlocks::AIR->defaultState(), world_, stemPos, belowPos);

    // updatePostPlacement 不应直接返回空气，而是返回原状态并调度 tick
    EXPECT_EQ(
        result.get(BlockStateProperties::HORIZONTAL_FACING()), state.get(BlockStateProperties::HORIZONTAL_FACING()));

    // 验证已调度 block tick
    EXPECT_TRUE(world_.tickManager().isBlockTickScheduled(stemPos, *stemBlock_));
}

TEST_F(BigDripleafStemIntegrationTest, UpdatePostPlacement_SupportIntact_NoTickScheduled)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);

    // 设置有效结构
    setupValidStructure(stemPos, leafPos);

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);

    // 水平方向邻居变化不影响存活
    BlockPos neighborPos(1, 65, 0);
    BlockState state = *stemState;
    auto result = stemBlock_->updatePostPlacement(
        state, Direction::East, VanillaBlocks::AIR->defaultState(), world_, stemPos, neighborPos);

    // 不应调度 tick
    EXPECT_FALSE(world_.tickManager().isBlockTickScheduled(stemPos, *stemBlock_));
}

TEST_F(BigDripleafStemIntegrationTest, UpdatePostPlacement_LeafRemoved_SchedulesTick)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);

    // 设置有效结构
    setupValidStructure(stemPos, leafPos);

    // 移除上方大滴叶（模拟方块变化已经发生）
    world_.setBlockState(leafPos, &VanillaBlocks::AIR->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);

    // 通知茎方块：上方邻居变为空气
    BlockState state = *stemState;
    auto result = stemBlock_->updatePostPlacement(
        state, Direction::Up, VanillaBlocks::AIR->defaultState(), world_, stemPos, leafPos);

    // 应该调度 tick
    EXPECT_TRUE(world_.tickManager().isBlockTickScheduled(stemPos, *stemBlock_));
}

// ============================================================================
// tick 测试 - 延迟销毁
// ============================================================================

TEST_F(BigDripleafStemIntegrationTest, Tick_StillUnsupported_DestroysBlock)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos belowPos(0, 64, 0);

    // 仅放置茎，不放置下方支撑和上方大滴叶 -> 无法存活
    world_.setBlockStateCopy(stemPos, stemBlock_->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);
    ASSERT_TRUE(stemState->is(stemBlock_.get()));

    // 直接调用 tick（模拟延迟 tick 触发）
    BlockState mutableState = *stemState;
    math::Random& rng = world_.getRandom();
    stemBlock_->tick(world_, stemPos, mutableState, rng);

    // 方块应被销毁（变为空气）
    const BlockState* finalState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

TEST_F(BigDripleafStemIntegrationTest, Tick_SupportRestored_BlockSurvives)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);
    BlockPos belowPos(0, 64, 0);

    // 初始设置有效结构
    setupValidStructure(stemPos, leafPos);

    // 移除下方支撑 -> 无法存活
    world_.setBlockState(belowPos, &VanillaBlocks::AIR->defaultState());

    // 在 tick 触发之前恢复下方支撑
    world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);

    // tick 触发 -> 重新检查，支撑已恢复，方块存活
    BlockState mutableState = *stemState;
    math::Random& rng = world_.getRandom();
    stemBlock_->tick(world_, stemPos, mutableState, rng);

    // 方块应存活
    const BlockState* finalState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->is(stemBlock_.get()));
}

TEST_F(BigDripleafStemIntegrationTest, Tick_BlockReplaced_DoesNothing)
{
    BlockPos stemPos(0, 65, 0);

    // 在世界中放置空气（方块已被替换）
    world_.setBlockState(stemPos, &VanillaBlocks::AIR->defaultState());

    // 尝试对已不存在的茎调用 tick
    BlockState stemState = stemBlock_->defaultState();
    math::Random& rng = world_.getRandom();
    stemBlock_->tick(world_, stemPos, stemState, rng);

    // 位置应仍为空气
    const BlockState* finalState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->isAir());
}

TEST_F(BigDripleafStemIntegrationTest, Tick_BlockReplacedWithDifferentBlock_DoesNothing)
{
    BlockPos stemPos(0, 65, 0);

    // 在世界中放置石砖（方块已被替换为不同方块）
    world_.setBlockState(stemPos, &VanillaBlocks::DIRT->defaultState());

    // 尝试对已被替换的位置调用茎的 tick
    BlockState stemState = stemBlock_->defaultState();
    math::Random& rng = world_.getRandom();
    stemBlock_->tick(world_, stemPos, stemState, rng);

    // 石砖应保持不变
    const BlockState* finalState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(finalState, nullptr);
    EXPECT_TRUE(finalState->is(VanillaBlocks::DIRT));
}

// ============================================================================
// isValidPosition 测试
// ============================================================================

TEST_F(BigDripleafStemIntegrationTest, IsValidPosition_BothSupportsValid_ReturnsTrue)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);

    setupValidStructure(stemPos, leafPos);

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);
    EXPECT_TRUE(stemBlock_->isValidPosition(*stemState, world_, stemPos));
}

TEST_F(BigDripleafStemIntegrationTest, IsValidPosition_NoSupportBelow_ReturnsFalse)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos leafPos(0, 66, 0);

    // 仅放置茎和上方大滴叶，下方无支撑
    world_.setBlockStateCopy(stemPos, stemBlock_->defaultState());
    world_.setBlockState(leafPos, &VanillaBlocks::BIG_DRIPLEAF->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);
    EXPECT_FALSE(stemBlock_->isValidPosition(*stemState, world_, stemPos));
}

TEST_F(BigDripleafStemIntegrationTest, IsValidPosition_NoSupportAbove_ReturnsFalse)
{
    BlockPos stemPos(0, 65, 0);
    BlockPos belowPos(0, 64, 0);

    // 下方有支撑，上方无大滴叶/茎
    world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
    world_.setBlockStateCopy(stemPos, stemBlock_->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);
    EXPECT_FALSE(stemBlock_->isValidPosition(*stemState, world_, stemPos));
}

TEST_F(BigDripleafStemIntegrationTest, IsValidPosition_StemChainAbove_ReturnsTrue)
{
    // 茎-茎链：下方有支撑，上方也是茎
    BlockPos belowPos(0, 64, 0);
    BlockPos stemPos(0, 65, 0);
    BlockPos stemAbovePos(0, 66, 0);

    world_.setBlockState(belowPos, &VanillaBlocks::DIRT->defaultState());
    world_.setBlockStateCopy(stemPos, stemBlock_->defaultState());
    world_.setBlockStateCopy(stemAbovePos, stemBlock_->defaultState());

    const BlockState* stemState = world_.getBlockState(stemPos.x, stemPos.y, stemPos.z);
    ASSERT_NE(stemState, nullptr);
    EXPECT_TRUE(stemBlock_->isValidPosition(*stemState, world_, stemPos));
}
