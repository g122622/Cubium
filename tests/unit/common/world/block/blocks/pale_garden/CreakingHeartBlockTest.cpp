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
 * @file CreakingHeartBlockTest.cpp
 * @brief 嘎枝之心周期检查链路单元测试
 *
 * 参考 MC 1.21.11 net.minecraft.world.level.block.CreakingHeartBlock。
 * 覆盖：hasRequiredLogs、updateState（uprooted→dormant/awake）、
 *       updatePostPlacement 调度、tick 写回、getStateForPlacement 解析、比较器输出。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/pale_garden/CreakingHeartBlock.hpp"
#include "common/world/block/registry/PaleGardenBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <map>
#include <memory>
#include <stdexcept>

using namespace mc;
using namespace mc::blocks;

namespace {

/**
 * @brief 测试用世界：在 BaseTestWorld 之上提供方块存储、真实 TickManager、可切换 CREAKING_ACTIVE。
 */
class CreakingHeartTestWorld final : public mc::test::BaseTestWorld {
public:
    CreakingHeartTestWorld()
        : m_tickManager(*this)
    {
        VanillaBlocks::initialize();
        block_registry::registerPaleGardenBlocks();
        BlockTags::initialize();
    }

    using IWorld::getBlockState;
    using IWorld::setBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_blocks.find(pos);
        return it == m_blocks.end() ? nullptr : it->second.get();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        if (state == nullptr) {
            m_blocks.erase(pos);
        } else {
            m_blocks[pos] = std::make_unique<BlockState>(*state);
        }
        // 记录写回（供 tick 断言）
        m_lastWrittenPos = pos;
        m_writeCount++;
        return true;
    }

    void setBlock(const BlockPos& pos, const BlockState& state)
    {
        m_blocks[pos] = std::make_unique<BlockState>(state);
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        return m_tickManager;
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        return m_tickManager;
    }

    void setCreakingActive(bool active)
    {
        m_creakingActive = active;
    }

    [[nodiscard]] bool isCreakingActive(const BlockPos& /*pos*/) const override
    {
        return m_creakingActive;
    }

    [[nodiscard]] i32 writeCount() const
    {
        return m_writeCount;
    }

    void resetWrites()
    {
        m_writeCount = 0;
    }

    /**
     * @brief 推进 TickManager 若干刻，使计划刻到期执行
     */
    void runTicks(i32 ticks)
    {
        for (i32 i = 0; i < ticks; ++i) {
            m_tickManager.tick(++m_currentTick);
        }
    }

private:
    world::tick::TickManager m_tickManager;
    std::map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    BlockPos m_lastWrittenPos;
    i32 m_writeCount = 0;
    u64 m_currentTick = 0;
    bool m_creakingActive = false;
};

BlockState freshHeart()
{
    return block_registry::PaleGardenBlocks::CREAKING_HEART->defaultState();
}

} // namespace

class CreakingHeartBlockTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        block_registry::registerPaleGardenBlocks();
        BlockTags::initialize();
        m_world = std::make_unique<CreakingHeartTestWorld>();
    }

    void TearDown() override
    {
        m_world.reset();
    }

    /// 在 pos 放置嘎枝之心（写状态到世界）
    BlockState placeHeart(const BlockPos& pos, Axis axis = Axis::Y)
    {
        BlockState s = freshHeart().with(BlockStateProperties::AXIS(), axis);
        m_world->setBlock(pos, s);
        return s;
    }

    std::unique_ptr<CreakingHeartTestWorld> m_world;
};

// ==================== hasRequiredLogs ====================

TEST_F(CreakingHeartBlockTest, HasRequiredLogs_BothSidesPaleOakLogs)
{
    ASSERT_NE(block_registry::PaleGardenBlocks::PALE_OAK_LOG, nullptr);
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);

    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    ASSERT_EQ(log.get(BlockStateProperties::AXIS()), Axis::Y);
    m_world->setBlock(BlockPos(0, 0, 0), log);
    m_world->setBlock(BlockPos(0, 2, 0), log);

    EXPECT_TRUE(CreakingHeartBlock::hasRequiredLogs(heart, *m_world, pos));
}

TEST_F(CreakingHeartBlockTest, HasRequiredLogs_MissingOneSideFails)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);

    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    m_world->setBlock(BlockPos(0, 0, 0), log);
    // 上方留空

    EXPECT_FALSE(CreakingHeartBlock::hasRequiredLogs(heart, *m_world, pos));
}

TEST_F(CreakingHeartBlockTest, HasRequiredLogs_WrongAxisFails)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);

    // 两端为 X 轴原木，与 Y 轴嘎枝之心不同轴 → 不满足
    const BlockState logX =
        block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState().with(BlockStateProperties::AXIS(), Axis::X);
    m_world->setBlock(BlockPos(0, 0, 0), logX);
    m_world->setBlock(BlockPos(0, 2, 0), logX);

    EXPECT_FALSE(CreakingHeartBlock::hasRequiredLogs(heart, *m_world, pos));
}

TEST_F(CreakingHeartBlockTest, HasRequiredLogs_NonLogBlockFails)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);

    m_world->setBlock(BlockPos(0, 0, 0), VanillaBlocks::STONE->defaultState());
    m_world->setBlock(BlockPos(0, 2, 0), block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState());

    EXPECT_FALSE(CreakingHeartBlock::hasRequiredLogs(heart, *m_world, pos));
}

