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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/util/math/Vector3.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "server/world/ServerWorld.hpp"

using namespace mc;
using namespace mc::server;

/**
 * @brief ServerWorld::executeCommand() 测试固件
 *
 * 测试命令执行回调机制的集成。
 */
class ServerWorldCommandExecuteTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.viewDistance = 10;
        config.dimension = 0;
        world = std::make_unique<ServerWorld>(config);
    }

    void TearDown() override { world.reset(); }

    std::unique_ptr<ServerWorld> world;
};

// ============================================================================
// 基本功能测试
// ============================================================================

TEST_F(ServerWorldCommandExecuteTest, ExecuteCommandWithoutCallbackReturnsZero)
{
    // 当没有设置回调时，executeCommand 应返回 0
    Vector3d position(100.0, 64.0, 200.0);
    i32 result = world->executeCommand("/say hello", position, 2, mc::math::Vector2f(0.0f, 0.0f));
    EXPECT_EQ(result, 0);
}

TEST_F(ServerWorldCommandExecuteTest, ExecuteCommandWithCallback)
{
    // 记录回调参数
    std::string capturedCommand;
    Vector3d capturedPosition(0, 0, 0);
    i32 capturedPermissionLevel = 0;
    i32 callbackResult = 42;

    // 设置回调
    world->setOnExecuteCommand(
        [&](const std::string& command, const Vector3d& position, i32 permissionLevel, const Vector2f&) -> i32 {
            capturedCommand = command;
            capturedPosition = position;
            capturedPermissionLevel = permissionLevel;
            return callbackResult;
        });

    // 执行命令
    Vector3d position(100.5, 64.0, 200.5);
    i32 result = world->executeCommand("/gamemode creative", position, 2, mc::math::Vector2f(0.0f, 0.0f));

    // 验证回调参数
    EXPECT_EQ(result, callbackResult);
    EXPECT_EQ(capturedCommand, "/gamemode creative");
    EXPECT_DOUBLE_EQ(capturedPosition.x, 100.5);
    EXPECT_DOUBLE_EQ(capturedPosition.y, 64.0);
    EXPECT_DOUBLE_EQ(capturedPosition.z, 200.5);
    EXPECT_EQ(capturedPermissionLevel, 2);
}

TEST_F(ServerWorldCommandExecuteTest, ExecuteCommandWithDifferentPermissionLevels)
{
    // 测试不同权限级别
    for (i32 level = 0; level <= 4; ++level) {
        i32 capturedLevel = -1;

        world->setOnExecuteCommand(
            [&](const std::string&, const Vector3d&, i32 permissionLevel, const Vector2f&) -> i32 {
                capturedLevel = permissionLevel;
                return 1;
            });

        Vector3d position(0, 0, 0);
        i32 result = world->executeCommand("/test", position, level, mc::math::Vector2f(0.0f, 0.0f));

        EXPECT_EQ(result, 1);
        EXPECT_EQ(capturedLevel, level);
    }
}

TEST_F(ServerWorldCommandExecuteTest, ExecuteEmptyCommand)
{
    std::string capturedCommand;
    bool callbackCalled = false;

    world->setOnExecuteCommand([&](const std::string& command, const Vector3d&, i32, const Vector2f&) -> i32 {
        capturedCommand = command;
        callbackCalled = true;
        return 0;
    });

    Vector3d position(0, 0, 0);
    i32 result = world->executeCommand("", position, 2, mc::math::Vector2f(0.0f, 0.0f));

    // 空命令仍应调用回调
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(capturedCommand, "");
    EXPECT_EQ(result, 0);
}

TEST_F(ServerWorldCommandExecuteTest, ExecuteCommandWithSlashPrefix)
{
    // 测试带 '/' 前缀的命令
    std::string capturedCommand;

    world->setOnExecuteCommand([&](const std::string& command, const Vector3d&, i32, const Vector2f&) -> i32 {
        capturedCommand = command;
        return 1;
    });

    Vector3d position(0, 0, 0);
    world->executeCommand("/time set day", position, 2, mc::math::Vector2f(0.0f, 0.0f));

    // 命令应原样传递（包含 '/' 前缀）
    EXPECT_EQ(capturedCommand, "/time set day");
}

