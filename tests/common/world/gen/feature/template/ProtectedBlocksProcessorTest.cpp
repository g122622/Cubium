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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/BaseBlocks.hpp"
#include "common/world/block/registry/BuildingVariantBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/template/ProtectedBlocksProcessor.hpp"
#include "common/world/gen/feature/template/Template.hpp"

#include <unordered_map>

using namespace mc;
using namespace mc::world::gen::feature::template_;

// ============================================================================
// 测试用世界：可配置目标位置的方块状态
// ============================================================================

/**
 * @brief 测试用世界桩
 *
 * 继承 BaseTestWorld 并覆写 getBlockState，允许测试按位置注入指定的方块状态。
 * 用于模拟ProtectedBlocksProcessor在结构放置时读取到的世界原有方块。
 */
class ProtectedBlocksTestWorld : public mc::test::BaseTestWorld {
public:
    ProtectedBlocksTestWorld()
    {
        // 初始化 BlockTags 以支持 contains 检查
        BlockTags::initialize();
    }

    /// 设置特定位置的方块状态（nullptr 表示该位置使用默认行为）
    void setBlockStateAt(const BlockPos& pos, const BlockState* state)
    {
        if (state != nullptr) {
            m_blockStates[pos] = state;
        } else {
            m_blockStates.erase(pos);
        }
    }

    [[nodiscard]] const BlockState* getBlockState(const BlockPos& pos) const override
    {
        auto it = m_blockStates.find(pos);
        if (it != m_blockStates.end()) {
            return it->second;
        }
        return BaseTestWorld::getBlockState(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(BlockPos(x, y, z));
    }

private:
    std::unordered_map<BlockPos, const BlockState*> m_blockStates;
};

// ============================================================================
// 测试夹具
// ============================================================================

class ProtectedBlocksProcessorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
        m_world = std::make_unique<ProtectedBlocksTestWorld>();
    }

    void TearDown() override { m_world.reset(); }

    /// 构造 PlacementSettings 并绑定测试世界
    PlacementSettings makeSettings() const
    {
        PlacementSettings settings;
        settings.setWorld(m_world.get());
        return settings;
    }

    std::unique_ptr<ProtectedBlocksTestWorld> m_world;
};

// ============================================================================
// 构造与克隆测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, ConstructorPreservesTagId)
{
    ResourceLocation tagId("minecraft", "features_cannot_replace");
    ProtectedBlocksProcessor processor(tagId);
    EXPECT_EQ(processor.getTagId(), tagId);
}

TEST_F(ProtectedBlocksProcessorTest, ClonePreservesTagId)
{
    ResourceLocation tagId("minecraft", "features_cannot_replace");
    ProtectedBlocksProcessor processor(tagId);
    auto cloned = processor.clone();

    auto* clonedProcessor = dynamic_cast<ProtectedBlocksProcessor*>(cloned.get());
    ASSERT_NE(clonedProcessor, nullptr);
    EXPECT_EQ(clonedProcessor->getTagId(), tagId);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, NoWorldReturnsOriginal)
{
    // 未设置世界读取器，应直接透传模板方块
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    BlockInfo blockInfo(BlockPos(0, 0, 0), dirtState.stateId());

    // 未设置 world 的 PlacementSettings
    PlacementSettings settings;
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, dirtState.stateId());
}

TEST_F(ProtectedBlocksProcessorTest, NonExistentTagReturnsOriginal)
{
    // 标签不存在时，视为空标签，所有方块均正常放置
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "non_existent_tag"));

    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    BlockInfo blockInfo(BlockPos(0, 0, 0), dirtState.stateId());

    // 世界中该位置为基岩（受保护），但因标签不存在，应正常放置
    if (VanillaBlocks::BEDROCK) {
        m_world->setBlockStateAt(BlockPos(0, 0, 0), &VanillaBlocks::BEDROCK->defaultState());
    }

    PlacementSettings settings = makeSettings();
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, dirtState.stateId());
}

TEST_F(ProtectedBlocksProcessorTest, NullWorldBlockStateReturnsOriginal)
{
    // getBlockState 返回 nullptr（位置未加载）时，应直接透传模板方块
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    BlockInfo blockInfo(BlockPos(0, 0, 0), dirtState.stateId());

    // 未设置该位置的方块状态，BaseTestWorld::getBlockState 返回 nullptr
    PlacementSettings settings = makeSettings();
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 0, 0), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, dirtState.stateId());
}

// ============================================================================
// 核心逻辑测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, ProtectedBlockIsSkipped)
{
    // 世界方块在保护标签内 → 返回 nullopt（跳过放置）
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::BEDROCK) {
        GTEST_SKIP() << "BEDROCK block not registered";
    }

    // 世界中该位置为基岩（在 FEATURES_CANNOT_REPLACE 标签内）
    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    m_world->setBlockStateAt(BlockPos(10, 20, 30), &bedrockState);

    // 模板方块为泥土（任意非保护方块）
    if (!VanillaBlocks::DIRT) {
        GTEST_SKIP() << "DIRT block not registered";
    }
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    BlockInfo blockInfo(BlockPos(10, 20, 30), dirtState.stateId());

    PlacementSettings settings = makeSettings();
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(10, 20, 30), blockInfo, blockInfo, settings);

    EXPECT_FALSE(result.has_value()) << "Protected block (bedrock) should be skipped";
}