TEST_F(CreakingHeartBlockTest, HasRequiredLogs_XAxisUsesEastWest)
{
    const BlockPos pos(1, 3, 1);
    const BlockState heart = placeHeart(pos, Axis::X);

    const BlockState logX =
        block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState().with(BlockStateProperties::AXIS(), Axis::X);
    m_world->setBlock(BlockPos(0, 3, 1), logX); // West
    m_world->setBlock(BlockPos(2, 3, 1), logX); // East

    EXPECT_TRUE(CreakingHeartBlock::hasRequiredLogs(heart, *m_world, pos));
}

// ==================== updateState ====================

TEST_F(CreakingHeartBlockTest, UpdateState_UprootedToDormantWhenNotActive)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);
    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    m_world->setBlock(BlockPos(0, 0, 0), log);
    m_world->setBlock(BlockPos(0, 2, 0), log);

    m_world->setCreakingActive(false);
    const BlockState updated = CreakingHeartBlock::updateState(heart, *m_world, pos);

    EXPECT_EQ(
        updated.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Dormant);
}

TEST_F(CreakingHeartBlockTest, UpdateState_UprootedToAwakeWhenActive)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);
    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    m_world->setBlock(BlockPos(0, 0, 0), log);
    m_world->setBlock(BlockPos(0, 2, 0), log);

    m_world->setCreakingActive(true);
    const BlockState updated = CreakingHeartBlock::updateState(heart, *m_world, pos);

    EXPECT_EQ(
        updated.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Awake);
}

TEST_F(CreakingHeartBlockTest, UpdateState_NoLogsStaysUprooted)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);

    m_world->setCreakingActive(true);
    const BlockState updated = CreakingHeartBlock::updateState(heart, *m_world, pos);

    EXPECT_EQ(
        updated.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Uprooted);
}

TEST_F(CreakingHeartBlockTest, UpdateState_AlreadyDormantStaysDormant)
{
    // 非 UPROOTED 状态不再被 updateState 改写（vanilla 仅 UPROOTED 分支生效）
    const BlockPos pos(0, 1, 0);
    BlockState heart = placeHeart(pos, Axis::Y);
    heart = heart.with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Dormant);
    m_world->setBlock(pos, heart);

    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    m_world->setBlock(BlockPos(0, 0, 0), log);
    m_world->setBlock(BlockPos(0, 2, 0), log);

    m_world->setCreakingActive(true); // 即便活跃也不改非 UPROOTED
    const BlockState updated = CreakingHeartBlock::updateState(heart, *m_world, pos);

    EXPECT_EQ(
        updated.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Dormant);
}

// ==================== tick / updatePostPlacement ====================

TEST_F(CreakingHeartBlockTest, Tick_WritesBackWhenStateChanges)
{
    const BlockPos pos(0, 1, 0);
    BlockState heart = placeHeart(pos, Axis::Y);
    const BlockState log = block_registry::PaleGardenBlocks::PALE_OAK_LOG->defaultState();
    m_world->setBlock(BlockPos(0, 0, 0), log);
    m_world->setBlock(BlockPos(0, 2, 0), log);

    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);

    m_world->resetWrites();
    math::JavaLegacyRandom random;
    block->tick(*m_world, pos, heart, random);

    EXPECT_GT(m_world->writeCount(), 0);
    EXPECT_EQ(
        heart.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Dormant);
}

TEST_F(CreakingHeartBlockTest, Tick_NoWriteWhenStateUnchanged)
{
    const BlockPos pos(0, 1, 0);
    BlockState heart = placeHeart(pos, Axis::Y); // 无原木 → updateState 不变
    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);

    m_world->resetWrites();
    math::JavaLegacyRandom random;
    block->tick(*m_world, pos, heart, random);

    EXPECT_EQ(m_world->writeCount(), 0);
}

TEST_F(CreakingHeartBlockTest, UpdatePostPlacement_SchedulesTickAndKeepsState)
{
    const BlockPos pos(0, 1, 0);
    const BlockState heart = placeHeart(pos, Axis::Y);
    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);

    const BlockState result = block->updatePostPlacement(
        heart, Direction::Up, VanillaBlocks::STONE->defaultState(), *m_world, pos, BlockPos(0, 2, 0));

    // updatePostPlacement 本身不改状态（延迟到 tick）
    EXPECT_EQ(result, heart);
}

// ==================== getStateForPlacement ====================

TEST_F(CreakingHeartBlockTest, Placement_WithoutLogsStaysUprooted)
{
    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    // 放置位置 (0,1,0) 上下为空 → 保持 UPROOTED
    m_world->setBlock(BlockPos(0, 1, 0), freshHeart());
    const BlockState state = block->updateState(freshHeart(), *m_world, BlockPos(0, 1, 0));

    EXPECT_EQ(
        state.get(BlockStateProperties::CREAKING_HEART_STATE()), BlockStateProperties::CreakingHeartState::Uprooted);
}

// ==================== 比较器输出 ====================

TEST_F(CreakingHeartBlockTest, Comparator_UprootedZero)
{
    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);
    const BlockPos pos(0, 1, 0);
    const BlockState s = freshHeart(); // 默认 UPROOTED
    EXPECT_EQ(block->getComparatorInputOverride(s, *m_world, pos), 0);
}

TEST_F(CreakingHeartBlockTest, Comparator_StateMapping)
{
    auto* block = dynamic_cast<CreakingHeartBlock*>(block_registry::PaleGardenBlocks::CREAKING_HEART);
    ASSERT_NE(block, nullptr);
    const BlockPos pos(0, 1, 0);

    BlockState s = freshHeart();
    s = s.with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Dormant);
    EXPECT_EQ(block->getComparatorInputOverride(s, *m_world, pos), 1);

    s = s.with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Awake);
    EXPECT_EQ(block->getComparatorInputOverride(s, *m_world, pos), 2);
}
