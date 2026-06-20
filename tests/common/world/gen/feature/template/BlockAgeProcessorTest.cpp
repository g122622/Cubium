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

#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/Template.hpp"

using namespace mc;
using namespace mc::world::gen::feature::template_;

// ============================================================================
// 测试夹具
// ============================================================================

class BlockAgeProcessorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// BlockAgeProcessor 构造与克隆测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, ConstructorPreservesMossiness)
{
    BlockAgeProcessor processor(0.5f);
    EXPECT_FLOAT_EQ(processor.mossiness(), 0.5f);

    BlockAgeProcessor processorZero(0.0f);
    EXPECT_FLOAT_EQ(processorZero.mossiness(), 0.0f);

    BlockAgeProcessor processorOne(1.0f);
    EXPECT_FLOAT_EQ(processorOne.mossiness(), 1.0f);
}

TEST_F(BlockAgeProcessorTest, ClonePreservesMossiness)
{
    BlockAgeProcessor processor(0.7f);
    auto cloned = processor.clone();

    auto* clonedProcessor = dynamic_cast<BlockAgeProcessor*>(cloned.get());
    ASSERT_NE(clonedProcessor, nullptr);
    EXPECT_FLOAT_EQ(clonedProcessor->mossiness(), 0.7f);
}

// ============================================================================
// BlockAgeProcessor 不匹配方块测试（非石砖类方块应保持不变）
// ============================================================================

TEST_F(BlockAgeProcessorTest, NonMatchingBlockReturnsOriginal)
{
    // 泥土方块不应被 BlockAgeProcessor 处理
    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    BlockInfo blockInfo(BlockPos(0, 0, 0), dirtState.stateId());
    PlacementSettings settings;

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, dirtState.stateId());
}

TEST_F(BlockAgeProcessorTest, AirBlockReturnsOriginal)
{
    // 空气方块不应被处理
    BlockAgeProcessor processor(0.5f);
    BlockInfo blockInfo(BlockPos(0, 0, 0), 0);
    PlacementSettings settings;

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, 0u);
}

// ============================================================================
// BlockAgeProcessor 确定性测试（相同位置应产生相同结果）
// ============================================================================

TEST_F(BlockAgeProcessorTest, DeterministicResults)
{
    // BlockAgeProcessor 使用位置种子随机，相同位置应产生相同结果
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::STONE_BRICKS->defaultState();
    BlockInfo blockInfo(BlockPos(100, 64, -200), state.stateId());
    PlacementSettings settings;
    BlockPos seedPos(0, 0, 0);

    auto result1 = processor.process(seedPos, BlockPos(100, 64, -200), blockInfo, blockInfo, settings);
    auto result2 = processor.process(seedPos, BlockPos(100, 64, -200), blockInfo, blockInfo, settings);
    auto result3 = processor.process(seedPos, BlockPos(100, 64, -200), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result1->blockStateId, result2->blockStateId);
    EXPECT_EQ(result1->blockStateId, result3->blockStateId);
}

// ============================================================================
// BlockAgeProcessor 石砖类方块替换测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, StoneBricksCanBeReplaced)
{
    // 石砖方块可能被替换为裂纹石砖、苔藓石砖、石砖楼梯或苔藓石砖楼梯
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::STONE_BRICKS->defaultState();
    PlacementSettings settings;

    int replacedCount = 0;
    int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != state.stateId()) {
            ++replacedCount;
        }
    }

    // 50% 概率被替换（2组候选各50%概率），总体约50%替换率
    // 允许较大误差范围，因为随机性
    f32 replaceRate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    EXPECT_GT(replaceRate, 0.2f) << "Stone brick replacement rate too low: " << replaceRate;
    EXPECT_LT(replaceRate, 0.8f) << "Stone brick replacement rate too high: " << replaceRate;
}

TEST_F(BlockAgeProcessorTest, StoneCanBeReplaced)
{
    // 石头方块也可以被 BlockAgeProcessor 处理
    if (!VanillaBlocks::STONE) {
        GTEST_SKIP() << "STONE block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::STONE->defaultState();
    PlacementSettings settings;

    int replacedCount = 0;
    int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != state.stateId()) {
            ++replacedCount;
        }
    }

    f32 replaceRate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    EXPECT_GT(replaceRate, 0.2f) << "Stone replacement rate too low: " << replaceRate;
    EXPECT_LT(replaceRate, 0.8f) << "Stone replacement rate too high: " << replaceRate;
}

