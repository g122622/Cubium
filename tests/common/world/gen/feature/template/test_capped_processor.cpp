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

using namespace mc;
using namespace mc::world::gen::feature::template_;

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
// 构造函数测试
// ============================================================================

TEST_F(CappedProcessorTest, Constructor_SetsLimit)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), 5);
    EXPECT_EQ(processor.getLimit(), 5);
}

TEST_F(CappedProcessorTest, Constructor_NegativeLimitClampedToZero)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), -3);
    EXPECT_EQ(processor.getLimit(), 0);
}

TEST_F(CappedProcessorTest, Constructor_ZeroLimit)
{
    auto delegate = std::make_unique<NopStructureProcessor>();
    CappedStructureProcessor processor(std::move(delegate), 0);
    EXPECT_EQ(processor.getLimit(), 0);
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
    EXPECT_EQ(clonedCapped->getLimit(), 7);
    EXPECT_NE(clonedCapped->getDelegate(), nullptr);
}

TEST_F(CappedProcessorTest, Clone_NullDelegate)
{
    CappedStructureProcessor original(nullptr, 3);
    auto cloned = original.clone();
    auto* clonedCapped = dynamic_cast<CappedStructureProcessor*>(cloned.get());
    ASSERT_NE(clonedCapped, nullptr);
    EXPECT_EQ(clonedCapped->getLimit(), 3);
    EXPECT_EQ(clonedCapped->getDelegate(), nullptr);
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
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 10; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);
    for (const auto& block : result) {
        // limit=0，不应替换任何方块
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
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 10; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

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
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 5; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

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
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 10; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

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
    std::vector<BlockInfo> originalBlocks;
    for (int i = 0; i < 20; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

    // 运行两次，使用相同的 seedPos
    auto makeProcessedBlocks = [&originalBlocks]() -> std::vector<ProcessedBlockInfo> {
        std::vector<ProcessedBlockInfo> blocks;
        for (int i = 0; i < 20; ++i) {
            blocks.emplace_back(BlockPos(i, 0, 0), 1);
        }
        return blocks;
    };

    auto processor1 = createProcessor();
    auto result1 =
        processor1->finalizeProcessing(BlockPos(100, 0, 200), settings, originalBlocks, makeProcessedBlocks());

    auto processor2 = createProcessor();
    auto result2 =
        processor2->finalizeProcessing(BlockPos(100, 0, 200), settings, originalBlocks, makeProcessedBlocks());

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
    std::vector<BlockInfo> originalBlocks;
    for (int i = 0; i < 50; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

    auto makeProcessedBlocks = [&originalBlocks]() -> std::vector<ProcessedBlockInfo> {
        std::vector<ProcessedBlockInfo> blocks;
        for (int i = 0; i < 50; ++i) {
            blocks.emplace_back(BlockPos(i, 0, 0), 1);
        }
        return blocks;
    };

    auto processor1 = createProcessor();
    auto result1 = processor1->finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, makeProcessedBlocks());

    auto processor2 = createProcessor();
    auto result2 =
        processor2->finalizeProcessing(BlockPos(999, 0, 999), settings, originalBlocks, makeProcessedBlocks());

    // 统计两个结果中替换的位置是否不同
    // （由于随机性，大多数情况下不同的种子会产生不同的替换位置）
    int samePositionCount = 0;
    for (size_t i = 0; i < result1.size(); ++i) {
        if (result1[i].blockStateId == 99u && result2[i].blockStateId == 99u) {
            ++samePositionCount;
        }
    }

    // 两个不同种子产生完全相同替换位置的概率极低
    // 如果都替换5个方块，完全重合的概率约为 C(50,5)/C(50,5)*C(5,5)/C(50,5) ≈ 极小
    // 因此至少有一些位置的替换应该不同
    // 注意：这不是严格的数学测试，但足以验证随机性在工作
    EXPECT_LT(samePositionCount, 5);
}

TEST_F(CappedProcessorTest, FinalizeProcessing_SizeMismatch_ReturnsUnmodified)
{
    // 当原始列表和处理后列表大小不一致时，应返回未修改的列表
    auto delegate = std::make_unique<ReplaceAllProcessor>(99);
    CappedStructureProcessor processor(std::move(delegate), 5);

    PlacementSettings settings;
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 10; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }
    // 只有 5 个处理后方块（大小不匹配）
    for (int i = 0; i < 5; ++i) {
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

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
    std::vector<BlockInfo> originalBlocks;
    std::vector<ProcessedBlockInfo> processedBlocks;

    for (int i = 0; i < 10; ++i) {
        originalBlocks.emplace_back(BlockPos(i, 0, 0), 1);
        processedBlocks.emplace_back(BlockPos(i, 0, 0), 1);
    }

    auto result = processor.finalizeProcessing(BlockPos(0, 0, 0), settings, originalBlocks, std::move(processedBlocks));

    ASSERT_EQ(result.size(), 10u);
    for (const auto& block : result) {
        EXPECT_EQ(block.blockStateId, 1u);
    }
}
