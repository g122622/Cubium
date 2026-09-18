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
// 对应 MC 1.21.11 net.minecraft.world.level.block.state.pattern.BlockPattern 系列。
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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/block/state/pattern/BlockInWorld.hpp"
#include "common/world/block/state/pattern/BlockPattern.hpp"
#include "common/world/block/state/pattern/BlockPatternBuilder.hpp"
#include "common/world/dimension/teleport/Teleporter.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mc;
using namespace mc::blockpattern;

namespace {

/// 带区块存储的测试世界，用于 BlockPattern 测试
class PatternTestWorld : public mc::test::BaseChunkBackedTestWorld {
public:
    [[nodiscard]] const mc::BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const mc::ChunkData* chunk = getChunk(mc::math::toChunkCoord(x), mc::math::toChunkCoord(z));
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
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockPatternBuilderTest, Build_SingleBlockPattern_HasCorrectDimensions)
{
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"#"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
            .build();

    EXPECT_EQ(pattern->depth(), 1);
    EXPECT_EQ(pattern->height(), 1);
    EXPECT_EQ(pattern->width(), 1);
}

TEST_F(BlockPatternBuilderTest, Build_3x3Pattern_HasCorrectDimensions)
{
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"###", "###", "###"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
            .build();

    EXPECT_EQ(pattern->depth(), 1);
    EXPECT_EQ(pattern->height(), 3);
    EXPECT_EQ(pattern->width(), 3);
}

TEST_F(BlockPatternBuilderTest, Build_MultipleAisles_HasCorrectDepth)
{
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"#"})
            .aisle({"#"})
            .aisle({"#"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
            .build();

    EXPECT_EQ(pattern->depth(), 3);
    EXPECT_EQ(pattern->height(), 1);
    EXPECT_EQ(pattern->width(), 1);
}

TEST_F(BlockPatternBuilderTest, Aisle_InconsistentHeight_ThrowsException)
{
    EXPECT_THROW({ BlockPatternBuilder::start().aisle({"###", "###"}).aisle({"###"}).build(); }, std::invalid_argument);
}

TEST_F(BlockPatternBuilderTest, Aisle_InconsistentWidth_ThrowsException)
{
    EXPECT_THROW({ BlockPatternBuilder::start().aisle({"###", "##"}).build(); }, std::invalid_argument);
}

TEST_F(BlockPatternBuilderTest, Build_MissingPredicate_ThrowsException)
{
    EXPECT_THROW({ BlockPatternBuilder::start().aisle({"#"}).build(); }, std::runtime_error);
}

TEST_F(BlockPatternBuilderTest, Build_EmptyAisle_ThrowsException)
{
    EXPECT_THROW({ BlockPatternBuilder::start().aisle({""}).build(); }, std::invalid_argument);
}

// ============================================================================
// BlockPattern::find 测试
// ============================================================================

class BlockPatternFindTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    PatternTestWorld m_world;
};

TEST_F(BlockPatternFindTest, Find_SingleBedrockAtOrigin_ReturnsMatch)
{
    // 在原点放置一个基岩
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrockState, nullptr);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"#"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
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
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"#"})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
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
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({" # ", "###", " # "})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
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
    auto pattern =
        BlockPatternBuilder::start()
            .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
            .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
            .aisle({"       ", "       ", "       ", "   #   ", "       ", "       ", "       "})
            .aisle({"  ###  ", " #   # ", "#     #", "#  #  #", "#     #", " #   # ", "  ###  "})
            .aisle({"       ", "  ###  ", " ##### ", " ##### ", " ##### ", "  ###  ", "       "})
            .where('#', BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); }))
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
    void SetUp() override { VanillaBlocks::initialize(); }

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

    auto predicate = BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); });

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    EXPECT_TRUE(predicate(blockInWorld));
}

TEST_F(BlockInWorldTest, HasState_PredicateDoesNotMatch_ReturnsFalse)
{
    const BlockState* bedrockState = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    m_world.setBlockState(0, 64, 0, bedrockState);

    auto predicate = BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::END_STONE); });

    BlockInWorld blockInWorld(m_world, BlockPos(0, 64, 0), false);
    EXPECT_FALSE(predicate(blockInWorld));
}

TEST_F(BlockInWorldTest, HasState_NullState_ReturnsFalse)
{
    // 未加载区块，getState() 返回 nullptr
    auto predicate = BlockInWorld::hasState([](const BlockState& s) { return s.is(VanillaBlocks::BEDROCK); });

    BlockInWorld blockInWorld(m_world, BlockPos(10000, 64, 10000), false);
    EXPECT_FALSE(predicate(blockInWorld));
}
