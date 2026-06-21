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

#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/CappedStructureProcessor.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

using namespace mc;
using namespace mc::world::gen::feature::template_;
using namespace mc::world::gen::valueprovider;

// ============================================================================
// 测试夹具
// ============================================================================

class CappedProcessorTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// 替换所有方块的测试处理器
// ============================================================================

/**
 * @brief 将所有方块替换为指定方块状态ID的测试处理器
 */
class ReplaceAllProcessor : public StructureProcessor {
public:
    explicit ReplaceAllProcessor(u32 replacementStateId)
        : m_replacementStateId(replacementStateId)
    {}

    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& /*seedPos*/,
        const BlockPos& /*pos*/,
        const BlockInfo& /*rawBlockInfo*/,
        const BlockInfo& blockInfo,
        const PlacementSettings& /*settings*/) override
    {
        ProcessedBlockInfo result;
        result.pos = blockInfo.pos;
        result.blockStateId = m_replacementStateId;
        if (blockInfo.nbt) {
            result.nbt = std::make_unique<nbt::CompoundTag>(*blockInfo.nbt);
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<ReplaceAllProcessor>(m_replacementStateId);
    }

private:
    u32 m_replacementStateId;
};

/**
 * @brief 不替换任何方块的测试处理器（透传）
 */
class PassthroughProcessor : public StructureProcessor {
public:
    [[nodiscard]] std::optional<ProcessedBlockInfo> process(const BlockPos& /*seedPos*/,
        const BlockPos& /*pos*/,
        const BlockInfo& /*rawBlockInfo*/,
        const BlockInfo& blockInfo,
        const PlacementSettings& /*settings*/) override
    {
        return ProcessedBlockInfo::fromBlockInfo(blockInfo);
    }

    [[nodiscard]] std::unique_ptr<StructureProcessor> clone() const override
    {
        return std::make_unique<PassthroughProcessor>();
    }
};

// ============================================================================
// 辅助函数：构造测试用的方块列表
// ============================================================================

static std::vector<BlockInfo> makeOriginalBlocks(i32 count)
{
    std::vector<BlockInfo> blocks;
    blocks.reserve(count);
    for (i32 i = 0; i < count; ++i) {
        blocks.emplace_back(BlockPos(i, 0, 0), 1);
    }
    return blocks;
}

static std::vector<ProcessedBlockInfo> makeProcessedBlocks(i32 count)
{
    std::vector<ProcessedBlockInfo> blocks;
    blocks.reserve(count);
    for (i32 i = 0; i < count; ++i) {
        blocks.emplace_back(BlockPos(i, 0, 0), 1);
    }
    return blocks;
}

// ============================================================================
// 构造函数测试
// ============================================================================

TEST_F(CappedProcessorTest, Constructor_IntProvider_SetsLimitProvider)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    auto limitProvider = std::make_unique<UniformInt>(2, 6);
    CappedStructureProcessor processor(std::move(delegate), std::move(limitProvider));
    ASSERT_NE(processor.getLimitProvider(), nullptr);
    EXPECT_EQ(processor.getLimitProvider()->getMinValue(), 2);
    EXPECT_EQ(processor.getLimitProvider()->getMaxValue(), 6);
}

TEST_F(CappedProcessorTest, Constructor_FixedInt_SetsConstantProvider)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), 5);
    ASSERT_NE(processor.getLimitProvider(), nullptr);
    // i32 构造函数应创建 ConstantInt(5)
    EXPECT_EQ(processor.getLimitProvider()->getMinValue(), 5);
    EXPECT_EQ(processor.getLimitProvider()->getMaxValue(), 5);
}

TEST_F(CappedProcessorTest, Constructor_NegativeLimitClampedToZero)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), -3);
    ASSERT_NE(processor.getLimitProvider(), nullptr);
    EXPECT_EQ(processor.getLimitProvider()->getMinValue(), 0);
    EXPECT_EQ(processor.getLimitProvider()->getMaxValue(), 0);
}

TEST_F(CappedProcessorTest, Constructor_ZeroLimit)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), 0);
    ASSERT_NE(processor.getLimitProvider(), nullptr);
    EXPECT_EQ(processor.getLimitProvider()->getMinValue(), 0);
    EXPECT_EQ(processor.getLimitProvider()->getMaxValue(), 0);
}

TEST_F(CappedProcessorTest, Constructor_NullLimitProvider_DefaultsToZero)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), std::unique_ptr<IntProvider>(nullptr));
    ASSERT_NE(processor.getLimitProvider(), nullptr);
    EXPECT_EQ(processor.getLimitProvider()->getMinValue(), 0);
    EXPECT_EQ(processor.getLimitProvider()->getMaxValue(), 0);
}