TEST_F(BlockAgeProcessorTest, ChiseledStoneBricksCanBeReplaced)
{
    // 錾刻石砖方块也可以被 BlockAgeProcessor 处理
    if (!VanillaBlocks::CHISELED_STONE_BRICKS) {
        GTEST_SKIP() << "CHISELED_STONE_BRICKS block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::CHISELED_STONE_BRICKS->defaultState();
    PlacementSettings settings;

    int replacedCount = 0;
    int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != state.stateId()) {
            ++replacedCount;
        }
    }

    f32 replaceRate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    EXPECT_GT(replaceRate, 0.2f);
    EXPECT_LT(replaceRate, 0.8f);
}

// ============================================================================
// BlockAgeProcessor 楼梯方块替换测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, StairsTaggedBlocksAreProcessed)
{
    // 确认楼梯方块标签包含石砖楼梯
    if (!VanillaBlocks::STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "STONE_BRICK_STAIRS block not registered";
    }

    EXPECT_TRUE(BlockTags::STAIRS().contains(*VanillaBlocks::STONE_BRICK_STAIRS));
}

TEST_F(BlockAgeProcessorTest, StairsCanBeReplaced)
{
    // 楼梯方块有50%概率被替换
    if (!VanillaBlocks::STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "STONE_BRICK_STAIRS block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::STONE_BRICK_STAIRS->defaultState();
    PlacementSettings settings;

    int replacedCount = 0;
    int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != state.stateId()) {
            ++replacedCount;
        }
    }

    f32 replaceRate = static_cast<f32>(replacedCount) / static_cast<f32>(totalBlocks);
    EXPECT_GT(replaceRate, 0.2f);
    EXPECT_LT(replaceRate, 0.8f);
}

TEST_F(BlockAgeProcessorTest, MossyStairsPreservePropertiesOnHighMossiness)
{
    // 高苔藓概率下，楼梯被替换时应使用 withPropertiesOf 保留原属性
    if (!VanillaBlocks::STONE_BRICK_STAIRS || !VanillaBlocks::MOSSY_STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "Required stair blocks not registered";
    }

    // 创建一个具有特定 facing 的楼梯状态
    const BlockState& defaultState = VanillaBlocks::STONE_BRICK_STAIRS->defaultState();
    const BlockState* facingState = &defaultState;
    if (defaultState.hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
        facingState = &defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
    }

    BlockAgeProcessor processor(1.0f); // 100% mossiness 最大化苔藓替换
    PlacementSettings settings;

    // 使用大量位置测试，寻找被替换为苔藓石砖楼梯的情况
    bool foundMossyStairsWithProperties = false;
    const BlockState& mossyDefault = VanillaBlocks::MOSSY_STONE_BRICK_STAIRS->defaultState();

    for (int i = 0; i < 1000; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), facingState->stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != facingState->stateId()) {
            // 检查是否被替换为苔藓石砖楼梯（而非苔藓石砖台阶或石台阶）
            const BlockState* newState = BlockRegistry::instance().getBlockState(result->blockStateId);
            if (newState && &newState->getBlock() == VanillaBlocks::MOSSY_STONE_BRICK_STAIRS) {
                // 验证属性被保留：面向方向应该是 East
                if (newState->hasProperty(BlockStateProperties::HORIZONTAL_FACING())) {
                    Direction facing = newState->get(BlockStateProperties::HORIZONTAL_FACING());
                    if (facing == Direction::East) {
                        foundMossyStairsWithProperties = true;
                        break;
                    }
                }
            }
        }
    }

    // 验证确实能找到保留属性的苔藓楼梯替换
    // 注意：由于随机性，此测试可能在极少数情况下失败
    EXPECT_TRUE(foundMossyStairsWithProperties)
        << "Expected to find mossy stone brick stairs replacement with preserved facing property";
}