TEST_F(ServerWorldCommandExecuteTest, ExecuteCommandWithoutSlashPrefix)
{
    // 测试不带 '/' 前缀的命令
    std::string capturedCommand;

    world->setOnExecuteCommand([&](const std::string& command, const Vector3d&, i32, const Vector2f&) -> i32 {
        capturedCommand = command;
        return 1;
    });

    Vector3d position(0, 0, 0);
    world->executeCommand("time set day", position, 2, mc::math::Vector2f(0.0f, 0.0f));

    // 命令应原样传递（不含 '/' 前缀）
    EXPECT_EQ(capturedCommand, "time set day");
}

TEST_F(ServerWorldCommandExecuteTest, CallbackReturnsFailureCode)
{
    // 测试回调返回失败代码
    world->setOnExecuteCommand([&](const std::string&, const Vector3d&, i32, const Vector2f&) -> i32 {
        return 0; // 失败
    });

    Vector3d position(0, 0, 0);
    i32 result = world->executeCommand("/invalid_command", position, 2, mc::math::Vector2f(0.0f, 0.0f));
    EXPECT_EQ(result, 0);
}

TEST_F(ServerWorldCommandExecuteTest, MultipleCommandsSequential)
{
    // 测试连续执行多个命令
    std::vector<std::string> executedCommands;
    std::vector<i32> permissionLevels;

    world->setOnExecuteCommand(
        [&](const std::string& command, const Vector3d&, i32 permissionLevel, const Vector2f&) -> i32 {
            executedCommands.push_back(command);
            permissionLevels.push_back(permissionLevel);
            return static_cast<i32>(executedCommands.size());
        });

    Vector3d position(0, 0, 0);

    world->executeCommand("/say hello", position, 2, mc::math::Vector2f(0.0f, 0.0f));
    world->executeCommand("/time set day", position, 2, mc::math::Vector2f(0.0f, 0.0f));
    world->executeCommand("/gamemode survival", position, 4, mc::math::Vector2f(0.0f, 0.0f));

    EXPECT_EQ(executedCommands.size(), 3u);
    EXPECT_EQ(executedCommands[0], "/say hello");
    EXPECT_EQ(executedCommands[1], "/time set day");
    EXPECT_EQ(executedCommands[2], "/gamemode survival");
    EXPECT_EQ(permissionLevels[0], 2);
    EXPECT_EQ(permissionLevels[1], 2);
    EXPECT_EQ(permissionLevels[2], 4);
}

TEST_F(ServerWorldCommandExecuteTest, CommandBlockMinecartPermissionLevel)
{
    // 测试命令方块矿车使用的权限级别（应为 2）
    i32 capturedPermissionLevel = -1;

    world->setOnExecuteCommand([&](const std::string&, const Vector3d&, i32 permissionLevel, const Vector2f&) -> i32 {
        capturedPermissionLevel = permissionLevel;
        return 1;
    });

    // 模拟命令方块矿车执行命令
    Vector3d minecartPosition(100.0, 64.0, 100.0);
    i32 result = world->executeCommand("/say test", minecartPosition, 2, mc::math::Vector2f(0.0f, 0.0f));

    EXPECT_EQ(result, 1);
    EXPECT_EQ(capturedPermissionLevel, 2); // 命令方块矿车使用权限级别 2
}

TEST_F(ServerWorldCommandExecuteTest, CommandPositionPassedCorrectly)
{
    // 测试命令执行位置正确传递
    Vector3d expectedPosition(123.5, 64.0, -456.7);
    Vector3d capturedPosition(0, 0, 0);

    world->setOnExecuteCommand([&](const std::string&, const Vector3d& position, i32, const Vector2f&) -> i32 {
        capturedPosition = position;
        return 1;
    });

    world->executeCommand("/test", expectedPosition, 2, mc::math::Vector2f(0.0f, 0.0f));

    EXPECT_DOUBLE_EQ(capturedPosition.x, expectedPosition.x);
    EXPECT_DOUBLE_EQ(capturedPosition.y, expectedPosition.y);
    EXPECT_DOUBLE_EQ(capturedPosition.z, expectedPosition.z);
}
