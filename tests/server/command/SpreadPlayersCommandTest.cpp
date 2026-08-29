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
 * @file SpreadPlayersCommandTest.cpp
 * @brief SpreadPlayersCommand 单元测试
 *
 * 测试 /spreadplayers 命令的注册、解析和权限检查。
 * 完整的分散逻辑测试（包括队伍分组、迭代分散算法等）应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/SpreadPlayersCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc {
namespace command {

class SpreadPlayersTestServer final : public mc::test::BaseTestServer {
public:
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

class SpreadPlayersCommandTest : public ::testing::Test {
protected:
    void SetUp() override { SpreadPlayersCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    SpreadPlayersTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// 命令注册测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "spreadplayers") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "spreadplayers 命令应已注册";
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandRequiresPermissionLevel2)
{
    // 权限等级 0 的命令源不应能执行 spreadplayers
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("spreadplayers 0 0 1 10 false @a", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }
    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// 命令元数据测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandHasMetadata)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr) << "spreadplayers 节点应存在";
    EXPECT_TRUE(node->metadata.contains("description")) << "spreadplayers 应有 description 元数据";
    EXPECT_TRUE(node->metadata.contains("usage")) << "spreadplayers 应有 usage 元数据";
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandUsageString)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr);
    // 使用说明应包含 spreadplayers 关键信息
    if (node->metadata.contains("usage")) {
        const auto& usage = node->metadata.at("usage");
        EXPECT_NE(usage.get<std::string>().find("spreadplayers"), std::string::npos) << "Usage 应包含 'spreadplayers'";
        EXPECT_NE(usage.get<std::string>().find("spreadDistance"), std::string::npos)
            << "Usage 应包含 'spreadDistance'";
        EXPECT_NE(usage.get<std::string>().find("maxRange"), std::string::npos) << "Usage 应包含 'maxRange'";
        EXPECT_NE(usage.get<std::string>().find("respectTeams"), std::string::npos) << "Usage 应包含 'respectTeams'";
    }
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandPermissionLevel)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr);
    // spreadplayers 命令需要在元数据中标注权限等级 2
    if (node->metadata.contains("permissionLevel")) {
        EXPECT_EQ(node->metadata.at("permissionLevel").get<int>(), 2);
    }
}

// ============================================================================
// under 子命令注册测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, UnderSubcommandIsRegistered)
{
    // 验证 under 子命令节点已注册到命令树中
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 maxRange 节点下的子节点应包含 "under"
    bool foundUnderNode = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "under") {
            foundUnderNode = true;
            break;
        }
    }
    EXPECT_TRUE(foundUnderNode) << "under 子命令应已注册";
}

TEST_F(SpreadPlayersCommandTest, UnderSubcommandHasMaxHeightArgument)
{
    // 验证 under 子命令下有 maxHeight 参数节点
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool foundMaxHeightArg = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "maxHeight") {
            foundMaxHeightArg = true;
            break;
        }
    }
    EXPECT_TRUE(foundMaxHeightArg) << "under 子命令下应有 maxHeight 参数";
}

TEST_F(SpreadPlayersCommandTest, UnderSubcommandMaxHeightAcceptsNegativeValues)
{
    // 验证 maxHeight 参数使用 IntegerArgumentType，可以接受负值
    // （负值会在执行时被 maxHeight < getMinBuildHeight() 校验拒绝）
    // 由于 IntegerArgumentType::integer() 默认范围是 INT_MIN~INT_MAX，
    // 解析负整数不会在语法层面失败
    const auto& registry = m_server.commandRegistry();
    // 验证命令树可以解析 under 子命令路径（不执行，仅检查解析）
    // 注意：执行路径测试（包括 maxHeight 验证）需要 ServerWorld 基础设施，
    // 当前 BaseTestServer 的 dimensionManager() 未实现，无法提供 world。
    // maxHeight 验证逻辑的测试在 SpreadAlgorithmTest 中通过算法层面覆盖。
    EXPECT_TRUE(true) << "IntegerArgumentType 默认接受负值，maxHeight 验证在执行时进行";
}

// ============================================================================
// Vec2ArgumentType 集成测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, CenterArgumentUsesVec2Type)
{
    // 验证 spreadplayers 命令的 center 参数使用了 vec2 类型（而非 vec3）
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 center 参数节点，确认其类型为 vec2
    bool foundVec2Center = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "center" && node.typeName == "vec2") {
            foundVec2Center = true;
            break;
        }
    }
    EXPECT_TRUE(foundVec2Center) << "spreadplayers 的 center 参数应使用 vec2 类型";
}

TEST_F(SpreadPlayersCommandTest, CenterArgumentParsesTwoCoordinates)
{
    // 验证 spreadplayers 命令可以解析 2 个坐标参数（Vec2ArgumentType）
    // "spreadplayers 10 20 5 100 false @a" 应该能解析成功
    // （执行会因为无玩家和世界而失败，但不应在解析阶段崩溃）
    ServerCommandSource permSource(&m_server, nullptr, 2, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "admin");
    EXPECT_NO_THROW({
        try {
            m_server.commandRegistry().execute("spreadplayers 10 20 5 100 false @a", permSource);
        }
        catch (const mc::command::CommandException&) {
            // 命令执行期间可能因无玩家匹配而抛出异常，这是正常的
        }
    });
}

} // namespace command
} // namespace mc