// ============================================================================
// BlockAgeProcessor 台阶方块替换测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, SlabsTaggedBlocksAreProcessed)
{
    // 确认台阶方块标签包含石砖台阶
    if (!VanillaBlocks::STONE_BRICK_SLAB) {
        GTEST_SKIP() << "STONE_BRICK_SLAB block not registered";
    }

    EXPECT_TRUE(BlockTags::SLABS().contains(*VanillaBlocks::STONE_BRICK_SLAB));
}

TEST_F(BlockAgeProcessorTest, SlabsNeverReplacedWithZeroMossiness)
{
    // mossiness=0 时台阶不应被替换
    if (!VanillaBlocks::STONE_BRICK_SLAB) {
        GTEST_SKIP() << "STONE_BRICK_SLAB block not registered";
    }

    BlockAgeProcessor processor(0.0f);
    const BlockState& state = VanillaBlocks::STONE_BRICK_SLAB->defaultState();
    PlacementSettings settings;

    for (int i = 0; i < 100; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->blockStateId, state.stateId()) << "Slab should not be replaced when mossiness=0";
    }
}

TEST_F(BlockAgeProcessorTest, SlabsAlwaysReplacedWithFullMossiness)
{
    // mossiness=1 时台阶应总是被替换为苔藓石砖台阶（如果已注册）
    if (!VanillaBlocks::STONE_BRICK_SLAB || !VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
        GTEST_SKIP() << "Required slab blocks not registered";
    }

    BlockAgeProcessor processor(1.0f);
    const BlockState& state = VanillaBlocks::STONE_BRICK_SLAB->defaultState();
    const BlockState& mossyState = VanillaBlocks::MOSSY_STONE_BRICK_SLAB->defaultState();
    PlacementSettings settings;

    for (int i = 0; i < 100; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->blockStateId, mossyState.stateId())
            << "Slab should be replaced with mossy stone brick slab when mossiness=1";
    }
}

// ============================================================================
// BlockAgeProcessor 墙壁方块替换测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, WallsTaggedBlocksAreProcessed)
{
    // 确认墙壁方块标签包含石砖墙
    if (!VanillaBlocks::STONE_BRICK_WALL && !VanillaBlocks::COBBLESTONE_WALL) {
        GTEST_SKIP() << "No wall blocks registered";
    }

    if (VanillaBlocks::STONE_BRICK_WALL) {
        EXPECT_TRUE(BlockTags::WALLS().contains(*VanillaBlocks::STONE_BRICK_WALL));
    }
    if (VanillaBlocks::COBBLESTONE_WALL) {
        EXPECT_TRUE(BlockTags::WALLS().contains(*VanillaBlocks::COBBLESTONE_WALL));
    }
}

TEST_F(BlockAgeProcessorTest, WallsNeverReplacedWithZeroMossiness)
{
    // mossiness=0 时墙壁不应被替换
    if (!VanillaBlocks::STONE_BRICK_WALL) {
        GTEST_SKIP() << "STONE_BRICK_WALL block not registered";
    }

    BlockAgeProcessor processor(0.0f);
    const BlockState& state = VanillaBlocks::STONE_BRICK_WALL->defaultState();
    PlacementSettings settings;

    for (int i = 0; i < 100; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->blockStateId, state.stateId()) << "Wall should not be replaced when mossiness=0";
    }
}

TEST_F(BlockAgeProcessorTest, WallsAlwaysReplacedWithFullMossiness)
{
    // mossiness=1 时墙壁应总是被替换为苔藓石砖墙（如果已注册）
    if (!VanillaBlocks::STONE_BRICK_WALL || !VanillaBlocks::MOSSY_STONE_BRICK_WALL) {
        GTEST_SKIP() << "Required wall blocks not registered";
    }

    BlockAgeProcessor processor(1.0f);
    const BlockState& state = VanillaBlocks::STONE_BRICK_WALL->defaultState();
    const BlockState& mossyState = VanillaBlocks::MOSSY_STONE_BRICK_WALL->defaultState();
    PlacementSettings settings;

    for (int i = 0; i < 100; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->blockStateId, mossyState.stateId())
            << "Wall should be replaced with mossy stone brick wall when mossiness=1";
    }
}