TEST_F(ProtectedBlocksProcessorTest, NonProtectedBlockIsPlaced)
{
    // 世界方块不在保护标签内 → 返回 blockInfo（正常放置）
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::DIRT || !VanillaBlocks::STONE) {
        GTEST_SKIP() << "DIRT or STONE block not registered";
    }

    // 世界中该位置为泥土（不在保护标签内）
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    m_world->setBlockStateAt(BlockPos(5, 10, 15), &dirtState);

    // 模板方块为石头（任意方块）
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockInfo blockInfo(BlockPos(5, 10, 15), stoneState.stateId());

    PlacementSettings settings = makeSettings();
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(5, 10, 15), blockInfo, blockInfo, settings);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->blockStateId, stoneState.stateId());
}

TEST_F(ProtectedBlocksProcessorTest, SpawnerIsProtected)
{
    // 刷怪笼在 FEATURES_CANNOT_REPLACE 标签内，应被保护
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::SPAWNER) {
        GTEST_SKIP() << "SPAWNER block not registered";
    }

    const BlockState& spawnerState = VanillaBlocks::SPAWNER->defaultState();
    m_world->setBlockStateAt(BlockPos(0, 64, 0), &spawnerState);

    // 模板方块为石头（任意方块）
    if (!VanillaBlocks::STONE) {
        GTEST_SKIP() << "STONE block not registered";
    }
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    BlockInfo blockInfo(BlockPos(0, 64, 0), stoneState.stateId());

    PlacementSettings settings = makeSettings();
    auto result = processor.process(BlockPos(0, 0, 0), BlockPos(0, 64, 0), blockInfo, blockInfo, settings);

    EXPECT_FALSE(result.has_value()) << "Protected block (spawner) should be skipped";
}

// ============================================================================
// 多位置混合测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, MixedProtectedAndUnprotectedBlocks)
{
    // 在同一处理器中混合处理保护与非保护方块，验证行为一致性
    ProtectedBlocksProcessor processor(ResourceLocation("minecraft", "features_cannot_replace"));

    if (!VanillaBlocks::BEDROCK || !VanillaBlocks::DIRT || !VanillaBlocks::STONE) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    PlacementSettings settings = makeSettings();

    // 位置A：世界为基岩（保护）→ 跳过
    BlockPos posA(0, 0, 0);
    m_world->setBlockStateAt(posA, &bedrockState);
    BlockInfo infoA(posA, stoneState.stateId());
    auto resultA = processor.process(BlockPos(0, 0, 0), posA, infoA, infoA, settings);
    EXPECT_FALSE(resultA.has_value()) << "Bedrock at posA should be protected";

    // 位置B：世界为泥土（非保护）→ 放置
    BlockPos posB(1, 0, 0);
    m_world->setBlockStateAt(posB, &dirtState);
    BlockInfo infoB(posB, stoneState.stateId());
    auto resultB = processor.process(BlockPos(0, 0, 0), posB, infoB, infoB, settings);
    ASSERT_TRUE(resultB.has_value());
    EXPECT_EQ(resultB->blockStateId, stoneState.stateId());

    // 位置C：世界未加载（nullptr）→ 透传
    BlockPos posC(2, 0, 0);
    BlockInfo infoC(posC, stoneState.stateId());
    auto resultC = processor.process(BlockPos(0, 0, 0), posC, infoC, infoC, settings);
    ASSERT_TRUE(resultC.has_value());
    EXPECT_EQ(resultC->blockStateId, stoneState.stateId());
}

// ============================================================================
// 处理器链集成测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, WorksInProcessorList)
{
    // 验证 ProtectedBlocksProcessor 可以在 StructureProcessorList 中正常工作
    if (!VanillaBlocks::BEDROCK || !VanillaBlocks::STONE) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    StructureProcessorList list;
    list.addProcessor(
        std::make_unique<ProtectedBlocksProcessor>(ResourceLocation("minecraft", "features_cannot_replace")));

    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    BlockPos pos(10, 20, 30);
    m_world->setBlockStateAt(pos, &bedrockState);

    BlockInfo blockInfo(pos, stoneState.stateId());
    PlacementSettings settings = makeSettings();

    auto result = list.process(BlockPos(0, 0, 0), pos, blockInfo, blockInfo, settings);
    EXPECT_FALSE(result.has_value()) << "ProcessorList should skip protected block";

    // 移除保护方块，验证正常放置
    m_world->setBlockStateAt(pos, &stoneState);
    auto result2 = list.process(BlockPos(0, 0, 0), pos, blockInfo, blockInfo, settings);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->blockStateId, stoneState.stateId());
}

// ============================================================================
// clone() 后行为一致性测试
// ============================================================================

TEST_F(ProtectedBlocksProcessorTest, CloneProducesEquivalentBehavior)
{
    if (!VanillaBlocks::BEDROCK || !VanillaBlocks::STONE) {
        GTEST_SKIP() << "Required blocks not registered";
    }

    ProtectedBlocksProcessor original(ResourceLocation("minecraft", "features_cannot_replace"));
    auto cloned = original.clone();

    const BlockState& bedrockState = VanillaBlocks::BEDROCK->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    BlockPos pos(7, 8, 9);
    m_world->setBlockStateAt(pos, &bedrockState);
    BlockInfo blockInfo(pos, stoneState.stateId());

    PlacementSettings settings = makeSettings();

    auto resultOriginal = original.process(BlockPos(0, 0, 0), pos, blockInfo, blockInfo, settings);
    auto resultCloned = cloned->process(BlockPos(0, 0, 0), pos, blockInfo, blockInfo, settings);

    EXPECT_FALSE(resultOriginal.has_value());
    EXPECT_FALSE(resultCloned.has_value());
}