// ============================================================================
// clone 测试
// ============================================================================

TEST_F(CappedProcessorTest, Clone_ProducesEqualCopy)
{
    auto delegate = std::make_unique<ReplaceAllProcessor>(42);
    CappedStructureProcessor original(std::move(delegate), 7);

    auto cloned = original.clone();
    auto* clonedCapped = dynamic_cast<CappedStructureProcessor*>(cloned.get());
    ASSERT_NE(clonedCapped, nullptr);
    ASSERT_NE(clonedCapped->getLimitProvider(), nullptr);
    EXPECT_EQ(clonedCapped->getLimitProvider()->getMinValue(), 7);
    EXPECT_EQ(clonedCapped->getLimitProvider()->getMaxValue(), 7);
    EXPECT_NE(clonedCapped->getDelegate(), nullptr);
}

TEST_F(CappedProcessorTest, Clone_NullDelegate)
{
    CappedStructureProcessor original(nullptr, 3);
    auto cloned = original.clone();
    auto* clonedCapped = dynamic_cast<CappedStructureProcessor*>(cloned.get());
    ASSERT_NE(clonedCapped, nullptr);
    ASSERT_NE(clonedCapped->getLimitProvider(), nullptr);
    EXPECT_EQ(clonedCapped->getLimitProvider()->getMinValue(), 3);
    EXPECT_EQ(clonedCapped->getDelegate(), nullptr);
}

TEST_F(CappedProcessorTest, Clone_WithUniformIntProvider)
{
    auto delegate = std::make_unique<ReplaceAllProcessor>(42);
    auto limitProvider = std::make_unique<UniformInt>(2, 8);
    CappedStructureProcessor original(std::move(delegate), std::move(limitProvider));

    auto cloned = original.clone();
    auto* clonedCapped = dynamic_cast<CappedStructureProcessor*>(cloned.get());
    ASSERT_NE(clonedCapped, nullptr);
    ASSERT_NE(clonedCapped->getLimitProvider(), nullptr);
    EXPECT_EQ(clonedCapped->getLimitProvider()->getMinValue(), 2);
    EXPECT_EQ(clonedCapped->getLimitProvider()->getMaxValue(), 8);
}

// ============================================================================
// process 阶段测试（应直接透传）
// ============================================================================

TEST_F(CappedProcessorTest, Process_PassesThroughWithoutModification)
{
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 5);

    PlacementSettings settings;
    BlockInfo rawInfo(BlockPos(0, 0, 0), 1);
    BlockInfo blockInfo(BlockPos(10, 20, 30), 1);

    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), rawInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    // process 阶段应该直接透传，不调用 delegate
    EXPECT_EQ(result->blockStateId, 1u);
    EXPECT_EQ(result->pos.x, 10);
}

// ============================================================================
// finalizeProcessing 测试
// ============================================================================

TEST_F(CappedProcessorTest, FinalizeProcessing_LimitZero_NoReplacements)
{
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 0);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);
    for (const auto& block : result) {
        // limit=0（ConstantInt(0) 的 maxValue=0），不应替换任何方块
        EXPECT_EQ(block.blockStateId, 1u);
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_EmptyList_ReturnsEmpty)
{
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 5);

    PlacementSettings settings;
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    EXPECT_TRUE(result.empty());
}

TEST_F(CappedProcessorTest, FinalizeProcessing_LimitReplacesOnlyUpToLimit)
{
    // 创建一个将所有方块替换为 stateId=99 的处理器，限制替换 3 次
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 3);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);

    // 统计替换后的方块数量（blockStateId == 99 表示被替换）
    i32 replacedCount = 0;
    for (const auto& block : result) {
        if (block.blockStateId == 99u) {
            ++replacedCount;
        }
    }

    // 替换次数应恰好等于 limit
    EXPECT_EQ(replacedCount, 3);
}

