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

// 测试 BlockPattern 方块模式匹配系统（BlockInWorld / BlockPatternBuilder / BlockPattern）。
//
// 覆盖：
//   1. 单方块模式匹配
//   2. 2D 平面模式匹配（如十字形）
//   3. 3D 立体模式匹配（如 EndDragonFight 的 exitPortalPattern）
//   4. 模式未找到时返回 nullopt
//   5. BlockPatternBuilder 参数校验（高度/宽度不一致）
//   6. BlockInWorld 延迟加载与缓存

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/pattern/BlockInWorld.hpp"
#include "common/world/block/pattern/BlockPattern.hpp"
#include "common/world/block/pattern/BlockPatternBuilder.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using namespace mc;
using namespace mc::blockpattern;

namespace {

/// 带区块存储的测试世界，用于 BlockPattern 测试
///
/// 记录 getBlockState 的调用次数与去重位置集合，供"find 共享缓存"测试断言
/// 每个世界坐标在一次 find 中最多被查询一次（共享缓存语义）。
class PatternTestWorld : public mc::test::BaseChunkBackedTestWorld {
public:
    mutable size_t getBlockStateCallCount = 0;
    mutable std::unordered_set<mc::BlockPos> queriedPositions;

    void resetQueryStats() const
    {
        getBlockStateCallCount = 0;
        queriedPositions.clear();
    }

    [[nodiscard]] const mc::BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        ++getBlockStateCallCount;
        queriedPositions.insert(mc::BlockPos(x, y, z));

        const mc::ChunkData* chunk =
            chunkManager()->tryToGetChunkInMem(mc::math::toChunkCoord(x), mc::math::toChunkCoord(z));
        if (chunk == nullptr) {
            return nullptr;
        }
        return chunk->getBlockState(
            mc::math::toLocalCoord(x), y - mc::world::MIN_BUILD_HEIGHT, mc::math::toLocalCoord(z));
    }

    bool setBlockState(i32 x, i32 y, i32 z, const mc::BlockState* state) override
    {
        mc::ChunkData& chunk = ensureChunk(mc::math::toChunkCoord(x), mc::math::toChunkCoord(z));
        chunk.setBlockState(
            mc::math::toLocalCoord(x), y - mc::world::MIN_BUILD_HEIGHT, mc::math::toLocalCoord(z), state);
        return true;
    }
};

} // namespace

// ============================================================================
// BlockPatternBuilder 测试
// ============================================================================

class BlockPatternBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

TEST_F(BlockPatternBuilderTest, Build_SingleBlockPattern_HasCorrectDimensions)
{
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"#"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    EXPECT_EQ(pattern->depth(), 1);
    EXPECT_EQ(pattern->height(), 1);
    EXPECT_EQ(pattern->width(), 1);
}

TEST_F(BlockPatternBuilderTest, Build_3x3Pattern_HasCorrectDimensions)
{
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"###", "###", "###"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    EXPECT_EQ(pattern->depth(), 1);
    EXPECT_EQ(pattern->height(), 3);
    EXPECT_EQ(pattern->width(), 3);
}

TEST_F(BlockPatternBuilderTest, Build_MultipleAisles_HasCorrectDepth)
{
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"#"})
                       .aisle({"#"})
                       .aisle({"#"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    EXPECT_EQ(pattern->depth(), 3);
    EXPECT_EQ(pattern->height(), 1);
    EXPECT_EQ(pattern->width(), 1);
}

TEST_F(BlockPatternBuilderTest, Aisle_InconsistentHeight_Asserts)
{
    EXPECT_DEATH({ BlockPatternBuilder::start().aisle({"###", "###"}).aisle({"###"}).build(); }, "BlockPatternBuilder");
}

TEST_F(BlockPatternBuilderTest, Aisle_InconsistentWidth_Asserts)
{
    EXPECT_DEATH({ BlockPatternBuilder::start().aisle({"###", "##"}).build(); }, "BlockPatternBuilder");
}

TEST_F(BlockPatternBuilderTest, Build_MissingPredicate_Asserts)
{
    EXPECT_DEATH({ BlockPatternBuilder::start().aisle({"#"}).build(); }, "BlockPatternBuilder");
}

TEST_F(BlockPatternBuilderTest, Build_EmptyAisle_Asserts)
{
    EXPECT_DEATH({ BlockPatternBuilder::start().aisle({""}).build(); }, "BlockPatternBuilder");
}

// ============================================================================
// BlockPattern::find 测试
// ============================================================================

class BlockPatternFindTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    PatternTestWorld m_world;
};

