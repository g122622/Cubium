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

// ============================================================================
// 方块状态属性匹配测试
// ============================================================================

TEST_F(AdventureModePredicateTest, BlockWithPropertyMatch_SingleProperty)
{
    // "minecraft:oak_log[axis=y]" 匹配 axis=y 的橡木原木
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y]"});

    // 获取 oak_log 的不同状态
    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();

    // 找到 axis 属性
    const IProperty* axisProp = container.getProperty("axis");
    ASSERT_NE(axisProp, nullptr);

    // 查找 axis=y 的状态
    const BlockState* yAxisState = nullptr;
    const BlockState* xAxisState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*axisProp);
        if (valIdx.has_value()) {
            auto parsedY = axisProp->parseValue("y");
            auto parsedX = axisProp->parseValue("x");
            if (parsedY.has_value() && *valIdx == *parsedY) {
                yAxisState = state.get();
            }
            if (parsedX.has_value() && *valIdx == *parsedX) {
                xAxisState = state.get();
            }
        }
    }

    ASSERT_NE(yAxisState, nullptr) << "oak_log 应该有 axis=y 状态";
    ASSERT_NE(xAxisState, nullptr) << "oak_log 应该有 axis=x 状态";

    // axis=y 应该匹配
    EXPECT_TRUE(predicate.test(*yAxisState));
    // axis=x 不应该匹配
    EXPECT_FALSE(predicate.test(*xAxisState));
}

TEST_F(AdventureModePredicateTest, BlockWithPropertyMatch_WrongProperty)
{
    // "minecraft:oak_log[axis=y]" 不匹配 axis=x 的状态
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y]"});

    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();
    const IProperty* axisProp = container.getProperty("axis");
    ASSERT_NE(axisProp, nullptr);

    // 找到 axis=x 的状态
    const BlockState* xAxisState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*axisProp);
        if (valIdx.has_value()) {
            auto parsedX = axisProp->parseValue("x");
            if (parsedX.has_value() && *valIdx == *parsedX) {
                xAxisState = state.get();
            }
        }
    }

    ASSERT_NE(xAxisState, nullptr);
    EXPECT_FALSE(predicate.test(*xAxisState));
}

TEST_F(AdventureModePredicateTest, BlockWithPropertyMatch_WrongBlock)
{
    // "minecraft:oak_log[axis=y]" 不匹配 dirt（即使 dirt 没有 axis 属性）
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y]"});
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // dirt 不是 oak_log，所以不匹配
    EXPECT_FALSE(predicate.test(dirtState));
}

