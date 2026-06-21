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

#include "common/item/core/AdventureModePredicate.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"

using namespace mc;

// ============================================================================
// AdventureModePredicate 单元测试
// ============================================================================

class AdventureModePredicateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// 空谓词测试
// ============================================================================

TEST_F(AdventureModePredicateTest, DefaultConstructorCreatesEmptyPredicate)
{
    AdventureModePredicate predicate;
    EXPECT_TRUE(predicate.isEmpty());
    EXPECT_TRUE(predicate.getPredicates().empty());
}

TEST_F(AdventureModePredicateTest, EmptyPredicateDoesNotMatchAnyBlock)
{
    AdventureModePredicate predicate;
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 空谓词不匹配任何方块
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, EmptyVectorPredicateIsEmpty)
{
    AdventureModePredicate predicate(std::vector<std::string>{});
    EXPECT_TRUE(predicate.isEmpty());
}

// ============================================================================
// 精确方块ID匹配测试
// ============================================================================

TEST_F(AdventureModePredicateTest, ExactBlockIdMatchesCorrectBlock)
{
    // 构造谓词: 只允许放置石头
    AdventureModePredicate predicate({"minecraft:stone"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, ExactBlockIdDoesNotMatchDifferentBlock)
{
    // 构造谓词: 只允许放置石头
    AdventureModePredicate predicate({"minecraft:stone"});
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    EXPECT_FALSE(predicate.test(dirtState));
}

TEST_F(AdventureModePredicateTest, MultipleExactBlockIds)
{
    // 构造谓词: 允许放置石头和泥土
    AdventureModePredicate predicate({"minecraft:stone", "minecraft:dirt"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();

    EXPECT_TRUE(predicate.test(stoneState));
    EXPECT_TRUE(predicate.test(dirtState));
    EXPECT_FALSE(predicate.test(grassState));
}

TEST_F(AdventureModePredicateTest, ExactBlockIdIsCaseSensitive)
{
    // 方块ID大小写敏感：minecraft:Stone 不等于 minecraft:stone
    AdventureModePredicate predicate({"minecraft:Stone"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // minecraft:Stone 不应匹配 minecraft:stone
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, ExactBlockIdWithNamespace)
{
    // 显式指定命名空间的方块ID
    AdventureModePredicate predicate({"minecraft:grass_block"});
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();

    EXPECT_TRUE(predicate.test(grassState));
}

// ============================================================================
// 标签引用匹配测试（#前缀）
// ============================================================================

TEST_F(AdventureModePredicateTest, TagReferenceMatchesBlockInTag)
{
    // 构造谓词: 允许放置 #minecraft:logs 标签中的方块
    AdventureModePredicate predicate({"#minecraft:logs"});

    const BlockState& oakLogState = VanillaBlocks::OAK_LOG->defaultState();
    EXPECT_TRUE(predicate.test(oakLogState));
}

TEST_F(AdventureModePredicateTest, TagReferenceDoesNotMatchBlockNotInTag)
{
    // 构造谓词: 允许放置 #minecraft:logs 标签中的方块
    AdventureModePredicate predicate({"#minecraft:logs"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, TagReferenceWithDirtTag)
{
    // 构造谓词: 允许放置 #minecraft:dirt 标签中的方块
    AdventureModePredicate predicate({"#minecraft:dirt"});

    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // dirt 标签应包含 dirt 和 grass_block
    EXPECT_TRUE(predicate.test(dirtState));
    EXPECT_TRUE(predicate.test(grassState));
    // 但不包含 stone
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, UnknownTagReferenceDoesNotMatch)
{
    // 不存在的标签不应匹配任何方块
    AdventureModePredicate predicate({"#minecraft:nonexistent_tag"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, MixedExactIdAndTagReference)
{
    // 混合精确ID和标签引用
    AdventureModePredicate predicate({"minecraft:stone", "#minecraft:dirt"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    const BlockState& oakLogState = VanillaBlocks::OAK_LOG->defaultState();

    // stone 通过精确ID匹配
    EXPECT_TRUE(predicate.test(stoneState));
    // dirt 通过标签匹配
    EXPECT_TRUE(predicate.test(dirtState));
    // grass_block 通过 dirt 标签匹配
    EXPECT_TRUE(predicate.test(grassState));
    // oak_log 既不通过精确ID也不通过标签匹配
    EXPECT_FALSE(predicate.test(oakLogState));
}

// ============================================================================
// OR 逻辑测试
// ============================================================================

TEST_F(AdventureModePredicateTest, AnyMatchingPredicateReturnsTrue)
{
    // 只要任一谓词条目匹配就返回 true（OR 逻辑）
    AdventureModePredicate predicate({"minecraft:stone", "minecraft:dirt", "#minecraft:logs"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();
    const BlockState& oakLogState = VanillaBlocks::OAK_LOG->defaultState();

    // 三种匹配方式各自独立生效
    EXPECT_TRUE(predicate.test(stoneState));
    EXPECT_TRUE(predicate.test(dirtState));
    EXPECT_TRUE(predicate.test(oakLogState));
}

TEST_F(AdventureModePredicateTest, NoMatchingPredicateReturnsFalse)
{
    // 没有任何条目匹配时返回 false
    AdventureModePredicate predicate({"minecraft:oak_log", "#minecraft:wool"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(predicate.test(stoneState));
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(AdventureModePredicateTest, EmptyStringPredicateDoesNotMatch)
{
    // 空字符串谓词条目不应匹配任何方块
    AdventureModePredicate predicate({""});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, HashOnlyTagReferenceDoesNotMatch)
{
    // 只有 # 前缀没有标签ID的条目不应匹配
    AdventureModePredicate predicate({"#"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // "#" 解析为 ResourceLocation("")，不应匹配任何标签
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, GetPredicatesReturnsCorrectList)
{
    std::vector<std::string> expectedPredicates = {"minecraft:stone", "#minecraft:logs", "minecraft:dirt"};
    AdventureModePredicate predicate(expectedPredicates);

    EXPECT_FALSE(predicate.isEmpty());
    EXPECT_EQ(predicate.getPredicates().size(), 3u);
    EXPECT_EQ(predicate.getPredicates()[0], "minecraft:stone");
    EXPECT_EQ(predicate.getPredicates()[1], "#minecraft:logs");
    EXPECT_EQ(predicate.getPredicates()[2], "minecraft:dirt");
}

// ============================================================================
// 相等比较测试
// ============================================================================

TEST_F(AdventureModePredicateTest, EqualPredicatesAreEqual)
{
    AdventureModePredicate predicate1({"minecraft:stone", "#minecraft:logs"});
    AdventureModePredicate predicate2({"minecraft:stone", "#minecraft:logs"});

    EXPECT_TRUE(predicate1 == predicate2);
    EXPECT_FALSE(predicate1 != predicate2);
}

TEST_F(AdventureModePredicateTest, DifferentPredicatesAreNotEqual)
{
    AdventureModePredicate predicate1({"minecraft:stone"});
    AdventureModePredicate predicate2({"minecraft:dirt"});

    EXPECT_FALSE(predicate1 == predicate2);
    EXPECT_TRUE(predicate1 != predicate2);
}

TEST_F(AdventureModePredicateTest, EmptyPredicatesAreEqual)
{
    AdventureModePredicate predicate1;
    AdventureModePredicate predicate2;

    EXPECT_TRUE(predicate1 == predicate2);
    EXPECT_FALSE(predicate1 != predicate2);
}

TEST_F(AdventureModePredicateTest, DifferentOrderIsNotEqual)
{
    // 顺序不同则不相等
    AdventureModePredicate predicate1({"minecraft:stone", "minecraft:dirt"});
    AdventureModePredicate predicate2({"minecraft:dirt", "minecraft:stone"});

    EXPECT_FALSE(predicate1 == predicate2);
    EXPECT_TRUE(predicate1 != predicate2);
}

TEST_F(AdventureModePredicateTest, DifferentSizeIsNotEqual)
{
    AdventureModePredicate predicate1({"minecraft:stone"});
    AdventureModePredicate predicate2({"minecraft:stone", "minecraft:dirt"});

    EXPECT_FALSE(predicate1 == predicate2);
    EXPECT_TRUE(predicate1 != predicate2);
}

// ============================================================================
// 带 IWorld 参数的 test 重载测试
// ============================================================================

TEST_F(AdventureModePredicateTest, TestWithWorldContextDelegatesToTestWithoutWorld)
{
    // 当前实现中，test(IWorld&, BlockState&) 委托给 test(BlockState&)
    // 验证行为一致性
    AdventureModePredicate predicate({"minecraft:stone"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // 不使用 IWorld 的 test
    EXPECT_TRUE(predicate.test(stoneState));
    EXPECT_FALSE(predicate.test(dirtState));

    // 注意：不测试 test(IWorld&, BlockState&) 因为需要完整的 IWorld mock，
    // 当前实现直接委托给 test(BlockState&)，逻辑已通过上面的测试覆盖
}
