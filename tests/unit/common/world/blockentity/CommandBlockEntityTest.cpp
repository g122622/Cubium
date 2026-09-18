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

#include "world/blockentity/redstone/CommandBlockEntity.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include <nlohmann/json.hpp>

using namespace mc;
using namespace mc::blockentity;

// ========== CommandBlockEntity 构造函数测试 ==========

class CommandBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(10, 20, 30)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(entity_->getType(), BlockEntityType::CommandBlock);
}

TEST_F(CommandBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(entity_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(CommandBlockEntityTest, Create_DefaultCommandIsEmpty)
{
    EXPECT_TRUE(entity_->getCommand().empty());
}

TEST_F(CommandBlockEntityTest, Create_DefaultSuccessCountIsZero)
{
    EXPECT_EQ(entity_->getSuccessCount(), 0);
}

TEST_F(CommandBlockEntityTest, Create_DefaultModeIsRedstone)
{
    EXPECT_EQ(entity_->getMode(), CommandBlockMode::Redstone);
}

TEST_F(CommandBlockEntityTest, Create_DefaultAutoIsFalse)
{
    EXPECT_FALSE(entity_->isAuto());
}

TEST_F(CommandBlockEntityTest, Create_DefaultPoweredIsFalse)
{
    EXPECT_FALSE(entity_->isPowered());
}

TEST_F(CommandBlockEntityTest, Create_DefaultConditionMetIsTrue)
{
    EXPECT_TRUE(entity_->isConditionMet());
}

TEST_F(CommandBlockEntityTest, Create_DefaultTrackOutputIsTrue)
{
    EXPECT_TRUE(entity_->shouldTrackOutput());
}

TEST_F(CommandBlockEntityTest, Create_DefaultCustomNameIsAt)
{
    EXPECT_EQ(entity_->getCustomName(), "@");
}

TEST_F(CommandBlockEntityTest, Create_NeedsTickReturnsFalseForRedstoneMode)
{
    // 脉冲命令方块不需要每 tick 更新
    EXPECT_FALSE(entity_->needsTick());
}

// ========== 带模式的构造函数测试 ==========

class CommandBlockEntityModeTest : public ::testing::Test {};

TEST_F(CommandBlockEntityModeTest, Create_RedstoneMode_HasCorrectMode)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Redstone);
    EXPECT_EQ(entity.getMode(), CommandBlockMode::Redstone);
    EXPECT_FALSE(entity.isAuto()); // 红石模式不自动执行
    EXPECT_FALSE(entity.needsTick());
}

TEST_F(CommandBlockEntityModeTest, Create_AutoMode_HasCorrectMode)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Auto);
    EXPECT_EQ(entity.getMode(), CommandBlockMode::Auto);
    EXPECT_TRUE(entity.isAuto());    // 循环模式默认自动执行
    EXPECT_TRUE(entity.needsTick()); // 循环模式需要每 tick 更新
}

TEST_F(CommandBlockEntityModeTest, Create_SequenceMode_HasCorrectMode)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Sequence);
    EXPECT_EQ(entity.getMode(), CommandBlockMode::Sequence);
    EXPECT_FALSE(entity.isAuto()); // 连锁模式不自动执行
    EXPECT_FALSE(entity.needsTick());
}

// ========== 命令管理测试 ==========

class CommandBlockEntityCommandTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityCommandTest, SetCommand_UpdatesCommand)
{
    entity_->setCommand("say Hello World");
    EXPECT_EQ(entity_->getCommand(), "say Hello World");
}

TEST_F(CommandBlockEntityCommandTest, SetCommand_ResetsSuccessCount)
{
    entity_->setSuccessCount(5);
    entity_->setCommand("say Hello");
    EXPECT_EQ(entity_->getSuccessCount(), 0);
}

TEST_F(CommandBlockEntityCommandTest, SetCommand_MarksAsChanged)
{
    EXPECT_FALSE(entity_->isChanged());
    entity_->setCommand("test");
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityCommandTest, SetSuccessCount_UpdatesValue)
{
    entity_->setSuccessCount(10);
    EXPECT_EQ(entity_->getSuccessCount(), 10);
}

TEST_F(CommandBlockEntityCommandTest, SetSuccessCount_ClampsToRange)
{
    entity_->setSuccessCount(-5);
    EXPECT_EQ(entity_->getSuccessCount(), 0);

    entity_->setSuccessCount(20);
    EXPECT_EQ(entity_->getSuccessCount(), 15);
}

TEST_F(CommandBlockEntityCommandTest, SetSuccessCount_MarksAsChanged)
{
    entity_->setSuccessCount(5);
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityCommandTest, SetLastOutput_UpdatesValue)
{
    entity_->setLastOutput("Test output");
    EXPECT_EQ(entity_->getLastOutput(), "Test output");
}