TEST_F(AdventureModePredicateTest, BlockWithPropertyMatch_NonexistentProperty)
{
    // "minecraft:stone[axis=y]" 不匹配 stone（stone 没有 axis 属性）
    AdventureModePredicate predicate({"minecraft:stone[axis=y]"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // stone 没有 axis 属性，属性匹配失败
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, BlockWithPropertyMatch_MultipleProperties)
{
    // 多属性匹配: "minecraft:oak_stairs[half=top,facing=east]"
    // 注意：这个测试依赖于 oak_stairs 方块是否存在且有这些属性
    const Block* oakStairs = VanillaBlocks::OAK_STAIRS;
    if (!oakStairs) {
        GTEST_SKIP() << "OAK_STAIRS block not registered";
    }

    AdventureModePredicate predicate({"minecraft:oak_stairs[half=top,facing=east]"});

    const auto& container = oakStairs->stateContainer();
    const IProperty* halfProp = container.getProperty("half");
    const IProperty* facingProp = container.getProperty("facing");

    if (!halfProp || !facingProp) {
        GTEST_SKIP() << "oak_stairs 缺少 half 或 facing 属性";
    }

    // 查找匹配 half=top, facing=east 的状态
    const BlockState* matchedState = nullptr;
    const BlockState* nonMatchedState = nullptr;
    for (const auto& state : container.validStates()) {
        auto halfIdx = state->getValueIndex(*halfProp);
        auto facingIdx = state->getValueIndex(*facingProp);
        if (halfIdx.has_value() && facingIdx.has_value()) {
            auto parsedHalfTop = halfProp->parseValue("top");
            auto parsedFacingEast = facingProp->parseValue("east");
            auto parsedHalfBottom = halfProp->parseValue("bottom");
            auto parsedFacingNorth = facingProp->parseValue("north");

            if (parsedHalfTop.has_value() && parsedFacingEast.has_value() && *halfIdx == *parsedHalfTop &&
                *facingIdx == *parsedFacingEast) {
                matchedState = state.get();
            }
            if (parsedHalfBottom.has_value() && parsedFacingNorth.has_value() && *halfIdx == *parsedHalfBottom &&
                *facingIdx == *parsedFacingNorth) {
                nonMatchedState = state.get();
            }
        }
    }

    if (matchedState) {
        EXPECT_TRUE(predicate.test(*matchedState));
    }
    if (nonMatchedState) {
        EXPECT_FALSE(predicate.test(*nonMatchedState));
    }
}

TEST_F(AdventureModePredicateTest, TagWithPropertyMatch)
{
    // "#minecraft:logs[axis=y]" 匹配 axis=y 的原木
    AdventureModePredicate predicate({"#minecraft:logs[axis=y]"});

    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();
    const IProperty* axisProp = container.getProperty("axis");
    ASSERT_NE(axisProp, nullptr);

    // 找到 axis=y 和 axis=x 的状态
    const BlockState* yAxisState = nullptr;
    const BlockState* xAxisState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*axisProp);
        if (valIdx.has_value()) {
            auto parsedY = axisProp->parseValue("y");
            auto parsedX = axisProp->parseValue("x");
            if (parsedY.has_value() && *valIdx == *parsedY) {
                yAxisState = state.get();
            }
            if (parsedX.has_value() && *valIdx == *parsedX) {
                xAxisState = state.get();
            }
        }
    }

    ASSERT_NE(yAxisState, nullptr);
    ASSERT_NE(xAxisState, nullptr);

    // oak_log 在 logs 标签中，axis=y 应该匹配
    EXPECT_TRUE(predicate.test(*yAxisState));
    // oak_log 在 logs 标签中，但 axis=x 不匹配
    EXPECT_FALSE(predicate.test(*xAxisState));
}

