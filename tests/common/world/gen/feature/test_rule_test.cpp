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

#include "TestWorldHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/state/SimpleBlockStateProvider.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace state = mc::world::gen::feature::state;

namespace {
// BaseTestWorld 默认构造为 protected，派生一个 public 构造的测试世界供采样调用。
class RuleTestWorld : public mc::test::BaseTestWorld {
public:
    RuleTestWorld() = default;
};
} // namespace

class RuleTestTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

// ============================================================================
// AlwaysTrueRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, AlwaysTrueReturnsTrue)
{
    math::Random rng(12345);
    const BlockState* state = VanillaBlocks::getState(VanillaBlocks::STONE);

    EXPECT_TRUE(AlwaysTrueRuleTest::INSTANCE.test(*state, rng));
}

TEST_F(RuleTestTest, AlwaysTrueClone)
{
    auto clone = AlwaysTrueRuleTest::INSTANCE.clone();
    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->name(), "always_true");
}

// ============================================================================
// BlockMatchRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, BlockMatchMatchesCorrectBlock)
{
    math::Random rng(12345);
    BlockMatchRuleTest test(VanillaBlocks::STONE);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);

    EXPECT_TRUE(test.test(*stoneState, rng));
    EXPECT_FALSE(test.test(*dirtState, rng));
}

TEST_F(RuleTestTest, BlockMatchClone)
{
    BlockMatchRuleTest test(VanillaBlocks::STONE);
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->name(), "block_match");

    BlockMatchRuleTest* clonedTest = dynamic_cast<BlockMatchRuleTest*>(clone.get());
    EXPECT_NE(clonedTest, nullptr);
    EXPECT_EQ(clonedTest->getBlock(), VanillaBlocks::STONE);
}

TEST_F(RuleTestTest, BlockMatchNullBlock)
{
    math::Random rng(12345);
    BlockMatchRuleTest test(nullptr);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    EXPECT_FALSE(test.test(*stoneState, rng));
}

// ============================================================================
// BlockStateMatchRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, BlockStateMatchMatchesExactState)
{
    math::Random rng(12345);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    BlockStateMatchRuleTest test(stoneState);

    // 相同状态应该匹配
    EXPECT_TRUE(test.test(*stoneState, rng));

    // 不同方块不应该匹配
    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);
    EXPECT_FALSE(test.test(*dirtState, rng));
}

TEST_F(RuleTestTest, BlockStateMatchClone)
{
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    BlockStateMatchRuleTest test(stoneState);

    auto clone = test.clone();
    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->name(), "block_state_match");
}

// ============================================================================
// RandomBlockMatchRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, RandomBlockMatchWithProbability1)
{
    math::Random rng(12345);
    RandomBlockMatchRuleTest test(VanillaBlocks::STONE, 1.0f);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 概率1.0应该总是匹配
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(test.test(*stoneState, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockMatchWithProbability0)
{
    math::Random rng(12345);
    RandomBlockMatchRuleTest test(VanillaBlocks::STONE, 0.0f);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 概率0.0应该从不匹配
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(test.test(*stoneState, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockMatchWrongBlock)
{
    math::Random rng(12345);
    RandomBlockMatchRuleTest test(VanillaBlocks::STONE, 1.0f);

    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);

    // 即使概率1.0，方块不匹配也不应该返回true
    EXPECT_FALSE(test.test(*dirtState, rng));
}

TEST_F(RuleTestTest, RandomBlockMatchProbabilityDistribution)
{
    math::Random rng(12345);
    RandomBlockMatchRuleTest test(VanillaBlocks::STONE, 0.5f);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    int matches = 0;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        if (test.test(*stoneState, rng)) {
            matches++;
        }
    }

    // 验证概率分布（允许10%误差）
    float actualProbability = static_cast<float>(matches) / trials;
    EXPECT_NEAR(actualProbability, 0.5f, 0.1f);
}

TEST_F(RuleTestTest, RandomBlockMatchClone)
{
    RandomBlockMatchRuleTest test(VanillaBlocks::STONE, 0.7f);
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->name(), "random_block_match");

    RandomBlockMatchRuleTest* clonedTest = dynamic_cast<RandomBlockMatchRuleTest*>(clone.get());
    EXPECT_NE(clonedTest, nullptr);
    EXPECT_EQ(clonedTest->getBlock(), VanillaBlocks::STONE);
    EXPECT_FLOAT_EQ(clonedTest->getProbability(), 0.7f);
}

// ============================================================================
// RandomBlockStateMatchRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, RandomBlockStateMatchWithProbability1)
{
    math::Random rng(12345);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    RandomBlockStateMatchRuleTest test(stoneState, 1.0f);

    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(test.test(*stoneState, rng));
    }
}

TEST_F(RuleTestTest, RandomBlockStateMatchWrongState)
{
    math::Random rng(12345);

    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    RandomBlockStateMatchRuleTest test(stoneState, 1.0f);

    const BlockState* dirtState = VanillaBlocks::getState(VanillaBlocks::DIRT);
    EXPECT_FALSE(test.test(*dirtState, rng));
}