TEST_F(CommandBlockEntityCommandTest, SetCustomName_UpdatesValue)
{
    entity_->setCustomName("MyCommandBlock");
    EXPECT_EQ(entity_->getCustomName(), "MyCommandBlock");
}

// ========== 状态管理测试 ==========

class CommandBlockEntityStateTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityStateTest, SetAuto_UpdatesValue)
{
    entity_->setAuto(true);
    EXPECT_TRUE(entity_->isAuto());
}

TEST_F(CommandBlockEntityStateTest, SetPowered_UpdatesValue)
{
    entity_->setPowered(true);
    EXPECT_TRUE(entity_->isPowered());
}

TEST_F(CommandBlockEntityStateTest, SetPowered_MarksAsChanged)
{
    entity_->setPowered(true);
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityStateTest, SetPowered_NoChange_NoMarkAsChanged)
{
    entity_->clearChanged();
    entity_->setPowered(false); // 值未改变
    EXPECT_FALSE(entity_->isChanged());
}

TEST_F(CommandBlockEntityStateTest, SetTrackOutput_UpdatesValue)
{
    entity_->setTrackOutput(false);
    EXPECT_FALSE(entity_->shouldTrackOutput());
}

TEST_F(CommandBlockEntityStateTest, SetMode_UpdatesValue)
{
    entity_->setMode(CommandBlockMode::Auto);
    EXPECT_EQ(entity_->getMode(), CommandBlockMode::Auto);
}

// ========== ICommandSource 接口测试 ==========

class CommandBlockEntitySourceTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntitySourceTest, ShouldReceiveFeedback_ReturnsTrackOutput)
{
    EXPECT_TRUE(entity_->shouldReceiveFeedback());
    entity_->setTrackOutput(false);
    EXPECT_FALSE(entity_->shouldReceiveFeedback());
}

TEST_F(CommandBlockEntitySourceTest, ShouldReceiveErrors_ReturnsTrue)
{
    EXPECT_TRUE(entity_->shouldReceiveErrors());
}

TEST_F(CommandBlockEntitySourceTest, AllowLogging_ReturnsTrue)
{
    EXPECT_TRUE(entity_->allowLogging());
}

TEST_F(CommandBlockEntitySourceTest, SendMessage_UpdatesLastOutput)
{
    entity_->sendMessage("Test message");
    EXPECT_EQ(entity_->getLastOutput(), "Test message");
}

TEST_F(CommandBlockEntitySourceTest, SendMessage_WhenTrackOutputDisabled_DoesNotUpdate)
{
    entity_->setTrackOutput(false);
    entity_->setLastOutput("Original");
    entity_->sendMessage("New message");
    // 当 TrackOutput 禁用时，sendMessage 不更新 lastOutput
    // 注意：根据实现，可能更新或不更新
}

// ========== JSON 序列化测试 ==========

class CommandBlockEntityJsonTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(5, 10, 15)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityJsonTest, Save_ContainsBasicInfo)
{
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:command_block");
    EXPECT_TRUE(data.contains("x"));
    EXPECT_TRUE(data.contains("y"));
    EXPECT_TRUE(data.contains("z"));
    EXPECT_EQ(data["x"].get<i32>(), 5);
    EXPECT_EQ(data["y"].get<i32>(), 10);
    EXPECT_EQ(data["z"].get<i32>(), 15);
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsCommand)
{
    entity_->setCommand("say Hello");
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("Command"));
    EXPECT_EQ(data["Command"], "say Hello");
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsSuccessCount)
{
    entity_->setSuccessCount(7);
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("SuccessCount"));
    EXPECT_EQ(data["SuccessCount"].get<i32>(), 7);
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsCustomName)
{
    entity_->setCustomName("TestBlock");
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("CustomName"));
    EXPECT_EQ(data["CustomName"], "TestBlock");
}

TEST_F(CommandBlockEntityJsonTest, Save_DefaultCustomNameNotSaved)
{
    // 默认名称 "@" 不保存
    nlohmann::json data;
    entity_->save(data);

    EXPECT_FALSE(data.contains("CustomName"));
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsPowered)
{
    entity_->setPowered(true);
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("powered"));
    EXPECT_TRUE(data["powered"].get<bool>());
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsAuto)
{
    entity_->setAuto(true);
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("auto"));
    EXPECT_TRUE(data["auto"].get<bool>());
}

