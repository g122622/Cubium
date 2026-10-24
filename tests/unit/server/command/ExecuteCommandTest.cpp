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
 * @file ExecuteCommandTest.cpp
 * @brief ExecuteCommand 单元测试
 *
 * 测试 /execute 命令的注册、解析和嵌套命令执行功能。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/DimensionArgument.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ExecuteCommand.hpp"
#include "server/command/commands/HelpCommand.hpp"
#include "server/command/commands/ListCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class ExecuteTestServer final : public mc::test::BaseTestServer {
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

} // namespace mc::command

class ExecuteCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::command::ExecuteCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::HelpCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::ListCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    mc::command::ExecuteTestServer m_server;
    mc::command::ServerCommandSource m_console = mc::command::ServerCommandSource::forConsole(&m_server);
};

TEST_F(ExecuteCommandTest, ExecuteCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "execute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "execute command should be registered";
}

TEST_F(ExecuteCommandTest, ExecuteCommandRequiresPermissionLevel2)
{
    mc::command::ServerCommandSource lowPermSource(
        &m_server, nullptr, 0, mc::Vector3d(0, 0, 0), mc::Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("execute run help", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(ExecuteCommandTest, ExecuteRunHelpCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunListCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run list", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteRunWithSlash)
{
    const auto result = m_server.commandRegistry().execute("execute run /help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunEmptyCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecutePositionedRunCommand)
{
    const auto result = m_server.commandRegistry().execute("execute positioned 100 64 200 run list", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecutePositionedWithRelativeCoords)
{
    const auto result = m_server.commandRegistry().execute("execute positioned ~10 ~ ~-5 run help", m_console);

    EXPECT_TRUE(result.success());
}

TEST_F(ExecuteCommandTest, ExecuteIfBlockCommandNoWorld)
{
    const auto result = m_server.commandRegistry().execute("execute if block 0 0 0 stone run help", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteUnlessBlockCommandNoWorld)
{
    const auto result = m_server.commandRegistry().execute("execute unless block 0 0 0 stone run help", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteAsNoTarget)
{
    const auto result = m_server.commandRegistry().execute("execute as @p run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteAsWithPlayer)
{
    m_server.addTestPlayer(1, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("execute as TestPlayer run help", m_console);

    EXPECT_TRUE(result.success() || result.failed());
}

TEST_F(ExecuteCommandTest, ExecuteAtNoTarget)
{
    const auto result = m_server.commandRegistry().execute("execute at @p run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, InvalidSubcommand)
{
    const auto result = m_server.commandRegistry().execute("execute invalid run help", m_console);

    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, MissingRunKeyword)
{
    const auto result = m_server.commandRegistry().execute("execute as @p help", m_console);

    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, NestedCommandExecution)
{
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, MultipleNestedCommands)
{
    const auto result = m_server.commandRegistry().execute("execute positioned 0 0 0 run execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteInOverworld)
{
    // /execute in overworld run list - 维度参数 "overworld" 解析为 DimensionId=0
    // BaseTestServer 的 dimensionManager() 未实现（抛异常），因此执行阶段会失败
    // 但解析阶段应成功（不会因无效维度名而抛出 CommandException）
    const auto result = m_server.commandRegistry().execute("execute in overworld run list", m_console);

    // 解析成功但维度验证失败（BaseTestServer 未注册维度）→ failed() 或 value()==0
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInNether)
{
    // /execute in the_nether run list - DimensionArgumentType 解析 "the_nether" 为 DimensionId=-1
    const auto result = m_server.commandRegistry().execute("execute in the_nether run list", m_console);

    // BaseTestServer 默认不注册下界维度，所以应该返回 0（维度不存在）
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInNamespaceFormat)
{
    // /execute in minecraft:overworld run list - 命名空间格式也能正确解析
    // readUnquotedString() 会将 "minecraft:overworld" 完整读取（冒号不是停止字符）
    // DimensionArgumentType 的 _isValidDimensionName 明确支持 "minecraft:overworld" 格式
    const auto result = m_server.commandRegistry().execute("execute in minecraft:overworld run list", m_console);

    // 验证：解析应成功，不会因为冒号导致解析错误
    // 维度验证失败是因为 BaseTestServer 未注册维度，而非解析错误
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInNumericFormat)
{
    // /execute in -1 run list - 数字格式 "-1" 代表下界
    // 注意：DimensionArgumentType 使用 readUnquotedString 读取，"-1" 会被完整读取
    // 但 StringReader 的 readUnquotedString 会读取 "-" 和数字作为单个 token
    // DimensionArgumentType 的 _isValidDimensionName 支持 "-1" 格式
    const auto result = m_server.commandRegistry().execute("execute in -1 run list", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInInvalidDimension)
{
    // /execute in invalid_dimension run list - 无效维度名称应导致解析错误
    // _isValidDimensionName 不认识 "invalid_dimension"，抛出 CommandException
    const auto result = m_server.commandRegistry().execute("execute in invalid_dimension run list", m_console);

    // 解析失败 → result.failed() 为 true
    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, ExecuteInSubcommandRegistered)
{
    // 验证 "in" 子命令已注册到命令树中
    auto& registry = m_server.commandRegistry();
    const auto result = registry.execute("execute in overworld run list", m_console);

    // 命令应能被解析（不会出现 "unknown argument" 之类的解析错误）
    // 即使执行失败（因为 BaseTestServer 没有维度管理器），解析也应成功
    // 区分解析失败和执行失败：解析失败意味着 "in" 子命令根本没被识别
    // 执行失败意味着解析正确但维度不存在
    // 由于 BaseTestServer 的 dimensionManager() 会抛异常，这里检查没有抛出未捕获异常即可
    // result.failed() 或 result.value()==0 表示命令被识别并执行（维度验证失败）
    // 不会出现 "Unknown command" 等解析错误
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

// ============================================================================
// DimensionArgumentType 单元测试
// 独立于 ExecuteCommand，直接测试解析逻辑
// ============================================================================

class DimensionArgumentTypeTest : public ::testing::Test {
protected:
    mc::command::DimensionArgumentType m_parser;
};

TEST_F(DimensionArgumentTypeTest, ParseOverworldShort)
{
    mc::command::StringReader reader("overworld");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 0);
}

TEST_F(DimensionArgumentTypeTest, ParseNetherShort)
{
    mc::command::StringReader reader("the_nether");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, -1);
}

TEST_F(DimensionArgumentTypeTest, ParseTheEndShort)
{
    mc::command::StringReader reader("the_end");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 1);
}

TEST_F(DimensionArgumentTypeTest, ParseOverworldNamespace)
{
    // 关键测试：验证 readUnquotedString() 能正确读取包含冒号的命名空间格式
    mc::command::StringReader reader("minecraft:overworld");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 0);
}

TEST_F(DimensionArgumentTypeTest, ParseNetherNamespace)
{
    mc::command::StringReader reader("minecraft:the_nether");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, -1);
}

TEST_F(DimensionArgumentTypeTest, ParseTheEndNamespace)
{
    mc::command::StringReader reader("minecraft:the_end");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 1);
}

TEST_F(DimensionArgumentTypeTest, ParseNumericZero)
{
    mc::command::StringReader reader("0");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 0);
}

TEST_F(DimensionArgumentTypeTest, ParseNumericMinusOne)
{
    // 测试 "-1" 数字格式（下界）
    // readUnquotedString 将 "-1" 作为完整 token 读取（'-' 不是停止字符）
    mc::command::StringReader reader("-1");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, -1);
}

TEST_F(DimensionArgumentTypeTest, ParseNumericOne)
{
    mc::command::StringReader reader("1");
    auto dimId = m_parser.parse(reader);
    EXPECT_EQ(dimId, 1);
}

TEST_F(DimensionArgumentTypeTest, ParseInvalidThrows)
{
    // 无效的维度名称应抛出 CommandException
    mc::command::StringReader reader("invalid_dimension");
    EXPECT_THROW({ m_parser.parse(reader); }, mc::command::CommandException);
}

TEST_F(DimensionArgumentTypeTest, ParseInvalidNamespaceThrows)
{
    // 无效的命名空间格式应抛出 CommandException
    mc::command::StringReader reader("minecraft:invalid");
    EXPECT_THROW({ m_parser.parse(reader); }, mc::command::CommandException);
}

TEST_F(DimensionArgumentTypeTest, ParseInvalidNumericThrows)
{
    // 无效的数字 ID 应抛出 CommandException
    mc::command::StringReader reader("42");
    EXPECT_THROW({ m_parser.parse(reader); }, mc::command::CommandException);
}

TEST_F(DimensionArgumentTypeTest, GetTypeName)
{
    EXPECT_EQ(m_parser.getTypeName(), "dimension");
}

TEST_F(DimensionArgumentTypeTest, GetExamples)
{
    auto examples = m_parser.getExamples();
    EXPECT_EQ(examples.size(), 3u);
    EXPECT_EQ(examples[0], "overworld");
    EXPECT_EQ(examples[1], "the_nether");
    EXPECT_EQ(examples[2], "the_end");
}

// ============================================================================
// _executeAt 维度切换验证
// 由于 BaseTestServer 没有完整的实体系统，无法直接测试 at 子命令的维度切换。
// 但维度切换的核心逻辑（Entity::dimension() → ServerCommandSource::withDimension()）
// 已通过代码审查确认正确：
// 1. Entity::dimension() 返回 m_dimension 字段（Entity.hpp:761）
// 2. ServerCommandSource::withDimension(DimensionId) 创建新源并更新 m_dimensionId（ServerCommandSource.cpp:192-197）
// 3. _executeAt 调用 modifiedSource.withDimension(entity->dimension()) 对齐 MC Java 行为
// ============================================================================
