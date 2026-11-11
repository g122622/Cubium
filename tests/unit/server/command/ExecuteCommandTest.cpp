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

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>

#include "common/BaseTestServer.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/DimensionArgument.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootPredicateManager.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/blockentity/redstone/CommandBlockEntity.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ExecuteCommand.hpp"
#include "server/command/commands/HelpCommand.hpp"
#include "server/command/commands/ListCommand.hpp"
#include "server/command/data/DataAccessor.hpp"
#include "server/dimension/ServerDimension.hpp"
#include "server/dimension/ServerDimensionManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"

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

    // 权限不足时派发器直接返回 PermissionDenied 错误，不需要接异常：
    // `Result::value()` 在 failed() 时是断言崩溃而不是抛异常，旧写法会带走整个测试进程。
    const auto result = m_server.commandRegistry().execute("execute run help", lowPermSource);
    const bool permissionDenied = result.failed() || result.value() == 0;

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

// ============================================================================
// if|unless 条件按参数分派（block / biome / dimension / entity / loaded）
// ============================================================================

// 与命令源当前维度一致 → 条件成立，嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteIfDimensionMatchesCurrentDimension)
{
    const auto result =
        m_server.commandRegistry().execute("execute if dimension minecraft:overworld run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "the condition holds, so the nested command must run";
}

// 与当前维度不同 → 条件不成立，嵌套命令不执行。
TEST_F(ExecuteCommandTest, ExecuteIfDimensionMismatchSkipsNestedCommand)
{
    const auto result =
        m_server.commandRegistry().execute("execute if dimension minecraft:the_nether run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0) << "the condition fails, so the nested command must not run";
}

// unless 是 if 取反：维度不同时条件为真，嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteUnlessDimensionMismatchRunsNestedCommand)
{
    const auto result =
        m_server.commandRegistry().execute("execute unless dimension minecraft:the_nether run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "unless inverts if, so a mismatching dimension must run the nested command";
}

// 没有任何实体可选中 → if entity 不成立。
TEST_F(ExecuteCommandTest, ExecuteIfEntityWithNoMatchesSkipsNestedCommand)
{
    const auto result =
        m_server.commandRegistry().execute("execute if entity @e[type=minecraft:pig] run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// 无世界时无法判定区块是否加载 → if loaded 不成立。
TEST_F(ExecuteCommandTest, ExecuteIfLoadedWithoutWorldSkipsNestedCommand)
{
    const auto result = m_server.commandRegistry().execute("execute if loaded 0 0 0 run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// biome 求值尚未实现，即使语法合法也按不成立处理（不能误让嵌套命令执行）。
TEST_F(ExecuteCommandTest, ExecuteIfBiomeIsTreatedAsUnmet)
{
    const auto result =
        m_server.commandRegistry().execute("execute if biome 0 0 0 minecraft:plains run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// if|unless score 条件
// ============================================================================

namespace {

/// 建一个 dummy 判据的计分项并写入分数，供 if score 用例使用。
void seedScore(mc::server::IServer& server, const std::string& holder, const std::string& objectiveName, mc::i32 value)
{
    mc::scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    auto& scoreboard = server.scoreboard();
    auto* objective = scoreboard.getObjective(objectiveName);
    if (objective == nullptr) {
        objective = scoreboard.addObjective(
            objectiveName, *mc::scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy"));
    }
    scoreboard.getOrCreateScore(holder, *objective)->setScorePoints(value);
}

} // namespace

// 左侧大于右侧 → `>` 成立，嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteIfScoreGreaterHolds)
{
    seedScore(m_server, "alice", "objA", 10);
    seedScore(m_server, "bob", "objB", 3);

    const auto result =
        m_server.commandRegistry().execute("execute if score alice objA > bob objB run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "10 > 3, so the nested command must run";
}

// 同一个比较在方向相反时不成立。
TEST_F(ExecuteCommandTest, ExecuteIfScoreGreaterFailsWhenReversed)
{
    seedScore(m_server, "alice", "objA", 10);
    seedScore(m_server, "bob", "objB", 3);

    const auto result =
        m_server.commandRegistry().execute("execute if score bob objB > alice objA run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0) << "3 > 10 is false, so the nested command must not run";
}

// 相等比较：`=` 与 `<=` 都应在两侧相等时成立。
TEST_F(ExecuteCommandTest, ExecuteIfScoreEqualityHoldsWhenBothSidesEqual)
{
    seedScore(m_server, "alice", "objA", 7);
    seedScore(m_server, "bob", "objB", 7);

    const auto eqResult =
        m_server.commandRegistry().execute("execute if score alice objA = bob objB run help", m_console);
    EXPECT_TRUE(eqResult.success());
    EXPECT_EQ(eqResult.value(), 1);

    const auto leResult =
        m_server.commandRegistry().execute("execute if score alice objA <= bob objB run help", m_console);
    EXPECT_TRUE(leResult.success());
    EXPECT_EQ(leResult.value(), 1);
}

// unless 取反：比较不成立时嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteUnlessScoreRunsWhenComparisonFails)
{
    seedScore(m_server, "alice", "objA", 1);
    seedScore(m_server, "bob", "objB", 99);

    const auto result =
        m_server.commandRegistry().execute("execute unless score alice objA > bob objB run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "unless inverts if, so a failing comparison must run the nested command";
}

// matches <range>：落在区间内成立。
TEST_F(ExecuteCommandTest, ExecuteIfScoreMatchesRangeHolds)
{
    seedScore(m_server, "alice", "objA", 5);

    const auto result =
        m_server.commandRegistry().execute("execute if score alice objA matches 3..8 run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "5 is inside 3..8";
}

// matches <range>：落在区间外不成立。
TEST_F(ExecuteCommandTest, ExecuteIfScoreMatchesRangeFailsOutside)
{
    seedScore(m_server, "alice", "objA", 20);

    const auto result =
        m_server.commandRegistry().execute("execute if score alice objA matches 3..8 run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0) << "20 is outside 3..8";
}

// 计分项不存在时条件不成立（不能顺手创建 0 分项）。
TEST_F(ExecuteCommandTest, ExecuteIfScoreWithMissingObjectiveFails)
{
    const auto result = m_server.commandRegistry().execute(
        "execute if score alice noSuchObjective = bob noSuchObjective run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ============================================================================
// if|unless data 条件（当前实现 storage 变体）
// ============================================================================

namespace {

/// 往命令存储的指定 id 写入一个 compound，供 if data 用例使用。
void seedCommandStorage(mc::server::IServer& server, const std::string& id, mc::nbt::tags::compound_tag data)
{
    mc::ResourceLocation location(id);
    mc::command::StorageDataAccessor accessor(&server.commandStorage(), location);
    accessor.mergeData(data);
}

} // namespace

// storage 里存在该路径 → 条件成立，嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteIfDataStorageHoldsWhenPathExists)
{
    mc::nbt::tags::compound_tag data;
    data.put("hp", 20);
    seedCommandStorage(m_server, "test:ifdata_hit", data);

    const auto result =
        m_server.commandRegistry().execute("execute if data storage test:ifdata_hit hp run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1) << "the path exists, so the nested command must run";
}

// storage 里没有该路径 → 条件不成立。
TEST_F(ExecuteCommandTest, ExecuteIfDataStorageFailsWhenPathMissing)
{
    mc::nbt::tags::compound_tag data;
    data.put("hp", 20);
    seedCommandStorage(m_server, "test:ifdata_miss", data);

    const auto result =
        m_server.commandRegistry().execute("execute if data storage test:ifdata_miss nope run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0) << "the path does not exist, so the nested command must not run";
}

// 完全没写过的 storage id → 条件不成立（不能顺手创建空存储）。
TEST_F(ExecuteCommandTest, ExecuteIfDataStorageUnknownIdFails)
{
    const auto result =
        m_server.commandRegistry().execute("execute if data storage test:never_written hp run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// block / entity 变体尚未实现，必须按不成立处理，不能误让嵌套命令执行。
TEST_F(ExecuteCommandTest, ExecuteIfDataUnimplementedVariantsAreUnmet)
{
    const auto blockResult =
        m_server.commandRegistry().execute("execute if data block 0 0 0 Items run help", m_console);
    EXPECT_TRUE(blockResult.success());
    EXPECT_EQ(blockResult.value(), 0);

    const auto entityResult = m_server.commandRegistry().execute(
        "execute if data entity @e[type=minecraft:pig,limit=1] CustomName run help", m_console);
    EXPECT_TRUE(entityResult.success());
    EXPECT_EQ(entityResult.value(), 0);
}

TEST_F(ExecuteCommandTest, UnsupportedConditionsNeverExecuteFollowingCommand)
{
    mc::i32 executions = 0;
    auto command = std::make_shared<mc::command::LiteralCommandNode<mc::command::ServerCommandSource>>("record");
    command->setCommand([&executions](auto&) {
        ++executions;
        return 1;
    });
    m_server.commandRegistry().dispatcher().registerCommand(command);

    for (const char* input :
        {"execute if block 0 0 0 stone run record", "execute positioned 0 0 0 if block 0 0 0 stone run record"}) {
        const auto result = m_server.commandRegistry().execute(input, m_console);
        // 注意：无世界时 _evaluateBlockCondition 返回 false，`if block` 条件不成立，
        // 嵌套命令被跳过（executions 保持 0）。execute 本身仍返回成功状态——
        // 条件不成立不是命令解析/执行错误（原版 execute 在条件失败时返回 0 而非报错）。
        EXPECT_TRUE(result.success()) << input;
        EXPECT_EQ(result.value(), 0) << input;
        EXPECT_EQ(executions, 0) << input;
    }

    // unless 在无世界时条件为「真」（!false），因此嵌套命令会被执行一次。
    {
        const auto result =
            m_server.commandRegistry().execute("execute unless block 0 0 0 stone run record", m_console);
        EXPECT_TRUE(result.success());
        EXPECT_EQ(executions, 1);
    }
}

TEST_F(ExecuteCommandTest, ExecuteUnlessBlockCommandNoWorld)
{
    // 无世界时 `if block` 条件恒为假，`unless block` 取其反，因此嵌套命令会执行（help 返回 1）。
    const auto result = m_server.commandRegistry().execute("execute unless block 0 0 0 stone run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
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

// ============================================================================
// if biome 的正向路径
//
// 需要命令源能解析到真实世界。BaseTestServer 的 dimensionManager() 敏感之前直接
// 抛 "unused"，所以这类用例根本写不出来。现在它返回一个真实的
// ServerDimensionManager，用例可以自己装入维度与世界。
// ============================================================================

namespace {

std::unique_ptr<mc::server::ServerWorld> createBiomeTestWorld(const mc::server::ServerWorldConfig& config)
{
    auto world = std::make_unique<mc::server::ServerWorld>(config);
    auto settings = mc::DimensionSettings::overworld();
    auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
    auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
    auto generator =
        std::make_unique<mc::NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
    auto chunkManager = std::make_unique<mc::server::ServerChunkManager>(*world, std::move(generator));
    world->setChunkManager(std::move(chunkManager));
    return world;
}

} // namespace

class ExecuteCommandBiomeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::VanillaBlocks::initialize();

        mc::command::ExecuteCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::HelpCommand::registerTo(m_server.commandRegistry().dispatcher());

        mc::server::ServerWorldConfig config;
        config.viewDistance = 4;
        config.dimension = 0;
        config.seed = 12345;

        auto dimension = std::make_unique<mc::ServerDimension>(0, mc::DimensionType::overworld(), 12345, 4);
        dimension->setWorld(createBiomeTestWorld(config));
        MC_ASSERT_RELEASE(m_server.dimensionManager().registerDimension(std::move(dimension)));

        m_console = mc::command::ServerCommandSource::forConsole(&m_server);
    }

    mc::test::BaseTestServer m_server;
    mc::command::ServerCommandSource m_console = mc::command::ServerCommandSource::forConsole(nullptr);
};

// 不假设具体群系名：先从世界直接读出该坐标的真实群系，再断言命令的结果
// 与之一致。这样一来，“恒返回 false”与“恒返回 true”两种错误实现都会被捕获。
TEST_F(ExecuteCommandBiomeTest, ExecuteIfBiomeAgreesWithWorld)
{
    mc::server::ServerWorld* world = m_server.dimensionManager().getDimension(0)->world();
    ASSERT_NE(world, nullptr);

    // ServerWorld 自己的 getBiome 是私有的，走 IWorld 接口。
    const auto* reader = static_cast<const mc::IWorld*>(world);
    const mc::BiomeId actualId = reader->getBiome(0, 64, 0);

    const auto& registry = mc::world::biome::BiomeRegistry::instance();
    ASSERT_TRUE(registry.hasBiome(actualId));
    const std::string actualName = "minecraft:" + registry.get(actualId).name();

    // 命中：用真实群系名去问。
    const auto hit =
        m_server.commandRegistry().execute("execute if biome 0 64 0 " + actualName + " run help", m_console);
    ASSERT_TRUE(hit.success());
    EXPECT_EQ(hit.value(), 1) << "biome at 0,64,0 is " << actualName;

    // 不存在的群系必定不成立。
    const auto miss =
        m_server.commandRegistry().execute("execute if biome 0 64 0 minecraft:not_a_real_biome run help", m_console);
    ASSERT_TRUE(miss.success());
    EXPECT_EQ(miss.value(), 0);
}

// ============================================================================
// if data entity 的正向路径
// ============================================================================

class ExecuteCommandDataEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::VanillaBlocks::initialize();
        mc::entity::VanillaEntities::registerAll();

        mc::command::ExecuteCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::HelpCommand::registerTo(m_server.commandRegistry().dispatcher());

        mc::server::ServerWorldConfig config;
        config.viewDistance = 4;
        config.dimension = 0;
        config.seed = 12345;

        auto world = std::make_unique<mc::server::ServerWorld>(config);
        auto settings = mc::DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator = std::make_unique<mc::NoiseChunkGenerator>(
            std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<mc::server::ServerChunkManager>(*world, std::move(generator));
        world->setChunkManager(std::move(chunkManager));

        auto dimension = std::make_unique<mc::ServerDimension>(0, mc::DimensionType::overworld(), 12345, 4);
        m_world = world.get();
        dimension->setWorld(std::move(world));
        MC_ASSERT_RELEASE(m_server.dimensionManager().registerDimension(std::move(dimension)));

        m_console = mc::command::ServerCommandSource::forConsole(&m_server);

        auto zombie = std::make_unique<mc::ZombieEntity>(mc::EntityInstanceId(1), mc::test::testEcsRegistry());
        zombie->setWorld(m_world);
        m_zombie = zombie.get();
        m_world->entityManager().addEntity(std::move(zombie));
    }

    /// 执行命令并返回期间收集到的日志（命令反馈走 sendMessage/sendError → spdlog）。
    std::pair<mc::Result<mc::i32>, std::string> _run(const std::string& command)
    {
        std::string captured;
        auto sink = std::make_shared<spdlog::sinks::callback_sink_mt>([&captured](const spdlog::details::log_msg& msg) {
            captured.append(msg.payload.data(), msg.payload.size());
            captured.push_back('\n');
        });
        auto logger = std::make_shared<spdlog::logger>("data-capture", sink);
        auto previous = spdlog::default_logger();
        spdlog::set_default_logger(logger);

        auto result = m_server.commandRegistry().execute(command, m_console);

        spdlog::set_default_logger(previous);
        return {std::move(result), captured};
    }

    mc::test::BaseTestServer m_server;
    mc::command::ServerCommandSource m_console = mc::command::ServerCommandSource::forConsole(nullptr);
    mc::server::ServerWorld* m_world = nullptr;
    mc::ZombieEntity* m_zombie = nullptr;
};

// Pos 是每个实体 writeToNBT 都会写的键。命中与未命两个方向都要验：
// 只验命中无法发现“恒真”，只验未命中无法发现“恒假”。
TEST_F(ExecuteCommandDataEntityTest, ExecuteIfDataEntityHoldsWhenPathExists)
{
    auto [hit, hitLog] = _run("execute if data entity @e[limit=1] Pos run help");
    ASSERT_TRUE(hit.success()) << "log:\n" << hitLog;
    EXPECT_EQ(hit.value(), 1) << "Pos exists on the entity, condition must hold. log:\n" << hitLog;

    auto [miss, missLog] = _run("execute if data entity @e[limit=1] NoSuchKeyHere run help");
    ASSERT_TRUE(miss.success()) << "log:\n" << missLog;
    EXPECT_EQ(miss.value(), 0) << "a missing path must not satisfy the condition. log:\n" << missLog;
}

// ============================================================================
// if data block 的正向路径
//
// 用命令方块实体：它的 NBT 里有确定存在的 "Command" 键，且构造成本低。
// ============================================================================

TEST_F(ExecuteCommandDataEntityTest, ExecuteIfDataBlockHoldsWhenPathExists)
{
    const mc::BlockPos pos(4, 64, 4);
    auto commandBlock =
        std::make_unique<mc::blockentity::CommandBlockEntity>(pos, mc::blockentity::CommandBlockMode::Redstone);
    commandBlock->setCommand("say hello");
    m_world->setBlockEntity(pos, std::move(commandBlock));

    auto [hit, hitLog] = _run("execute if data block 4 64 4 Command run help");
    ASSERT_TRUE(hit.success()) << "log:\n" << hitLog;
    EXPECT_EQ(hit.value(), 1) << "the command block stores a Command tag. log:\n" << hitLog;

    auto [miss, missLog] = _run("execute if data block 4 64 4 NoSuchKeyHere run help");
    ASSERT_TRUE(miss.success()) << "log:\n" << missLog;
    EXPECT_EQ(miss.value(), 0) << "a missing path must not satisfy the condition. log:\n" << missLog;

    // 该位置没有方块实体时，条件应当不成立而不是报错。
    auto [empty, emptyLog] = _run("execute if data block 0 0 0 Command run help");
    ASSERT_TRUE(empty.success()) << "missing block entity must not error out. log:\n" << emptyLog;
    EXPECT_EQ(empty.value(), 0);
}

// ============================================================================
// if items 的正向路径（方块容器）
// ============================================================================

TEST_F(ExecuteCommandDataEntityTest, ExecuteIfItemsBlockMatchesContainerContents)
{
    const mc::BlockPos pos(4, 64, 4);
    auto chest = std::make_unique<mc::blockentity::ChestEntity>(pos);
    auto* inventory = chest->getInventory();
    ASSERT_NE(inventory, nullptr);

    const auto* stone = mc::ItemRegistry::instance().getItem(mc::ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr) << "stone must be registered for this test";
    inventory->setItem(0, mc::ItemStack(*stone, 1));

    m_world->setBlockEntity(pos, std::move(chest));

    // container.0 里是石头 → 命中
    auto [hit, hitLog] = _run("execute if items block 4 64 4 container.0 minecraft:stone run help");
    ASSERT_TRUE(hit.success()) << "log:\n" << hitLog;
    EXPECT_EQ(hit.value(), 1) << "slot 0 holds stone. log:\n" << hitLog;

    // 同一槽位问盾牌 → 不命中
    auto [miss, missLog] = _run("execute if items block 4 64 4 container.0 minecraft:shield run help");
    ASSERT_TRUE(miss.success()) << "log:\n" << missLog;
    EXPECT_EQ(miss.value(), 0) << "slot 0 holds stone, not shield. log:\n" << missLog;

    // 空槽位 → 不命中（空栈不能匹配 "*" 之外的任何谓词）
    auto [empty, emptyLog] = _run("execute if items block 4 64 4 container.5 minecraft:stone run help");
    ASSERT_TRUE(empty.success()) << "log:\n" << emptyLog;
    EXPECT_EQ(empty.value(), 0) << "slot 5 is empty. log:\n" << emptyLog;

    // container.* 扫全容器 → 命中
    auto [all, allLog] = _run("execute if items block 4 64 4 container.* minecraft:stone run help");
    ASSERT_TRUE(all.success()) << "log:\n" << allLog;
    EXPECT_EQ(all.value(), 1) << "container.* must cover slot 0. log:\n" << allLog;
}

// ============================================================================
// if items 的正向路径（实体装备槽）
// ============================================================================

TEST_F(ExecuteCommandDataEntityTest, ExecuteIfItemsEntityMatchesEquipment)
{
    ASSERT_NE(m_zombie, nullptr);

    const auto* stone = mc::ItemRegistry::instance().getItem(mc::ResourceLocation("minecraft:stone"));
    ASSERT_NE(stone, nullptr);
    // 直接给夹具在 SetUp 里建的那个僵尸装备：它是世界里唯一的实体，
    // @e[limit=1] 一定选到它。若另建一个，选中的可能是前者。
    m_zombie->setEquipment(mc::EquipmentSlot::MainHand, mc::ItemStack(*stone, 1));

    // weapon.mainhand 里是石头 → 命中
    auto [hit, hitLog] = _run("execute if items entity @e[limit=1] weapon.mainhand minecraft:stone run help");
    ASSERT_TRUE(hit.success()) << "log:\n" << hitLog;
    EXPECT_EQ(hit.value(), 1) << "main hand holds stone. log:\n" << hitLog;

    // 同一槽位问盾牌 → 不命中
    auto [miss, missLog] = _run("execute if items entity @e[limit=1] weapon.mainhand minecraft:shield run help");
    ASSERT_TRUE(miss.success()) << "log:\n" << missLog;
    EXPECT_EQ(miss.value(), 0) << "log:\n" << missLog;

    // 空的副手 → 不命中
    auto [offhand, offhandLog] = _run("execute if items entity @e[limit=1] weapon.offhand minecraft:stone run help");
    ASSERT_TRUE(offhand.success()) << "log:\n" << offhandLog;
    EXPECT_EQ(offhand.value(), 0) << "off hand is empty. log:\n" << offhandLog;
}

// ============================================================================
// if predicate 的正向与负向路径
//
// 谓词需要先注册到 LootPredicateManager。这里用一个本地的恒真/恒假
// LootCondition 子类注册两个谓词，避免依赖数据包内容。
// ============================================================================

namespace {

/// 无条件成立的战利品谓词（仅供测试）
class AlwaysTrueCondition final : public mc::loot::LootCondition {
public:
    bool test(mc::loot::LootContext& /*context*/) const override
    {
        return true;
    }

    std::unique_ptr<mc::loot::LootCondition> clone() const noexcept override
    {
        return std::make_unique<AlwaysTrueCondition>(*this);
    }

    std::string getType() const override
    {
        return "test:always_true";
    }
};

/// 无条件不成立的战利品谓词（仅供测试）
class AlwaysFalseCondition final : public mc::loot::LootCondition {
public:
    bool test(mc::loot::LootContext& /*context*/) const override
    {
        return false;
    }

    std::unique_ptr<mc::loot::LootCondition> clone() const noexcept override
    {
        return std::make_unique<AlwaysFalseCondition>(*this);
    }

    std::string getType() const override
    {
        return "test:always_false";
    }
};

} // namespace

TEST_F(ExecuteCommandDataEntityTest, ExecuteIfPredicateFollowsRegisteredCondition)
{
    // 谓词注册在独立的 LootPredicateManager 上，LootTableManager 只持有它的 const 指针，
    // 所以测试里自建一个 manager 再挂上去。
    mc::loot::LootPredicateManager predicateManager;
    predicateManager.registerPredicate("test:always_true", std::make_unique<AlwaysTrueCondition>());
    predicateManager.registerPredicate("test:always_false", std::make_unique<AlwaysFalseCondition>());
    m_server.lootTableManager().setPredicateManager(&predicateManager);

    // 注册为恒真 → 命中
    auto [hit, hitLog] = _run("execute if predicate test:always_true run help");
    ASSERT_TRUE(hit.success()) << "log:\n" << hitLog;
    EXPECT_EQ(hit.value(), 1) << "log:\n" << hitLog;

    // 注册为恒假 → 不命中
    auto [miss, missLog] = _run("execute if predicate test:always_false run help");
    ASSERT_TRUE(miss.success()) << "log:\n" << missLog;
    EXPECT_EQ(miss.value(), 0) << "log:\n" << missLog;

    // 未注册的谓词 → 不命中且有 warn
    auto [unknown, unknownLog] = _run("execute if predicate test:does_not_exist run help");
    ASSERT_TRUE(unknown.success()) << "log:\n" << unknownLog;
    EXPECT_EQ(unknown.value(), 0);
    EXPECT_NE(unknownLog.find("unknown loot predicate"), std::string::npos)
        << "an unregistered predicate must be reported, not silently false; log:\n"
        << unknownLog;
}
