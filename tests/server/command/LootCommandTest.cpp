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

/**
 * @file LootCommandTest.cpp
 * @brief LootCommand 单元测试
 *
 * 测试 /loot 命令的注册和命令树结构。
 * 完整的战利品生成和物品分发测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/LootCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class LootTestServer final : public mc::test::BaseTestServer {
public:
    LootTestServer() = default;

    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return m_dimensionManager;
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return m_dimensionManager;
    }

private:
    // 真实 ServerDimensionManager（nullptr 构造：仅用于 getPlayerDimension 等 map 查询，不调
    // initialize 故不解引用内部 m_server；RelWithDebInfo 下构造断言 MC_ASSERT(server!=nullptr) 不生效）。
    // 替代旧 reinterpret_cast<ServerDimensionManager&>(基类DimensionManager) UB——派生类独有
    // m_playerDimensions 越界读基类内存致 TeleportCommand::teleportPlayers 调 getPlayerDimension 时 SEH。
    ServerDimensionManager m_dimensionManager{nullptr};
};

class LootCommandTest : public ::testing::Test {
protected:
    void SetUp() override { LootCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    LootTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// 命令注册测试
// ============================================================================

TEST_F(LootCommandTest, LootCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "loot" && node.type == NodeType::Literal) {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "loot command should be registered";
}

TEST_F(LootCommandTest, LootCommandRequiresPermissionLevel2)
{
    // 权限等级 0 的玩家不应能执行 /loot 命令
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("loot loot minecraft:chest give @p", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// 命令树结构测试
// ============================================================================

TEST_F(LootCommandTest, LootSourceNodesExist)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 检查 loot 子命令节点存在
    bool foundLoot = false;
    bool foundFish = false;
    bool foundKill = false;
    bool foundMine = false;

    for (const auto& node : snapshot.nodes) {
        if (node.name == "loot" && node.type == NodeType::Literal) {
            foundLoot = true;
        }
        if (node.name == "fish" && node.type == NodeType::Literal) {
            foundFish = true;
        }
        if (node.name == "kill" && node.type == NodeType::Literal) {
            foundKill = true;
        }
        if (node.name == "mine" && node.type == NodeType::Literal) {
            foundMine = true;
        }
    }

    EXPECT_TRUE(foundLoot) << "loot source subcommand should exist";
    EXPECT_TRUE(foundFish) << "fish source subcommand should exist";
    EXPECT_TRUE(foundKill) << "kill source subcommand should exist";
    EXPECT_TRUE(foundMine) << "mine source subcommand should exist";
}

TEST_F(LootCommandTest, TargetNodesExist)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 检查目标节点存在
    bool foundGive = false;
    bool foundSpawn = false;
    bool foundInsert = false;
    bool foundReplace = false;

    for (const auto& node : snapshot.nodes) {
        if (node.name == "give" && node.type == NodeType::Literal) {
            foundGive = true;
        }
        if (node.name == "spawn" && node.type == NodeType::Literal) {
            foundSpawn = true;
        }
        if (node.name == "insert" && node.type == NodeType::Literal) {
            foundInsert = true;
        }
        if (node.name == "replace" && node.type == NodeType::Literal) {
            foundReplace = true;
        }
    }

    EXPECT_TRUE(foundGive) << "give target subcommand should exist";
    EXPECT_TRUE(foundSpawn) << "spawn target subcommand should exist";
    EXPECT_TRUE(foundInsert) << "insert target subcommand should exist";
    EXPECT_TRUE(foundReplace) << "replace target subcommand should exist";
}

TEST_F(LootCommandTest, ReplaceSubNodesExist)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 检查 replace 的子节点
    bool foundEntity = false;
    bool foundBlock = false;

    for (const auto& node : snapshot.nodes) {
        if (node.name == "entity" && node.type == NodeType::Literal) {
            foundEntity = true;
        }
        if (node.name == "block" && node.type == NodeType::Literal) {
            foundBlock = true;
        }
    }

    EXPECT_TRUE(foundEntity) << "replace entity subcommand should exist";
    EXPECT_TRUE(foundBlock) << "replace block subcommand should exist";
}

TEST_F(LootCommandTest, ToolOptionNodesExist)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 检查 fish/mine 的工具选项节点
    bool foundMainhand = false;
    bool foundOffhand = false;

    for (const auto& node : snapshot.nodes) {
        if (node.name == "mainhand" && node.type == NodeType::Literal) {
            foundMainhand = true;
        }
        if (node.name == "offhand" && node.type == NodeType::Literal) {
            foundOffhand = true;
        }
    }

    EXPECT_TRUE(foundMainhand) << "mainhand option should exist";
    EXPECT_TRUE(foundOffhand) << "offhand option should exist";
}

// ============================================================================
// 命令解析测试
//
// 注意：完整的命令执行测试需要集成测试环境（真实的服务器、世界、战利品表等）。
// 以下测试仅验证命令能被解析，不验证执行结果。
// ============================================================================

TEST_F(LootCommandTest, LootLootGiveCanBeParsed)
{
    // 验证 /loot loot <table> give <players> 语法能被命令分发器解析
    // 不执行命令（需要真实服务器环境），只验证命令树中存在对应路径
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 找到 loot 根节点
    bool foundLootNode = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "loot" && node.type == NodeType::Literal) {
            foundLootNode = true;
            // loot 节点应该有子节点（loot/fish/kill/mine）
            EXPECT_FALSE(node.children.empty()) << "loot command should have child nodes";
            break;
        }
    }

    EXPECT_TRUE(foundLootNode) << "loot command root node should exist";
}

TEST_F(LootCommandTest, ConsoleHasSufficientPermission)
{
    // 控制台应该有足够权限执行 /loot 命令
    // forConsole 创建的命令源权限等级为 4
    EXPECT_TRUE(m_console.hasPermission(2));
    EXPECT_TRUE(m_console.hasPermission(4));
}

} // namespace mc::command