TEST_F(BlockPatternFindTest, Find_SingleBedrockAtOrigin_ReturnsMatch)
{
    // 在原点放置一个基岩
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrockState, nullptr);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto pattern = BlockPatternBuilder::start()
                       .aisle({"#"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->width(), 1);
    EXPECT_EQ(match->height(), 1);
    EXPECT_EQ(match->depth(), 1);
}

TEST_F(BlockPatternFindTest, Find_NoMatchingBlock_ReturnsNullopt)
{
    // 世界中只有空气，没有基岩
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"#"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    EXPECT_FALSE(match.has_value());
}

TEST_F(BlockPatternFindTest, Find_CrossPattern_ReturnsCenter)
{
    // 构建一个 3x3 十字模式（中间和四方向为基岩，四角为任意）
    //   " # "
    //   "###"
    //   " # "
    auto pattern = BlockPatternBuilder::start()
                       .aisle({" # ", "###", " # "})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    // 在世界中构建对应的十字
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(1, 64, 0, bedrockState);  // 上
    m_world.setBlockState(0, 64, 0, bedrockState);  // 中
    m_world.setBlockState(-1, 64, 0, bedrockState); // 下（实际为西）
    m_world.setBlockState(0, 64, 1, bedrockState);  // 东
    m_world.setBlockState(0, 64, -1, bedrockState); // 北

    // 搜索起始位置选择十字中心附近
    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    ASSERT_TRUE(match.has_value());
}

TEST_F(BlockPatternFindTest, Find_ExitPortalPattern_ReturnsCenter)
{
    // 构建 EndDragonFight 的 exitPortalPattern 并在世界中放置对应讲台结构
    // 讲台中心放在 (0, 64, 0)
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
                       .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
                       .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
                       .aisle({"  ###  ", " #   # ", "#     #", "#  #  #", "#     #", " #   # ", "  ###  "})
                       .aisle({"       ", "  ###  ", " ##### ", " ##### ", " ##### ", "  ###  ", "       "})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    // 在世界中原点构建讲台
    // 使用 EndTeleporter::createExitPortal 创建激活态讲台
    EndTeleporter::createExitPortal(m_world, BlockPos(0, 64, 0), true);

    // 通过模式匹配查找讲台
    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    ASSERT_TRUE(match.has_value());

    // getBlock(3, 3, 3) 应返回讲台中心位置（基岩柱顶端）
    BlockInWorld center = match->getBlock(3, 3, 3);
    const BlockState* centerState = center.getState();
    ASSERT_NE(centerState, nullptr);
    EXPECT_TRUE(centerState->is(VanillaBlocks::BEDROCK));
}

// ============================================================================
// BlockInWorld 测试
// ============================================================================

class BlockInWorldTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }

    PatternTestWorld m_world;
};

TEST_F(BlockInWorldTest, GetState_LoadedChunk_ReturnsState)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(0, 64, 0, bedrockState);

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    const BlockState* state = blockInWorld.getState();
    EXPECT_EQ(state, bedrockState);
}

TEST_F(BlockInWorldTest, GetState_UnloadedChunk_ReturnsNullptr)
{
    // 使用未加载的区块坐标（确保没有 ensureChunk）
    BlockInWorld blockInWorld(m_world, BlockPos(10000, 64, 10000), false);
    const BlockState* state = blockInWorld.getState();
    EXPECT_EQ(state, nullptr);
}

TEST_F(BlockInWorldTest, GetState_CachedOnSecondCall)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(0, 64, 0, bedrockState);

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    const BlockState* state1 = blockInWorld.getState();
    const BlockState* state2 = blockInWorld.getState();
    EXPECT_EQ(state1, state2);
}