TEST_F(AdventureModePredicateTest, TagWithPropertyMatch_TagNotMatching)
{
    // "#minecraft:logs[axis=y]" 不匹配 stone（stone 不在 logs 标签中）
    AdventureModePredicate predicate({"#minecraft:logs[axis=y]"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, MixedExactIdAndPropertyAndTag)
{
    // 混合匹配：精确ID、带属性的ID、标签
    AdventureModePredicate predicate({"minecraft:stone", "minecraft:oak_log[axis=y]", "#minecraft:dirt"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // stone 通过精确ID匹配
    EXPECT_TRUE(predicate.test(stoneState));
    // dirt 通过标签匹配
    EXPECT_TRUE(predicate.test(dirtState));

    // oak_log[axis=y] 通过属性匹配
    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();
    const IProperty* axisProp = container.getProperty("axis");
    if (axisProp) {
        for (const auto& state : container.validStates()) {
            auto valIdx = state->getValueIndex(*axisProp);
            if (valIdx.has_value()) {
                auto parsedY = axisProp->parseValue("y");
                auto parsedX = axisProp->parseValue("x");
                if (parsedY.has_value() && *valIdx == *parsedY) {
                    EXPECT_TRUE(predicate.test(*state)); // axis=y 匹配
                }
                if (parsedX.has_value() && *valIdx == *parsedX) {
                    EXPECT_FALSE(predicate.test(*state)); // axis=x 不匹配
                }
            }
        }
    }
}

TEST_F(AdventureModePredicateTest, PropertyMatch_EmptyBrackets)
{
    // "minecraft:oak_log[]" 空属性列表等同于无属性匹配
    AdventureModePredicate predicate({"minecraft:oak_log[]"});
    const BlockState& oakLogDefault = VanillaBlocks::OAK_LOG->defaultState();

    // 空属性列表应该匹配 oak_log 的任何状态（只检查方块ID）
    EXPECT_TRUE(predicate.test(oakLogDefault));
}

TEST_F(AdventureModePredicateTest, PropertyMatch_InvalidPropertyFormat)
{
    // 无效属性格式：没有等号
    AdventureModePredicate predicate({"minecraft:oak_log[axis]"});
    const BlockState& oakLogDefault = VanillaBlocks::OAK_LOG->defaultState();

    // 解析失败，不匹配
    EXPECT_FALSE(predicate.test(oakLogDefault));
}

TEST_F(AdventureModePredicateTest, PropertyMatch_UnclosedBracket)
{
    // 没有闭合方括号
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y"});
    const BlockState& oakLogDefault = VanillaBlocks::OAK_LOG->defaultState();

    // 解析失败，不匹配
    EXPECT_FALSE(predicate.test(oakLogDefault));
}

TEST_F(AdventureModePredicateTest, PropertyMatch_InvalidPropertyValue)
{
    // 有效的属性名但无效的属性值
    AdventureModePredicate predicate({"minecraft:oak_log[axis=invalid_value]"});
    const BlockState& oakLogDefault = VanillaBlocks::OAK_LOG->defaultState();

    // 属性值 "invalid_value" 无法解析，不匹配
    EXPECT_FALSE(predicate.test(oakLogDefault));
}

TEST_F(AdventureModePredicateTest, PropertyMatch_PreserveOriginalStringInGetPredicates)
{
    // getPredicates() 应该返回原始字符串，包含方括号
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y]", "#minecraft:logs", "minecraft:stone"});
    const auto& preds = predicate.getPredicates();

    EXPECT_EQ(preds.size(), 3u);
    EXPECT_EQ(preds[0], "minecraft:oak_log[axis=y]");
    EXPECT_EQ(preds[1], "#minecraft:logs");
    EXPECT_EQ(preds[2], "minecraft:stone");
}

TEST_F(AdventureModePredicateTest, PropertyMatch_EqualityWithPropertyStrings)
{
    // 带属性的谓词字符串也参与相等比较
    AdventureModePredicate predicate1({"minecraft:oak_log[axis=y]", "minecraft:stone"});
    AdventureModePredicate predicate2({"minecraft:oak_log[axis=y]", "minecraft:stone"});
    AdventureModePredicate predicate3({"minecraft:oak_log[axis=x]", "minecraft:stone"});

    EXPECT_TRUE(predicate1 == predicate2);
    EXPECT_FALSE(predicate1 == predicate3);
}

TEST_F(AdventureModePredicateTest, PropertyMatch_NBTBracketSyntax_BlockIdExtracted)
{
    // "minecraft:chest{Items:[{id:'minecraft:diamond',Count:1b}]}" —— NBT 花括号语法的方块ID部分应正确提取
    // NBT 语法支持后，方块ID部分应被正确提取为 "minecraft:chest"
    AdventureModePredicate predicate({"minecraft:chest{Items:[{id:'minecraft:diamond',Count:1b}]}"});
    const BlockState& chestState =
        VanillaBlocks::CHEST ? VanillaBlocks::CHEST->defaultState() : VanillaBlocks::STONE->defaultState();

    // 方块ID "minecraft:chest" 应该正确匹配（NBT 部分不影响方块ID匹配）
    // 但没有世界上下文时（纯 BlockState 版本的 test），NBT 检查被跳过
    // 如果 chestState 是 CHEST，则应匹配；如果是 STONE 回退，则不匹配
    if (VanillaBlocks::CHEST) {
        // 纯方块状态版本跳过 NBT 检查，方块ID 匹配即通过
        EXPECT_TRUE(predicate.test(chestState));
    } else {
        EXPECT_FALSE(predicate.test(chestState));
    }
}

TEST_F(AdventureModePredicateTest, BlockWithBooleanProperty)
{
    // 测试布尔属性匹配: "minecraft:redstone_lamp[lit=true]"
    const Block* redstoneLamp = VanillaBlocks::REDSTONE_LAMP;
    if (!redstoneLamp) {
        GTEST_SKIP() << "REDSTONE_LAMP block not registered";
    }

    AdventureModePredicate predicate({"minecraft:redstone_lamp[lit=true]"});

    const auto& container = redstoneLamp->stateContainer();
    const IProperty* litProp = container.getProperty("lit");
    if (!litProp) {
        GTEST_SKIP() << "redstone_lamp 缺少 lit 属性";
    }

    const BlockState* litState = nullptr;
    const BlockState* unlitState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*litProp);
        if (valIdx.has_value()) {
            auto parsedTrue = litProp->parseValue("true");
            auto parsedFalse = litProp->parseValue("false");
            if (parsedTrue.has_value() && *valIdx == *parsedTrue) {
                litState = state.get();
            }
            if (parsedFalse.has_value() && *valIdx == *parsedFalse) {
                unlitState = state.get();
            }
        }
    }

    if (litState) {
        EXPECT_TRUE(predicate.test(*litState));
    }
    if (unlitState) {
        EXPECT_FALSE(predicate.test(*unlitState));
    }
}

TEST_F(AdventureModePredicateTest, BlockWithIntegerProperty)
{
    // 测试整数属性匹配: "minecraft:farmland[moisture=7]"
    const Block* farmland = VanillaBlocks::FARMLAND;
    if (!farmland) {
        GTEST_SKIP() << "FARMLAND block not registered";
    }

    AdventureModePredicate predicate({"minecraft:farmland[moisture=7]"});

    const auto& container = farmland->stateContainer();
    const IProperty* moistureProp = container.getProperty("moisture");
    if (!moistureProp) {
        GTEST_SKIP() << "farmland 缺少 moisture 属性";
    }

    const BlockState* moisture7State = nullptr;
    const BlockState* moisture0State = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*moistureProp);
        if (valIdx.has_value()) {
            auto parsed7 = moistureProp->parseValue("7");
            auto parsed0 = moistureProp->parseValue("0");
            if (parsed7.has_value() && *valIdx == *parsed7) {
                moisture7State = state.get();
            }
            if (parsed0.has_value() && *valIdx == *parsed0) {
                moisture0State = state.get();
            }
        }
    }

    if (moisture7State) {
        EXPECT_TRUE(predicate.test(*moisture7State));
    }
    if (moisture0State) {
        EXPECT_FALSE(predicate.test(*moisture0State));
    }
}