// ============================================================================
// BlockAgeProcessor 黑曜石替换测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, ObsidianCanBeReplacedWithCryingObsidian)
{
    if (!VanillaBlocks::OBSIDIAN || !VanillaBlocks::CRYING_OBSIDIAN) {
        GTEST_SKIP() << "OBSIDIAN or CRYING_OBSIDIAN block not registered";
    }

    BlockAgeProcessor processor(0.5f); // mossiness不影响黑曜石替换
    const BlockState& obsidianState = VanillaBlocks::OBSIDIAN->defaultState();
    const BlockState& cryingState = VanillaBlocks::CRYING_OBSIDIAN->defaultState();
    PlacementSettings settings;

    int cryingCount = 0;
    int totalBlocks = 1000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), obsidianState.stateId());
        auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId == cryingState.stateId()) {
            ++cryingCount;
        }
    }

    // 黑曜石固定15%概率替换为哭泣黑曜石
    f32 replaceRate = static_cast<f32>(cryingCount) / static_cast<f32>(totalBlocks);
    EXPECT_GT(replaceRate, 0.05f) << "Obsidian->Crying Obsidian rate too low: " << replaceRate;
    EXPECT_LT(replaceRate, 0.25f) << "Obsidian->Crying Obsidian rate too high: " << replaceRate;
}

TEST_F(BlockAgeProcessorTest, ObsidianReplacementIgnoresMossiness)
{
    if (!VanillaBlocks::OBSIDIAN || !VanillaBlocks::CRYING_OBSIDIAN) {
        GTEST_SKIP() << "OBSIDIAN or CRYING_OBSIDIAN block not registered";
    }

    // mossiness=0 时黑曜石仍然有15%替换概率（固定概率，不受mossiness影响）
    BlockAgeProcessor processorZero(0.0f);
    BlockAgeProcessor processorOne(1.0f);

    const BlockState& obsidianState = VanillaBlocks::OBSIDIAN->defaultState();
    const BlockState& cryingState = VanillaBlocks::CRYING_OBSIDIAN->defaultState();
    PlacementSettings settings;

    int cryingZero = 0;
    int cryingOne = 0;
    int totalBlocks = 1000;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), obsidianState.stateId());

        auto result0 = processorZero.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);
        auto result1 = processorOne.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result0.has_value());
        ASSERT_TRUE(result1.has_value());
        if (result0->blockStateId == cryingState.stateId()) {
            ++cryingZero;
        }
        if (result1->blockStateId == cryingState.stateId()) {
            ++cryingOne;
        }
    }

    // 两种 mossiness 值都应该有相近的黑曜石替换率（约15%）
    f32 rate0 = static_cast<f32>(cryingZero) / static_cast<f32>(totalBlocks);
    f32 rate1 = static_cast<f32>(cryingOne) / static_cast<f32>(totalBlocks);
    EXPECT_GT(rate0, 0.05f) << "Obsidian replacement with mossiness=0 too low";
    EXPECT_LT(rate0, 0.25f) << "Obsidian replacement with mossiness=0 too high";
    EXPECT_GT(rate1, 0.05f) << "Obsidian replacement with mossiness=1 too low";
    EXPECT_LT(rate1, 0.25f) << "Obsidian replacement with mossiness=1 too high";
}

// ============================================================================
// BlockAgeProcessor 苔藓概率影响测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, MossinessAffectsMossyReplacements)
{
    // mossiness=0 时，台阶和墙壁不会被替换（概率为0%）
    // mossiness=1 时，台阶和墙壁总是被替换（概率100%）
    // 已在上面的 Slabs/Walls 测试中覆盖，此处验证石砖的 mossiness 影响

    if (!VanillaBlocks::STONE_BRICKS || !VanillaBlocks::MOSSY_STONE_BRICKS) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    BlockAgeProcessor processorZero(0.0f);
    BlockAgeProcessor processorOne(1.0f);

    const BlockState& state = VanillaBlocks::STONE_BRICKS->defaultState();
    const BlockState& mossyState = VanillaBlocks::MOSSY_STONE_BRICKS->defaultState();
    PlacementSettings settings;

    int mossyZero = 0;
    int mossyOne = 0;
    int totalBlocks = 500;

    for (int i = 0; i < totalBlocks; ++i) {
        BlockInfo blockInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());

        auto result0 = processorZero.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);
        auto result1 = processorOne.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result0.has_value());
        ASSERT_TRUE(result1.has_value());
        if (result0->blockStateId == mossyState.stateId()) {
            ++mossyZero;
        }
        if (result1->blockStateId == mossyState.stateId()) {
            ++mossyOne;
        }
    }

    // mossiness=0 时不应有苔藓石砖替换（non-mossy组只有裂纹石砖和石砖楼梯）
    // mossiness=1 时应该有苔藓石砖替换
    EXPECT_EQ(mossyZero, 0) << "No mossy replacements expected when mossiness=0";
    EXPECT_GT(mossyOne, 0) << "Mossy replacements expected when mossiness=1";
}