TEST_F(CappedProcessorTest, FinalizeProcessing_LimitExceedsBlockCount)
{
    // limit 大于方块数量，应替换所有方块
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 100);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(5);
    auto processedBlocks = makeProcessedBlocks(5);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 5u);

    // 所有方块都应被替换
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 99u);
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_PassthroughDelegate_NoChanges)
{
    // 使用透传处理器，delegate 不改变方块，因此替换计数应为 0
    auto delegate = std::make_unique<PassthroughProcessor>();
    CappedStructureProcessor processor(std::move(delegate), 5);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);

    // 所有方块应保持不变
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 1u);
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_DeterministicRandom)
{
    // 相同的 seedPos 应产生相同的替换结果
    auto createProcessor = []() -> std::unique_ptr<CappedStructureProcessor> {
        return std::make_unique<CappedStructureProcessor>(std::make_unique<ReplaceAllProcessor>(99), 3);
    };

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(20);

    auto makeBlocks = []() -> std::vector<ProcessedBlockInfo> { return makeProcessedBlocks(20); };

    auto processor1 = createProcessor();
    auto result1 =
        processor1->finalizeProcessing(BlockPos(100, 0, 200), settings, originalBlocks, makeBlocks());

    auto processor2 = createProcessor();
    auto result2 =
        processor2->finalizeProcessing(BlockPos(100, 0, 200), settings, originalBlocks, makeBlocks());

    ASSERT_EQ(result1.size(), result2.size());
    for (size_t i = 0; i < result1.size(); ++i) {
        EXPECT_EQ(result1[i].blockStateId, result2[i].blockStateId)
            << "Mismatch at block " << i << ": " << result1[i].blockStateId << " vs " << result2[i].blockStateId;
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_DifferentSeedsProduceDifferentResults)
{
    // 不同的 seedPos 可能产生不同的替换结果（概率性的）
    auto createProcessor = []() -> std::unique_ptr<CappedStructureProcessor> {
        return std::make_unique<CappedStructureProcessor>(std::make_unique<ReplaceAllProcessor>(99), 5);
    };

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(50);

    auto makeBlocks = []() -> std::vector<ProcessedBlockInfo> { return makeProcessedBlocks(50); };

    auto processor1 = createProcessor();
    auto result1 = processor1->finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, makeBlocks());

    auto processor2 = createProcessor();
    auto result2 =
        processor2->finalizeProcessing(BlockPos(999, 0, 999), settings, originalBlocks, makeBlocks());

    // 统计两个结果中替换的位置是否不同
    int samePositionCount = 0;
    for (size_t i = 0; i < result1.size(); ++i) {
        if (result1[i].blockStateId == 99u && result2[i].blockStateId == 99u) {
            ++samePositionCount;
        }
    }

    // 两个不同种子产生完全相同替换位置的概率极低
    EXPECT_LT(samePositionCount, 5);
}

TEST_F(CappedProcessorTest, FinalizeProcessing_SizeMismatch_ReturnsUnmodified)
{
    // 当原始列表和处理后列表大小不一致时，应返回未修改的列表
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 5);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    // 只有 5 个处理后方块（大小不匹配）
    auto processedBlocks = makeProcessedBlocks(5);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    // 应返回未修改的列表
    ASSERT_EQ(result.size(), 5u);
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 1u);
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_NullDelegate_NoCrash)
{
    // 当 delegate 为 nullptr 时，finalizeProcessing 不应崩溃，直接返回未修改的列表
    CappedStructureProcessor processor(nullptr, 5);

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 1u);
    }
}

// ============================================================================
// IntProvider 限制测试
// ============================================================================

TEST_F(CappedProcessorTest, FinalizeProcessing_UniformIntProvider_SamplesLimit)
{
    // 使用 UniformInt(3, 3) 等价于 ConstantInt(3)，替换次数应为 3
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    auto limitProvider = std::make_unique<UniformInt>(3, 3);
    CappedStructureProcessor processor(std::move(delegate), std::move(limitProvider));

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);

    i32 replacedCount = 0;
    for (const auto& block : result) {
        if (block.blockStateId == 99u) {
            ++replacedCount;
        }
    }

    // UniformInt(3, 3) 始终采样为 3
    EXPECT_EQ(replacedCount, 3);
}

TEST_F(CappedProcessorTest, FinalizeProcessing_ProviderMaxValueZero_NoReplacements)
{
    // IntProvider 最大值为 0 时，不应替换任何方块
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    auto limitProvider = std::make_unique<ConstantInt>(0);
    CappedStructureProcessor processor(std::move(delegate), std::move(limitProvider));

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 1u);
    }
}

TEST_F(CappedProcessorTest, FinalizeProcessing_BiasedToBottomIntProvider)
{
    // 使用 BiasedToBottomInt(5, 5) 等价于 ConstantInt(5)
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    auto limitProvider = std::make_unique<BiasedToBottomInt>(5, 5);
    CappedStructureProcessor processor(std::move(delegate), std::move(limitProvider));

    PlacementSettings settings;
    auto originalBlocks = makeOriginalBlocks(10);
    auto processedBlocks = makeProcessedBlocks(10);

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);

    i32 replacedCount = 0;
    for (const auto& block : result) {
        if (block.blockStateId == 99u) {
            ++replacedCount;
        }
    }

    EXPECT_EQ(replacedCount, 5);
}