TEST_F(BlockInWorldTest, HasState_PredicateMatches_ReturnsTrue)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto predicate = BlockInWorld::hasState([](const BlockState& s) {
        return s.is(VanillaBlocks::BEDROCK);
    });

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    EXPECT_TRUE(predicate(blockInWorld));
}

TEST_F(BlockInWorldTest, HasState_PredicateDoesNotMatch_ReturnsFalse)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto predicate = BlockInWorld::hasState([](const BlockState& s) {
        return s.is(VanillaBlocks::END_STONE);
    });

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    EXPECT_FALSE(predicate(blockInWorld));
}

TEST_F(BlockInWorldTest, HasState_NullState_ReturnsFalse)
{
    // 未加载区块，getState() 返回 nullptr
    auto predicate = BlockInWorld::hasState([](const BlockState& s) {
        return s.is(VanillaBlocks::BEDROCK);
    });

    BlockInWorld blockInWorld(m_world, BlockPos(10000, 64, 10000), false);
    EXPECT_FALSE(predicate(blockInWorld));
}

// ============================================================================
// 共享缓存语义：find() 全程共享一个位置→BlockInWorld 缓存
// ============================================================================

TEST_F(BlockPatternFindTest, Find_SharedCache_QueriesEachPositionAtMostOnce)
{
    // 放一个泥土，使十字模式（要求基岩）不匹配；find 会穷举候选位置与 24 种朝向。
    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);
    ASSERT_NE(dirtState, nullptr);
    m_world.setBlockState(0, 64, 0, dirtState);

    auto pattern = BlockPatternBuilder::start()
                       .aisle({" # ", "###", " # "})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    m_world.resetQueryStats();
    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    EXPECT_FALSE(match.has_value());

    // find() 在开头创建一个缓存并全程共享：
    // 同一世界坐标在一次 find 中只查询一次世界。
    EXPECT_GT(m_world.getBlockStateCallCount, 0u);
    EXPECT_EQ(m_world.getBlockStateCallCount, m_world.queriedPositions.size());
}

TEST_F(BlockPatternFindTest, MatchGetBlock_ReturnsMatchTimeSnapshot)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* endStoneState = VanillaBlocks::getState(VanillaBlocks::END_STONE);
    ASSERT_NE(bedrockState, nullptr);
    ASSERT_NE(endStoneState, nullptr);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto pattern = BlockPatternBuilder::start()
                       .aisle({"#"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    ASSERT_TRUE(match.has_value());

    // 匹配后修改世界：getBlock 必须返回匹配时的状态（匹配结果持有匹配时缓存）。
    m_world.setBlockState(0, 64, 0, endStoneState);

    const BlockInWorld& cached = match->getBlock(0, 0, 0);
    ASSERT_NE(cached.getState(), nullptr);
    EXPECT_TRUE(cached.getState()->is(VanillaBlocks::BEDROCK));

    const BlockState* current = m_world.getBlockState(0, 64, 0);
    ASSERT_NE(current, nullptr);
    EXPECT_TRUE(current->is(VanillaBlocks::END_STONE));
}

TEST_F(BlockPatternFindTest, Find_CandidateOrder_XFastestThenYThenZ)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrockState, nullptr);

    // 用 2 格宽模式（maxDim=2）制造两个可区分的匹配位置：
    //   x 方向：(1,64,0)-(2,64,0)
    //   y 方向：(0,65,0)-(1,65,0)
    // 候选顺序 x 最快 → 先命中 (1,64,0)；
    // 若按 z 最快遍历，则会先命中 (0,65,0)。
    m_world.setBlockState(1, 64, 0, bedrockState);
    m_world.setBlockState(2, 64, 0, bedrockState);
    m_world.setBlockState(0, 65, 0, bedrockState);
    m_world.setBlockState(1, 65, 0, bedrockState);

    auto pattern = BlockPatternBuilder::start()
                       .aisle({"##"})
                       .where('#', BlockInWorld::hasState([](const BlockState& s) {
                           return s.is(VanillaBlocks::BEDROCK);
                       }))
                       .build();

    auto match = pattern->find(m_world, BlockPos(0, 64, 0));
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->frontTopLeft(), BlockPos(1, 64, 0));
}