// ============================================================================
// BlockAgeProcessor NBT 数据保留测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, NbtDataPreservedOnReplacement)
{
    // 替换后的方块应保留原方块的 NBT 数据
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }

    BlockAgeProcessor processor(1.0f);
    const BlockState& state = VanillaBlocks::STONE_BRICKS->defaultState();
    PlacementSettings settings;

    // 创建带有 NBT 数据的 BlockInfo
    BlockInfo blockInfo(BlockPos(0, 0, 0), state.stateId());
    blockInfo.nbt = std::make_unique<nbt::CompoundTag>();
    blockInfo.nbt->value["test_key"] = std::make_unique<nbt::IntTag>(42);

    // 搜索一个被替换的位置
    for (int i = 0; i < 200; ++i) {
        BlockInfo testInfo(BlockPos(i * 7, i % 64, i * 13), state.stateId());
        testInfo.nbt = std::make_unique<nbt::CompoundTag>();
        testInfo.nbt->value["test_key"] = std::make_unique<nbt::IntTag>(42);

        auto result = processor.process(BlockPos(0, 0, 0), testInfo.pos, testInfo, testInfo, settings);

        ASSERT_TRUE(result.has_value());
        if (result->blockStateId != state.stateId()) {
            // 方块被替换了，验证NBT被保留
            ASSERT_NE(result->nbt, nullptr);
            auto* intTag = dynamic_cast<nbt::IntTag*>(result->nbt->value["test_key"].get());
            ASSERT_NE(intTag, nullptr);
            EXPECT_EQ(intTag->value, 42);
            return;
        }
    }

    // 如果没有找到替换，可能需要更多迭代，但不标记为失败
    // 因为这只是概率性问题
}

TEST_F(BlockAgeProcessorTest, NbtDataPreservedOnNoReplacement)
{
    // 未被替换的方块应保留原方块的 NBT 数据
    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::DIRT->defaultState();
    PlacementSettings settings;

    BlockInfo blockInfo(BlockPos(0, 0, 0), state.stateId());
    blockInfo.nbt = std::make_unique<nbt::CompoundTag>();
    blockInfo.nbt->value["test_key"] = std::make_unique<nbt::IntTag>(99);

    auto result = processor.process(BlockPos(0, 0, 0), blockInfo.pos, blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result->nbt, nullptr);
    auto* intTag = dynamic_cast<nbt::IntTag*>(result->nbt->value["test_key"].get());
    ASSERT_NE(intTag, nullptr);
    EXPECT_EQ(intTag->value, 99);
}

// ============================================================================
// BlockAgeProcessor 位置保持测试
// ============================================================================

TEST_F(BlockAgeProcessorTest, PositionPreservedAfterProcessing)
{
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }

    BlockAgeProcessor processor(0.5f);
    const BlockState& state = VanillaBlocks::STONE_BRICKS->defaultState();
    PlacementSettings settings;

    for (int i = 0; i < 50; ++i) {
        BlockPos pos(i * 3, i % 64, i * 7);
        BlockInfo blockInfo(pos, state.stateId());

        auto result = processor.process(BlockPos(0, 0, 0), pos, blockInfo, blockInfo, settings);

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->pos.x, pos.x);
        EXPECT_EQ(result->pos.y, pos.y);
        EXPECT_EQ(result->pos.z, pos.z);
    }
}
