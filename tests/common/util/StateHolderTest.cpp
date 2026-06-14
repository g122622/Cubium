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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/util/property/StateHolder.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/IntegerProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

// 测试用方块：拥有 facing(4方向) + lit(布尔) 两个属性
class TestStateBlock : public Block {
public:
    TestStateBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this)
                             .add(BlockStateProperties::HORIZONTAL_FACING())
                             .add(BlockStateProperties::LIT())
                             .create([](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                                 return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
                             });
        createBlockState(std::move(container));
        setDefaultState(defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                .with(BlockStateProperties::LIT(), false));
    }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

// 简单方块：无属性
class SimpleBlock : public Block {
public:
    SimpleBlock()
        : Block(BlockProperties(Material::ROCK).hardness(1.0f))
    {
        auto container = StateContainer<Block, BlockState>::Builder(*this).create(
            [](const Block& block, auto values, auto layouts, auto allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), layouts, allStates, id);
            });
        createBlockState(std::move(container));
    }

    void fillStateContainer(StateContainer<Block, BlockState>& /*container*/) override {}
};

} // namespace

class StateHolderWithValueIndexTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 手动注册方块到全局注册表以确保 BlockState 可正常工作
        // 但对于 StateHolder 测试来说，只需要 Block 和 StateContainer 即可
    }
};

// ========== withValueIndex 基本测试 ==========

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_SetsBooleanProperty)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();
    const BlockState& defaultState = block.defaultState();

    // 默认状态：facing=north, lit=false
    EXPECT_FALSE(defaultState.get(BlockStateProperties::LIT()));

    // 使用 withValueIndex 将 lit 设为 true (index=1)
    const IProperty* litProp = container.getProperty("lit");
    ASSERT_NE(litProp, nullptr);

    auto litTrueIndex = litProp->parseValue("true");
    ASSERT_TRUE(litTrueIndex.has_value());

    const BlockState& litState = defaultState.withValueIndex(*litProp, *litTrueIndex);
    EXPECT_TRUE(litState.get(BlockStateProperties::LIT()));
    // facing 应保持不变
    EXPECT_EQ(litState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_SetsEnumProperty)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();
    const BlockState& defaultState = block.defaultState();

    // 默认状态：facing=north
    EXPECT_EQ(defaultState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::North);

    // 使用 withValueIndex 将 facing 设为 east
    const IProperty* facingProp = container.getProperty("facing");
    ASSERT_NE(facingProp, nullptr);

    auto eastIndex = facingProp->parseValue("east");
    ASSERT_TRUE(eastIndex.has_value());

    const BlockState& eastState = defaultState.withValueIndex(*facingProp, *eastIndex);
    EXPECT_EQ(eastState.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::East);
    // lit 应保持不变
    EXPECT_FALSE(eastState.get(BlockStateProperties::LIT()));
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_ChainedPropertySetting)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();
    const BlockState& defaultState = block.defaultState();

    const IProperty* facingProp = container.getProperty("facing");
    const IProperty* litProp = container.getProperty("lit");
    ASSERT_NE(facingProp, nullptr);
    ASSERT_NE(litProp, nullptr);

    // 连续设置两个属性
    auto southIndex = facingProp->parseValue("south");
    auto litTrueIndex = litProp->parseValue("true");
    ASSERT_TRUE(southIndex.has_value());
    ASSERT_TRUE(litTrueIndex.has_value());

    const BlockState& result =
        defaultState.withValueIndex(*facingProp, *southIndex).withValueIndex(*litProp, *litTrueIndex);

    EXPECT_EQ(result.get(BlockStateProperties::HORIZONTAL_FACING()), Direction::South);
    EXPECT_TRUE(result.get(BlockStateProperties::LIT()));
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_SameValueReturnsSameState)
{
    TestStateBlock block;
    const BlockState& defaultState = block.defaultState();
    const auto& container = block.stateContainer();

    const IProperty* litProp = container.getProperty("lit");
    ASSERT_NE(litProp, nullptr);

    // lit 默认为 false (index=0)，设置 index=0 应返回同一状态
    const BlockState& sameState = defaultState.withValueIndex(*litProp, 0);
    EXPECT_EQ(&sameState, &defaultState);
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_InvalidIndexReturnsCurrentState)
{
    TestStateBlock block;
    const BlockState& defaultState = block.defaultState();
    const auto& container = block.stateContainer();

    const IProperty* litProp = container.getProperty("lit");
    ASSERT_NE(litProp, nullptr);

    // lit 只有 2 个值 (0=false, 1=true)，index=99 超出范围应返回当前状态
    const BlockState& result = defaultState.withValueIndex(*litProp, 99);
    EXPECT_EQ(&result, &defaultState);
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_PropertyNotFoundReturnsCurrentState)
{
    TestStateBlock block;
    const BlockState& defaultState = block.defaultState();

    // 使用不属于此方块的属性（例如 AGE_0_15）
    const auto& ageProp = BlockStateProperties::AGE_0_15();

    // withValueIndex 对不存在的属性应返回当前状态
    const BlockState& result = defaultState.withValueIndex(ageProp, 0);
    EXPECT_EQ(&result, &defaultState);
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_NoPropertyBlockReturnsSameState)
{
    SimpleBlock block;
    const BlockState& defaultState = block.defaultState();

    // 无属性方块，任何属性设置都应返回当前状态
    const auto& litProp = BlockStateProperties::LIT();
    const BlockState& result = defaultState.withValueIndex(litProp, 1);
    EXPECT_EQ(&result, &defaultState);
}