TEST_F(BlockPatternFindTest, TranslateAndRotate_OriginIsIdentityForAllOrthogonalPairs)
{
    const BlockPos origin(10, 20, 30);
    for (Direction forwards : Directions::all()) {
        for (Direction up : Directions::all()) {
            if (up == forwards || up == Directions::opposite(forwards)) {
                continue;
            }
            EXPECT_EQ(BlockPattern::translateAndRotate(origin, forwards, up, 0, 0, 0), origin);
        }
    }
}

TEST_F(BlockPatternFindTest, TranslateAndRotate_FollowsExpectedAxes)
{
    const BlockPos origin(0, 64, 0);

    // forwards=South(0,0,1)、up=Up(0,1,0)：
    // width 沿 up × forwards = (-1,0,0)，height 沿 -up，depth 沿 forwards。
    EXPECT_EQ(BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 1, 0, 0), BlockPos(-1, 64, 0));
    EXPECT_EQ(BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 0, 1, 0), BlockPos(0, 63, 0));
    EXPECT_EQ(BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 0, 0, 1), BlockPos(0, 64, 1));
    EXPECT_EQ(BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 1, 1, 1), BlockPos(-1, 63, 1));
}

TEST_F(BlockPatternFindTest, Matches_EvaluatesCellsInWidthHeightDepthOrder)
{
    std::vector<BlockPos> evaluationOrder;
    auto pattern = BlockPatternBuilder::start()
                       .aisle({"###", "###", "###"})
                       .where('#',
                           [&evaluationOrder](const BlockInWorld& block) {
                               evaluationOrder.push_back(block.pos());
                               return true;
                           })
                       .build();

    const BlockPos origin(0, 64, 0);
    auto match = pattern->matches(m_world, origin, Direction::South, Direction::Up);
    ASSERT_TRUE(match.has_value());
    ASSERT_EQ(evaluationOrder.size(), 9u);

    // matches 遍历顺序：width(i) 外层、height(j) 中层、depth(k) 内层。
    EXPECT_EQ(evaluationOrder[0], BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 0, 0, 0));
    EXPECT_EQ(evaluationOrder[1], BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 0, 1, 0));
    EXPECT_EQ(evaluationOrder[2], BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 0, 2, 0));
    EXPECT_EQ(evaluationOrder[3], BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 1, 0, 0));
    EXPECT_EQ(evaluationOrder[6], BlockPattern::translateAndRotate(origin, Direction::South, Direction::Up, 2, 0, 0));
}

TEST_F(BlockInWorldTest, GetState_AirInLoadedChunk_ReturnsAirState)
{
    // 先创建区块，再把目标位置写成空气（模拟已加载区块内的空气），
    // 此时 getState() 返回非 null 的空气 BlockState。
    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);
    const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
    ASSERT_NE(dirtState, nullptr);
    ASSERT_NE(airState, nullptr);
    m_world.setBlockState(0, 64, 0, dirtState);
    m_world.setBlockState(0, 64, 0, airState);

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    const BlockState* state = blockInWorld.getState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());

    auto alwaysTrue = BlockInWorld::hasState([](const BlockState&) {
        return true;
    });
    EXPECT_TRUE(alwaysTrue(blockInWorld));
}

TEST_F(BlockInWorldTest, GetState_UnloadedChunk_RetriesAfterChunkLoads)
{
    const BlockPos pos(10000, 64, 10000);
    BlockInWorld blockInWorld(m_world, pos, false);
    EXPECT_EQ(blockInWorld.getState(), nullptr);

    // 未加载时不缓存 null，区块随后加载后同一 BlockInWorld 仍能读到状态。
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrockState, nullptr);
    m_world.setBlockState(pos.x, pos.y, pos.z, bedrockState);

    const BlockState* state = blockInWorld.getState();
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::BEDROCK));
}
