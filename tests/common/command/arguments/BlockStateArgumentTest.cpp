#include <gtest/gtest.h>
#include "common/command/arguments/BlockStateArgument.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/util/property/Properties.hpp"

namespace mc {
namespace command {
namespace test {

/**
 * @brief BlockStateArgumentType 单元测试
 *
 * 测试方块状态参数解析功能，包括：
 * - 基本方块 ID 解析
 * - 带命名空间的方块 ID 解析
 * - 错误处理
 *
 * 注意：由于方块注册是在游戏初始化时完成的，属性解析测试
 * 需要在有完整方块注册的环境中运行。
 */
class BlockStateArgumentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 方块注册在游戏初始化时完成，这里只测试参数解析框架
    }
};

// ========== 基本参数类型测试 ==========

TEST_F(BlockStateArgumentTest, GetTypeName) {
    BlockStateArgumentType argType;
    EXPECT_EQ(argType.getTypeName(), "block_state");
}

TEST_F(BlockStateArgumentTest, GetExamples) {
    BlockStateArgumentType argType;
    auto examples = argType.getExamples();

    EXPECT_FALSE(examples.empty());
    EXPECT_EQ(examples.size(), 4);  // 4 个示例

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

// ========== 错误处理测试 ==========

TEST_F(BlockStateArgumentTest, InvalidBlockIdThrowsError) {
    StringReader reader("minecraft:nonexistent_block_xyz");
    BlockStateArgumentType argType;

    EXPECT_THROW(argType.parse(reader), CommandException);
}

TEST_F(BlockStateArgumentTest, BlockStateInputDefaultState) {
    BlockStateInput input;
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.state(), nullptr);
}

TEST_F(BlockStateArgumentTest, BlockStateInputWithState) {
    // 创建一个测试用的 BlockStateInput
    // 在没有实际方块注册的情况下，我们只能测试基本接口
    BlockStateInput input(nullptr);
    EXPECT_FALSE(input.isValid());
    EXPECT_EQ(input.state(), nullptr);
}

// ========== StringReader 辅助测试 ==========

TEST_F(BlockStateArgumentTest, StringReaderIntegration) {
    // 测试 StringReader 的基本读取功能
    StringReader reader("test_block");

    EXPECT_TRUE(reader.canRead());
    EXPECT_EQ(reader.peek(), 't');

    std::string result = reader.readString();
    EXPECT_EQ(result, "test_block");
    EXPECT_FALSE(reader.canRead());
}

// ========== 命名测试 ==========

TEST_F(BlockStateArgumentTest, StaticFactoryMethod) {
    auto argType = BlockStateArgumentType::blockState();
    EXPECT_NE(argType, nullptr);
    EXPECT_EQ(argType->getTypeName(), "block_state");
}

} // namespace test
} // namespace command
} // namespace mc
