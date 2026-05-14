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

#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace command {
namespace test {

/**
 * @brief BlockStateArgumentType 单元测试
 *
 * 测试方块状态参数解析功能，包括：
 * - 基本方块 ID 解析
 * - 带命名空间的方块 ID 解析
 * - 方块状态属性解析
 * - 错误处理
 */
class BlockStateArgumentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }

    void SetUp() override
    {
        // 每个测试前的设置
    }
};

// ========== 基本参数类型测试 ==========

TEST_F(BlockStateArgumentTest, GetTypeName)
{
    BlockStateArgumentType argType;
    EXPECT_EQ(argType.getTypeName(), "block_state");
}

TEST_F(BlockStateArgumentTest, GetExamples)
{
    BlockStateArgumentType argType;
    auto examples = argType.getExamples();

    EXPECT_FALSE(examples.empty());
    EXPECT_EQ(examples.size(), 4); // 4 个示例

    // 检查示例格式
    bool hasSimpleExample = false;
    bool hasPropertyExample = false;
    for (const auto& ex : examples) {
        if (ex.find('[') == std::string::npos) {
            hasSimpleExample = true;
        } else {
            hasPropertyExample = true;
        }
    }
    EXPECT_TRUE(hasSimpleExample);
    EXPECT_TRUE(hasPropertyExample);
}

// ========== 简单方块ID解析测试 ==========

TEST_F(BlockStateArgumentTest, ParseSimpleBlockId)
{
    StringReader reader("stone");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::STONE);
}

TEST_F(BlockStateArgumentTest, ParseBlockIdWithNamespace)
{
    StringReader reader("minecraft:stone");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::STONE);
}