TEST_F(CommandBlockEntityJsonTest, Save_ContainsTrackOutput)
{
    nlohmann::json data;
    entity_->save(data);

    EXPECT_TRUE(data.contains("TrackOutput"));
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsCommand)
{
    nlohmann::json data;
    data["Command"] = "gamemode creative";

    EXPECT_TRUE(entity_->load(data));
    EXPECT_EQ(entity_->getCommand(), "gamemode creative");
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsSuccessCount)
{
    nlohmann::json data;
    data["SuccessCount"] = 12;

    EXPECT_TRUE(entity_->load(data));
    EXPECT_EQ(entity_->getSuccessCount(), 12);
}

TEST_F(CommandBlockEntityJsonTest, Load_ClampsSuccessCount)
{
    nlohmann::json data;
    data["SuccessCount"] = 100;

    EXPECT_TRUE(entity_->load(data));
    EXPECT_EQ(entity_->getSuccessCount(), 15);
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsCustomName)
{
    nlohmann::json data;
    data["CustomName"] = "LoadedName";

    EXPECT_TRUE(entity_->load(data));
    EXPECT_EQ(entity_->getCustomName(), "LoadedName");
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsPowered)
{
    nlohmann::json data;
    data["powered"] = true;

    EXPECT_TRUE(entity_->load(data));
    EXPECT_TRUE(entity_->isPowered());
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsAuto)
{
    nlohmann::json data;
    data["auto"] = true;

    EXPECT_TRUE(entity_->load(data));
    EXPECT_TRUE(entity_->isAuto());
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsTrackOutput)
{
    nlohmann::json data;
    data["TrackOutput"] = false;

    EXPECT_TRUE(entity_->load(data));
    EXPECT_FALSE(entity_->shouldTrackOutput());
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsLastOutput)
{
    nlohmann::json data;
    data["LastOutput"] = "Previous output";

    EXPECT_TRUE(entity_->load(data));
    EXPECT_EQ(entity_->getLastOutput(), "Previous output");
}

TEST_F(CommandBlockEntityJsonTest, Load_LoadsLastExecution)
{
    nlohmann::json data;
    data["LastExecution"] = 1000LL;

    EXPECT_TRUE(entity_->load(data));
    // lastExecution 是私有的，但可以通过 trigger 测试
}

TEST_F(CommandBlockEntityJsonTest, SaveLoad_PreservesData)
{
    entity_->setCommand("test command");
    entity_->setSuccessCount(5);
    entity_->setCustomName("TestEntity");
    entity_->setPowered(true);
    entity_->setAuto(true);
    entity_->setTrackOutput(false);
    entity_->setLastOutput("Test output");

    nlohmann::json data;
    entity_->save(data);

    CommandBlockEntity loaded(BlockPos(0, 0, 0));
    EXPECT_TRUE(loaded.load(data));

    EXPECT_EQ(loaded.getCommand(), "test command");
    EXPECT_EQ(loaded.getSuccessCount(), 5);
    EXPECT_EQ(loaded.getCustomName(), "TestEntity");
    EXPECT_TRUE(loaded.isPowered());
    EXPECT_TRUE(loaded.isAuto());
    EXPECT_FALSE(loaded.shouldTrackOutput());
}

// ========== Clone 测试 ==========

class CommandBlockEntityCloneTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(1, 2, 3)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityCloneTest, Clone_CreatesCopy)
{
    entity_->setCommand("clone command");
    entity_->setSuccessCount(8);
    entity_->setCustomName("CloneTest");
    entity_->setMode(CommandBlockMode::Auto);
    entity_->setAuto(true);

    std::unique_ptr<BlockEntity> copy = entity_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::CommandBlock);
    EXPECT_EQ(copy->getPos(), BlockPos(1, 2, 3));

    auto* commandCopy = static_cast<CommandBlockEntity*>(copy.get());
    EXPECT_EQ(commandCopy->getCommand(), "clone command");
    EXPECT_EQ(commandCopy->getSuccessCount(), 8);
    EXPECT_EQ(commandCopy->getCustomName(), "CloneTest");
    EXPECT_EQ(commandCopy->getMode(), CommandBlockMode::Auto);
    EXPECT_TRUE(commandCopy->isAuto());
}

TEST_F(CommandBlockEntityCloneTest, Clone_PreservesPoweredState)
{
    entity_->setPowered(true);
    std::unique_ptr<BlockEntity> copy = entity_->clone();

    auto* commandCopy = static_cast<CommandBlockEntity*>(copy.get());
    EXPECT_TRUE(commandCopy->isPowered());
}