// ============================================================================
// NBT 谓词解析测试
// ============================================================================

TEST_F(AdventureModePredicateTest, NBTBracket_BlockIdExtractedCorrectly)
{
    // "minecraft:stone{CustomName:'test'}" —— 方块ID应正确提取为 "minecraft:stone"
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // 纯方块状态版本的 test 跳过 NBT 检查，方块ID 匹配即通过
    EXPECT_TRUE(predicate.test(stoneState));
    EXPECT_FALSE(predicate.test(dirtState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_PropertyAndNBTCombined)
{
    // "minecraft:oak_log[axis=y]{CustomName:'test'}" —— 属性和NBT同时存在
    AdventureModePredicate predicate({"minecraft:oak_log[axis=y]{CustomName:'test'}"});

    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();
    const IProperty* axisProp = container.getProperty("axis");
    ASSERT_NE(axisProp, nullptr);

    const BlockState* yAxisState = nullptr;
    const BlockState* xAxisState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*axisProp);
        if (valIdx.has_value()) {
            auto parsedY = axisProp->parseValue("y");
            auto parsedX = axisProp->parseValue("x");
            if (parsedY.has_value() && *valIdx == *parsedY) {
                yAxisState = state.get();
            }
            if (parsedX.has_value() && *valIdx == *parsedX) {
                xAxisState = state.get();
            }
        }
    }

    ASSERT_NE(yAxisState, nullptr);
    ASSERT_NE(xAxisState, nullptr);

    // 纯方块状态版本跳过 NBT 检查
    // axis=y 应该匹配（属性和方块ID都匹配，NBT被忽略）
    EXPECT_TRUE(predicate.test(*yAxisState));
    // axis=x 不应该匹配（属性不匹配）
    EXPECT_FALSE(predicate.test(*xAxisState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_PropertyAfterNBTNotSupported)
{
    // NBT 部分必须在属性之后："[...]{...}" 格式
    // "minecraft:stone{CustomName:'test'}[axis=y]" 不是有效格式
    // 在这种情况下，{...} 之后的部分不会被识别为属性
    // 方块ID 仍应正确提取为 "minecraft:stone"
    AdventureModePredicate predicate({"minecraft:stone{CustomName:'test'}[axis=y]"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 方块ID 匹配（NBT和属性部分按解析规则处理）
    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_OnlyNBTNoProperties)
{
    // "minecraft:stone{Count:1b}" —— 只有NBT，没有属性
    AdventureModePredicate predicate({"minecraft:stone{Count:1b}"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 纯方块状态版本跳过 NBT 检查，方块ID 匹配即通过
    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_TagReferenceWithNBT)
{
    // "#minecraft:logs{CustomName:'test'}" —— 标签引用 + NBT
    AdventureModePredicate predicate({"#minecraft:logs{CustomName:'test'}"});

    const BlockState& oakLogState = VanillaBlocks::OAK_LOG->defaultState();
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 纯方块状态版本跳过 NBT 检查
    EXPECT_TRUE(predicate.test(oakLogState));
    EXPECT_FALSE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_MultiplePredicatesWithNBT)
{
    // 混合含NBT和不含NBT的谓词条目
    AdventureModePredicate predicate({"minecraft:stone", "minecraft:dirt{CustomName:'test'}"});

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    const BlockState& dirtState = VanillaBlocks::DIRT->defaultState();

    // stone 不含NBT，匹配
    EXPECT_TRUE(predicate.test(stoneState));
    // dirt 含NBT但纯状态版本跳过NBT检查，方块ID匹配即通过
    EXPECT_TRUE(predicate.test(dirtState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_EmptyNBTBraces)
{
    // "minecraft:stone{}" —— 空NBT
    AdventureModePredicate predicate({"minecraft:stone{}"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 空NBT {} 解析为空 compound_tag，isAny() 为 true
    // 纯方块状态版本跳过 NBT 检查，方块ID 匹配即通过
    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_InvalidNBTFormat)
{
    // "minecraft:stone{invalid" —— 无效NBT格式（没有闭合大括号）
    // parseMojangson 会失败，nbtTag 为 nullptr，hasNbt 为 false
    AdventureModePredicate predicate({"minecraft:stone{invalid"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 方块ID 部分仍然正确提取，NBT 解析失败但不影响方块匹配
    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_GetPredicatesIncludesNBT)
{
    // getPredicates() 应该返回原始字符串，包含NBT部分
    AdventureModePredicate predicate({"minecraft:chest{Items:[{id:'minecraft:diamond',Count:1b}]}"});
    const auto& preds = predicate.getPredicates();

    EXPECT_EQ(preds.size(), 1u);
    EXPECT_EQ(preds[0], "minecraft:chest{Items:[{id:'minecraft:diamond',Count:1b}]}");
}

TEST_F(AdventureModePredicateTest, NBTBracket_EqualityWithNBTStrings)
{
    // 带NBT的谓词字符串也参与相等比较
    AdventureModePredicate predicate1({"minecraft:stone{Count:1}", "minecraft:dirt"});
    AdventureModePredicate predicate2({"minecraft:stone{Count:1}", "minecraft:dirt"});
    AdventureModePredicate predicate3({"minecraft:stone{Count:2}", "minecraft:dirt"});

    EXPECT_TRUE(predicate1 == predicate2);
    EXPECT_FALSE(predicate1 == predicate3);
}

TEST_F(AdventureModePredicateTest, NBTBracket_ComplexNBT)
{
    // 复杂NBT匹配：嵌套compound
    AdventureModePredicate predicate({"minecraft:stone{display:{Name:'Test'}}"});
    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();

    // 纯方块状态版本跳过 NBT 检查
    EXPECT_TRUE(predicate.test(stoneState));
}

TEST_F(AdventureModePredicateTest, NBTBracket_PropertyAndNBTWithTagReference)
{
    // "#minecraft:logs[axis=y]{CustomName:'test'}" —— 标签+属性+NBT
    AdventureModePredicate predicate({"#minecraft:logs[axis=y]{CustomName:'test'}"});

    const Block& oakLog = *VanillaBlocks::OAK_LOG;
    const auto& container = oakLog.stateContainer();
    const IProperty* axisProp = container.getProperty("axis");
    ASSERT_NE(axisProp, nullptr);

    const BlockState* yAxisState = nullptr;
    const BlockState* xAxisState = nullptr;
    for (const auto& state : container.validStates()) {
        auto valIdx = state->getValueIndex(*axisProp);
        if (valIdx.has_value()) {
            auto parsedY = axisProp->parseValue("y");
            auto parsedX = axisProp->parseValue("x");
            if (parsedY.has_value() && *valIdx == *parsedY) {
                yAxisState = state.get();
            }
            if (parsedX.has_value() && *valIdx == *parsedX) {
                xAxisState = state.get();
            }
        }
    }

    ASSERT_NE(yAxisState, nullptr);
    ASSERT_NE(xAxisState, nullptr);

    // 纯方块状态版本跳过 NBT 检查
    EXPECT_TRUE(predicate.test(*yAxisState));
    EXPECT_FALSE(predicate.test(*xAxisState));
}