// ============================================================================
// StoneRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, StoneRuleTestMatchesStoneTypes)
{
    math::Random rng(12345);
    StoneRuleTest test;

    // 应该匹配
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::GRANITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIORITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::ANDESITE), rng));

    // 不应该匹配
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIRT), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::COBBLESTONE), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::BEDROCK), rng));
}

TEST_F(RuleTestTest, StoneRuleTestClone)
{
    StoneRuleTest test;
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);
    EXPECT_EQ(clone->name(), "stone");
}

// ============================================================================
// TagMatchRuleTest 测试
// ============================================================================

TEST_F(RuleTestTest, TagMatchRuleTestCreation)
{
    TagMatchRuleTest test(ResourceLocation("minecraft:stone"));

    EXPECT_EQ(test.getTagName(), ResourceLocation("minecraft:stone"));
    EXPECT_EQ(test.name(), "tag_match");
}

TEST_F(RuleTestTest, TagMatchRuleTestClone)
{
    TagMatchRuleTest test(ResourceLocation("minecraft:logs"));
    auto clone = test.clone();

    EXPECT_NE(clone, nullptr);

    TagMatchRuleTest* clonedTest = dynamic_cast<TagMatchRuleTest*>(clone.get());
    EXPECT_NE(clonedTest, nullptr);
    EXPECT_EQ(clonedTest->getTagName(), ResourceLocation("minecraft:logs"));
}

TEST_F(RuleTestTest, TagMatchRuleTestMatchesStoneTag)
{
    // BlockTags::initialize() 已在 VanillaBlocks::initialize() 中调用
    TagMatchRuleTest test(ResourceLocation("minecraft:stone"));
    math::Random rng(12345);

    // 应该匹配 stone 标签中的方块
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::GRANITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIORITE), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::ANDESITE), rng));

    // 不应该匹配不在 stone 标签中的方块
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIRT), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::COBBLESTONE), rng));
}

TEST_F(RuleTestTest, TagMatchRuleTestMatchesLogsTag)
{
    TagMatchRuleTest test(ResourceLocation("minecraft:logs"));
    math::Random rng(12345);

    // 应该匹配 logs 标签中的方块
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::OAK_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::SPRUCE_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::BIRCH_LOG), rng));
    EXPECT_TRUE(test.test(*VanillaBlocks::getState(VanillaBlocks::JUNGLE_LOG), rng));

    // 不应该匹配不在 logs 标签中的方块
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::DIRT), rng));
}

TEST_F(RuleTestTest, TagMatchRuleTestNonExistentTag)
{
    // 不存在的标签应该返回 false
    TagMatchRuleTest test(ResourceLocation("minecraft:nonexistent_tag"));
    math::Random rng(12345);

    EXPECT_FALSE(test.test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
}

// ============================================================================
// createOreTarget 测试
// ============================================================================

TEST_F(RuleTestTest, CreateOreTargetNaturalStone)
{
    auto target = createOreTarget(OreTargetType::NaturalStone);
    EXPECT_NE(target, nullptr);
    EXPECT_EQ(target->name(), "stone");

    math::Random rng(12345);
    EXPECT_TRUE(target->test(*VanillaBlocks::getState(VanillaBlocks::STONE), rng));
}

TEST_F(RuleTestTest, CreateOreTargetNetherrack)
{
    auto target = createOreTarget(OreTargetType::Netherrack);
    EXPECT_NE(target, nullptr);

    math::Random rng(12345);
    if (VanillaBlocks::NETHERRACK) {
        EXPECT_TRUE(target->test(*VanillaBlocks::getState(VanillaBlocks::NETHERRACK), rng));
    }
}

TEST_F(RuleTestTest, CreateOreTargetBasalt)
{
    auto target = createOreTarget(OreTargetType::Basalt);
    EXPECT_NE(target, nullptr);

    math::Random rng(12345);
    if (VanillaBlocks::BASALT) {
        EXPECT_TRUE(target->test(*VanillaBlocks::getState(VanillaBlocks::BASALT), rng));
    }
}

// ============================================================================
// OreFeatureConfig 测试
// ============================================================================

TEST_F(RuleTestTest, OreFeatureConfigCreation)
{
    auto target = std::make_unique<StoneRuleTest>();
    const BlockState* oreState = VanillaBlocks::getState(VanillaBlocks::IRON_ORE);

    OreFeatureConfig config(std::move(target), oreState, 9);

    EXPECT_EQ(config.size, 9);
    EXPECT_EQ(config.state, oreState);
    EXPECT_NE(config.target, nullptr);
}

TEST_F(RuleTestTest, OreFeatureConfigNaturalStone)
{
    auto target = OreFeatureConfig::naturalStone();
    EXPECT_NE(target, nullptr);
    EXPECT_EQ(target->name(), "stone");
}

// ============================================================================
// SimpleBlockStateProvider 测试
// ============================================================================

TEST_F(RuleTestTest, SimpleBlockStateProvider)
{
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);
    state::SimpleBlockStateProvider provider(stoneState);

    math::Random rng(12345);
    RuleTestWorld world;

    // 应该总是返回相同的状态
    EXPECT_EQ(provider.getState(world, rng, 0, 0, 0), stoneState);
    EXPECT_EQ(provider.getState(world, rng, 100, 50, 100), stoneState);
}