TEST_F(BlockStateArgumentTest, ParseUnknownBlockThrowsError)
{
    StringReader reader("minecraft:nonexistent_block_xyz");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

// ========== 方块状态属性解析测试 ==========

TEST_F(BlockStateArgumentTest, ParseBlockWithSingleProperty)
{
    // 跳过测试如果 oak_door 方块未注册
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    StringReader reader("oak_door[facing=north]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_DOOR);
}

TEST_F(BlockStateArgumentTest, ParseBlockWithMultipleProperties)
{
    // 跳过测试如果 oak_stairs 方块未注册
    if (VanillaBlocks::OAK_STAIRS == nullptr) {
        GTEST_SKIP() << "OAK_STAIRS block not registered";
    }

    // StairsBlock uses 'half' property with values 'upper' and 'lower' (DoubleBlockHalf enum)
    StringReader reader("oak_stairs[facing=east,half=upper]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_STAIRS);
}

TEST_F(BlockStateArgumentTest, ParseBlockWithThreeProperties)
{
    // 跳过测试如果 oak_stairs 方块未注册
    if (VanillaBlocks::OAK_STAIRS == nullptr) {
        GTEST_SKIP() << "OAK_STAIRS block not registered";
    }

    // StairsBlock uses 'half' property with values 'upper' and 'lower'
    StringReader reader("oak_stairs[facing=south,half=lower,waterlogged=false]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_STAIRS);
}

TEST_F(BlockStateArgumentTest, ParseBlockWithNamespaceAndProperties)
{
    // 跳过测试如果 oak_door 方块未注册
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    StringReader reader("minecraft:oak_door[facing=south]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_DOOR);
}

// ========== 错误处理测试 ==========

TEST_F(BlockStateArgumentTest, InvalidPropertyThrowsError)
{
    // 跳过测试如果 stone 方块未注册
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE block not registered";
    }

    // 石头没有 facing 属性
    StringReader reader("stone[facing=north]");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, InvalidPropertyValueThrowsError)
{
    // 跳过测试如果 oak_door 方块未注册
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    // facing 属性不接受 "invalid_value"
    StringReader reader("oak_door[facing=invalid_value]");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, DuplicatePropertyThrowsError)
{
    // 跳过测试如果 oak_door 方块未注册
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    // 重复的 facing 属性
    StringReader reader("oak_door[facing=north,facing=south]");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, MissingPropertyValueThrowsError)
{
    StringReader reader("oak_door[facing=]");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, MissingPropertyNameThrowsError)
{
    StringReader reader("oak_door[=north]");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, MissingClosingBracketReturnsDefaultState)
{
    // 当缺少闭合括号时，解析器会返回方块的默认状态
    // 因为它会读取整个字符串作为方块名
    StringReader reader("oak_door[facing=north");
    BlockStateArgumentType argType;

    // 不抛出异常，返回默认状态
    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
}

TEST_F(BlockStateArgumentTest, EmptyBracketsReturnsDefaultState)
{
    // 空括号返回方块的默认状态
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    StringReader reader("oak_door[]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_DOOR);
}

TEST_F(BlockStateArgumentTest, TrailingCommaInBracketsReturnsValidState)
{
    // 尾随逗号是允许的，解析器会忽略它
    if (VanillaBlocks::OAK_DOOR == nullptr) {
        GTEST_SKIP() << "OAK_DOOR block not registered";
    }

    StringReader reader("oak_door[facing=north,]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_DOOR);
}

// ========== BlockStateInput 测试 ==========

TEST_F(BlockStateArgumentTest, BlockStateInputDefaultState)
{
    BlockStateInput input;
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.state(), nullptr);
}

TEST_F(BlockStateArgumentTest, BlockStateInputWithNullState)
{
    BlockStateInput input(nullptr);
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.state(), nullptr);
}

TEST_F(BlockStateArgumentTest, BlockStateInputWithValidState)
{
    // 跳过测试如果 stone 方块未注册
    if (VanillaBlocks::STONE == nullptr) {
        GTEST_SKIP() << "STONE block not registered";
    }

    const BlockState* state = &VanillaBlocks::STONE->defaultState();
    BlockStateInput input(state);
    EXPECT_TRUE(input.isValid());
    EXPECT_EQ(input.state(), state);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::STONE);
}

// ========== StringReader 集成测试 ==========

TEST_F(BlockStateArgumentTest, StringReaderIntegration)
{
    // 测试 StringReader 的基本读取功能
    StringReader reader("stone");
    EXPECT_TRUE(reader.canRead());
    EXPECT_EQ(reader.peek(), 's');

    std::string result = reader.readString();
    EXPECT_EQ(result, "stone");
    EXPECT_FALSE(reader.canRead()); // 读取完毕
}

// ========== 静态工厂方法测试 ==========

TEST_F(BlockStateArgumentTest, StaticFactoryMethod)
{
    auto argType = BlockStateArgumentType::blockState();
    EXPECT_NE(argType, nullptr);
    EXPECT_EQ(argType->getTypeName(), "block_state");
}

// ========== 边界情况测试 ==========

TEST_F(BlockStateArgumentTest, EmptyStringThrowsError)
{
    StringReader reader("");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, OnlyNamespaceThrowsError)
{
    StringReader reader("minecraft:");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

// ========== 更多方块解析测试 ==========

TEST_F(BlockStateArgumentTest, ParseMultipleBlockTypes)
{
    BlockStateArgumentType argType;

    // 测试各种方块
    {
        StringReader reader("dirt");
        BlockStateInput input = argType.parse(reader);
        EXPECT_TRUE(input.isValid());
        if (VanillaBlocks::DIRT != nullptr) {
            EXPECT_EQ(&input.getBlock(), VanillaBlocks::DIRT);
        }
    }

    {
        StringReader reader("cobblestone");
        BlockStateInput input = argType.parse(reader);
        EXPECT_TRUE(input.isValid());
        if (VanillaBlocks::COBBLESTONE != nullptr) {
            EXPECT_EQ(&input.getBlock(), VanillaBlocks::COBBLESTONE);
        }
    }

    {
        StringReader reader("oak_planks");
        BlockStateInput input = argType.parse(reader);
        EXPECT_TRUE(input.isValid());
        if (VanillaBlocks::OAK_PLANKS != nullptr) {
            EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_PLANKS);
        }
    }
}

TEST_F(BlockStateArgumentTest, ParseBlockStateId)
{
    // 跳过测试如果 oak_stairs 方块未注册
    if (VanillaBlocks::OAK_STAIRS == nullptr) {
        GTEST_SKIP() << "OAK_STAIRS block not registered";
    }

    // StairsBlock uses 'half' property with values 'upper' and 'lower'
    StringReader reader("oak_stairs[facing=west,half=upper,waterlogged=true]");
    BlockStateArgumentType argType;

    BlockStateInput input = argType.parse(reader);
    EXPECT_TRUE(input.isValid());
    EXPECT_NE(input.state(), nullptr);
    EXPECT_EQ(&input.getBlock(), VanillaBlocks::OAK_STAIRS);

    // 验证 stateId 返回有效值
    u32 stateId = input.stateId();
    EXPECT_GT(stateId, 0u);
}

} // namespace test
} // namespace command
} // namespace mc