TEST_F(StateHolderWithValueIndexTest, WithValueIndex_ConsistentWithTypedWith)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();
    const BlockState& defaultState = block.defaultState();

    // 通过 withValueIndex 设置
    const IProperty* facingProp = container.getProperty("facing");
    ASSERT_NE(facingProp, nullptr);
    auto westIndex = facingProp->parseValue("west");
    ASSERT_TRUE(westIndex.has_value());
    const BlockState& viaValueIndex = defaultState.withValueIndex(*facingProp, *westIndex);

    // 通过类型安全的 with() 设置
    const BlockState& viaWith = defaultState.with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);

    // 两种方式的结果应该完全一致
    EXPECT_EQ(viaValueIndex.stateId(), viaWith.stateId());
    EXPECT_EQ(&viaValueIndex, &viaWith);
}

// ========== StateContainer::getProperty 测试 ==========

TEST_F(StateHolderWithValueIndexTest, GetProperty_ReturnsNullForUnknownName)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();

    // 不存在的属性名应返回 nullptr
    EXPECT_EQ(container.getProperty("nonexistent"), nullptr);
    EXPECT_EQ(container.getProperty("age"), nullptr);
    EXPECT_EQ(container.getProperty("power"), nullptr);
}

TEST_F(StateHolderWithValueIndexTest, GetProperty_ReturnsValidForExistingName)
{
    TestStateBlock block;
    const auto& container = block.stateContainer();

    // 已注册的属性应返回非空指针
    EXPECT_NE(container.getProperty("facing"), nullptr);
    EXPECT_NE(container.getProperty("lit"), nullptr);
}

// ========== IProperty::parseValue 测试 ==========

TEST_F(StateHolderWithValueIndexTest, ParseValue_BooleanProperty)
{
    const auto& litProp = BlockStateProperties::LIT();

    auto trueIdx = litProp.parseValue("true");
    auto falseIdx = litProp.parseValue("false");
    auto invalidIdx = litProp.parseValue("maybe");

    EXPECT_TRUE(trueIdx.has_value());
    EXPECT_TRUE(falseIdx.has_value());
    EXPECT_FALSE(invalidIdx.has_value());

    EXPECT_EQ(litProp.valueToString(*trueIdx), "true");
    EXPECT_EQ(litProp.valueToString(*falseIdx), "false");
}

TEST_F(StateHolderWithValueIndexTest, ParseValue_IntegerProperty)
{
    auto ageProp = IntegerProperty::create("age", 0, 15);

    auto zeroIdx = ageProp->parseValue("0");
    auto fiveIdx = ageProp->parseValue("5");
    auto fifteenIdx = ageProp->parseValue("15");
    auto negativeIdx = ageProp->parseValue("-1");
    auto tooLargeIdx = ageProp->parseValue("16");
    auto notNumberIdx = ageProp->parseValue("abc");

    EXPECT_TRUE(zeroIdx.has_value());
    EXPECT_TRUE(fiveIdx.has_value());
    EXPECT_TRUE(fifteenIdx.has_value());
    EXPECT_FALSE(negativeIdx.has_value());
    EXPECT_FALSE(tooLargeIdx.has_value());
    EXPECT_FALSE(notNumberIdx.has_value());
}