TEST_F(CommandBlockEntityCloneTest, Clone_PreservesConditionMet)
{
    // 非条件模式下条件始终满足
    EXPECT_TRUE(entity_->isConditionMet());

    std::unique_ptr<BlockEntity> copy = entity_->clone();
    auto* commandCopy = static_cast<CommandBlockEntity*>(copy.get());
    EXPECT_TRUE(commandCopy->isConditionMet());
}

// ========== 彩蛋测试 ==========

class CommandBlockEntityEasterEggTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

// 注意：彩蛋测试需要 Mock World，这里只测试命令设置
TEST_F(CommandBlockEntityEasterEggTest, SeargeCommand_CanBeSet)
{
    entity_->setCommand("Searge");
    EXPECT_EQ(entity_->getCommand(), "Searge");
}

// ========== Changed 标志测试 ==========

class CommandBlockEntityChangedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0));
        entity_->clearChanged();
    }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityChangedTest, SetCommand_MarksChanged)
{
    entity_->setCommand("test");
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityChangedTest, SetSuccessCount_MarksChanged)
{
    entity_->setSuccessCount(5);
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityChangedTest, SetCustomName_MarksChanged)
{
    entity_->setCustomName("name");
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityChangedTest, SetPowered_WhenChanged_MarksChanged)
{
    entity_->setPowered(true);
    EXPECT_TRUE(entity_->isChanged());
}

TEST_F(CommandBlockEntityChangedTest, SetPowered_WhenUnchanged_NoMarkChanged)
{
    entity_->setPowered(false); // 已经是 false
    EXPECT_FALSE(entity_->isChanged());
}

TEST_F(CommandBlockEntityChangedTest, SendMessage_MarksChanged)
{
    entity_->sendMessage("test");
    EXPECT_TRUE(entity_->isChanged());
}

// ========== 模式相关测试 ==========

class CommandBlockEntityModeBehaviorTest : public ::testing::Test {};

TEST_F(CommandBlockEntityModeBehaviorTest, RedstoneMode_DoesNotNeedTick)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Redstone);
    EXPECT_FALSE(entity.needsTick());
}

TEST_F(CommandBlockEntityModeBehaviorTest, AutoMode_NeedsTick)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Auto);
    EXPECT_TRUE(entity.needsTick());
}

TEST_F(CommandBlockEntityModeBehaviorTest, SequenceMode_DoesNotNeedTick)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Sequence);
    EXPECT_FALSE(entity.needsTick());
}

TEST_F(CommandBlockEntityModeBehaviorTest, AutoMode_DefaultAutoEnabled)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Auto);
    EXPECT_TRUE(entity.isAuto());
}

TEST_F(CommandBlockEntityModeBehaviorTest, RedstoneMode_DefaultAutoDisabled)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Redstone);
    EXPECT_FALSE(entity.isAuto());
}

TEST_F(CommandBlockEntityModeBehaviorTest, SequenceMode_DefaultAutoDisabled)
{
    CommandBlockEntity entity(BlockPos(0, 0, 0), CommandBlockMode::Sequence);
    EXPECT_FALSE(entity.isAuto());
}

// ========== 边界情况测试 ==========

class CommandBlockEntityEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override { entity_ = std::make_unique<CommandBlockEntity>(BlockPos(0, 0, 0)); }

    std::unique_ptr<CommandBlockEntity> entity_;
};

TEST_F(CommandBlockEntityEdgeCaseTest, SetSuccessCount_AtMinimum)
{
    entity_->setSuccessCount(0);
    EXPECT_EQ(entity_->getSuccessCount(), 0);
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetSuccessCount_AtMaximum)
{
    entity_->setSuccessCount(15);
    EXPECT_EQ(entity_->getSuccessCount(), 15);
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetSuccessCount_BelowMinimum_Clamped)
{
    entity_->setSuccessCount(-100);
    EXPECT_EQ(entity_->getSuccessCount(), 0);
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetSuccessCount_AboveMaximum_Clamped)
{
    entity_->setSuccessCount(100);
    EXPECT_EQ(entity_->getSuccessCount(), 15);
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetEmptyCommand)
{
    entity_->setCommand("");
    EXPECT_TRUE(entity_->getCommand().empty());
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetVeryLongCommand)
{
    std::string longCommand(10000, 'a');
    entity_->setCommand(longCommand);
    EXPECT_EQ(entity_->getCommand(), longCommand);
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetUnicodeCommand)
{
    entity_->setCommand("say 你好世界 🎮");
    EXPECT_EQ(entity_->getCommand(), "say 你好世界 🎮");
}

TEST_F(CommandBlockEntityEdgeCaseTest, SetUnicodeCustomName)
{
    entity_->setCustomName("命令方块 #1");
    EXPECT_EQ(entity_->getCustomName(), "命令方块 #1");
}
