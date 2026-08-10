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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file WhitelistBanCommandTest.cpp
 * @brief WhitelistCommand 和 BanCommand UUID 生成集成测试
 *
 * 测试 /whitelist 和 /ban 命令中离线 UUID 生成的正确性，
 * 验证与 Minecraft 原版算法的一致性、UUID 格式规范、
 * 在线玩家回退逻辑以及命令的整体功能。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/BanCommand.hpp"
#include "server/command/commands/WhitelistCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

// ============================================================================
// 测试服务器
// ============================================================================

class WhitelistBanTestServer final : public mc::test::BaseTestServer {
public:
    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    // 注意：DimensionManager 是 ServerDimensionManager 的基类，
    // 我们将 DimensionManager reinterpret_cast 为 ServerDimensionManager，
    // 因为 ServerDimensionManager::getDimension() 仅调用基类 DimensionManager::getDimension()
    // 然后做 static_cast，在我们的测试场景中是安全的。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return reinterpret_cast<ServerDimensionManager&>(m_dimensionManager);
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return reinterpret_cast<const ServerDimensionManager&>(m_dimensionManager);
    }

private:
    DimensionManager m_dimensionManager;
};

// ============================================================================
// 测试夹具
// ============================================================================

class WhitelistBanCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        WhitelistCommand::registerTo(m_server.commandRegistry().dispatcher());
        BanCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    WhitelistBanTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// UUID 生成正确性测试
// ============================================================================

/**
 * @brief 验证 /whitelist add 对离线玩家生成的 UUID 与 MC 原版一致
 *
 * MC 原版算法: UUID.nameUUIDFromBytes(("OfflinePlayer:" + name).getBytes(UTF_8))
 * 对 "Steve" 的期望 UUID: 5627dd98-e6be-3c21-b8a8-e92344183641
 */
TEST_F(WhitelistBanCommandTest, WhitelistAddOfflinePlayerGeneratesCorrectOfflineUuid)
{
    // 添加离线玩家到白名单（通过名称，非在线选择器）
    const auto result = m_server.commandRegistry().execute("whitelist add Steve", m_console);
    ASSERT_TRUE(result.success());

    // 验证白名单中包含 Steve
    auto& whitelist = m_server.whitelistManager();
    EXPECT_TRUE(whitelist.isNameWhitelisted("Steve"));

    // 验证 UUID 是 MC 原版离线 UUID 算法生成的
    auto entry = whitelist.getEntryByName("Steve");
    ASSERT_TRUE(entry.has_value());

    // 使用 util::generateOfflineUuid 独立计算期望的 UUID
    Uuid expectedUuid = util::generateOfflineUuid("Steve");
    std::string expectedUuidStr = util::uuidToStringWithDashes(expectedUuid);
    EXPECT_EQ(entry->uuid, expectedUuidStr);

    // 验证与 MC 原版已知值一致
    EXPECT_EQ(expectedUuidStr, "5627dd98-e6be-3c21-b8a8-e92344183641");
}

/**
 * @brief 验证 /ban 对离线玩家生成的 UUID 与 MC 原版一致
 */
TEST_F(WhitelistBanCommandTest, BanOfflinePlayerGeneratesCorrectOfflineUuid)
{
    // 封禁离线玩家（通过名称，非在线选择器）
    const auto result = m_server.commandRegistry().execute("ban Alex", m_console);
    ASSERT_TRUE(result.success());

    // 验证封禁列表中包含 Alex
    auto& banList = m_server.bannedPlayerList();
    EXPECT_TRUE(banList.isNameBanned("Alex"));

    // 验证 UUID 是 MC 原版离线 UUID 算法生成的
    auto entry = banList.getEntryByName("Alex");
    ASSERT_TRUE(entry.has_value());

    // 使用 util::generateOfflineUuid 独立计算期望的 UUID
    Uuid expectedUuid = util::generateOfflineUuid("Alex");
    std::string expectedUuidStr = util::uuidToStringWithDashes(expectedUuid);
    EXPECT_EQ(entry->uuid, expectedUuidStr);
}

/**
 * @brief 验证离线 UUID 格式符合 UUID v3 规范
 *
 * UUID v3 规范要求:
 * - 第 7 字节高 4 位为版本号 3 (byte[6] & 0xF0 == 0x30)
 * - 第 9 字节高 2 位为 RFC 4122 变体 (byte[8] & 0xC0 == 0x80)
 * - 格式为 xxxxxxxx-xxxx-3xxx-8xxx-xxxxxxxxxxxx
 */
TEST_F(WhitelistBanCommandTest, OfflineUuidFormatConformsToUuidV3)
{
    // 添加离线玩家
    m_server.commandRegistry().execute("whitelist add TestPlayer", m_console);

    auto& whitelist = m_server.whitelistManager();
    auto entry = whitelist.getEntryByName("TestPlayer");
    ASSERT_TRUE(entry.has_value());

    const std::string& uuid = entry->uuid;

    // UUID 字符串长度应为 36 (8-4-4-4-12 格式含连字符)
    EXPECT_EQ(uuid.length(), 36u);

    // 连字符位置
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');

    // 版本号位（第 14 个字符，即 time_hi_and_version 的高 4 位）应为 '3'
    EXPECT_EQ(uuid[14], '3') << "UUID version should be 3 (MD5-based)";

    // 变体位（第 19 个字符，即 clock_seq_hi_and_reserved 的高 2 位）
    // RFC 4122 变体要求高 2 位为 10，即第 19 个字符应为 8/9/a/b
    char variantChar = uuid[19];
    EXPECT_TRUE(variantChar == '8' || variantChar == '9' || variantChar == 'a' || variantChar == 'b')
        << "UUID variant should be RFC 4122 (byte[8] & 0xC0 == 0x80), got: " << variantChar;
}

/**
 * @brief 验证不同玩家名生成不同的 UUID
 */
TEST_F(WhitelistBanCommandTest, DifferentPlayerNamesGenerateDifferentUuids)
{
    m_server.commandRegistry().execute("whitelist add Alice", m_console);
    m_server.commandRegistry().execute("whitelist add Bob", m_console);

    auto& whitelist = m_server.whitelistManager();
    auto aliceEntry = whitelist.getEntryByName("Alice");
    auto bobEntry = whitelist.getEntryByName("Bob");
    ASSERT_TRUE(aliceEntry.has_value());
    ASSERT_TRUE(bobEntry.has_value());

    EXPECT_NE(aliceEntry->uuid, bobEntry->uuid);
}

/**
 * @brief 验证离线 UUID 生成是确定性的
 *
 * 同一玩家名多次添加应生成相同 UUID
 */
TEST_F(WhitelistBanCommandTest, OfflineUuidGenerationIsDeterministic)
{
    // 通过命令添加
    m_server.commandRegistry().execute("whitelist add DeterminismTest", m_console);

    auto& whitelist = m_server.whitelistManager();
    auto entry = whitelist.getEntryByName("DeterminismTest");
    ASSERT_TRUE(entry.has_value());

    // 通过 util 函数独立计算
    Uuid computedUuid = util::generateOfflineUuid("DeterminismTest");
    std::string computedStr = util::uuidToStringWithDashes(computedUuid);

    EXPECT_EQ(entry->uuid, computedStr);
}

/**
 * @brief 验证 UUID 生成区分大小写
 *
 * MC 原版中 "Steve" 和 "steve" 生成不同的 UUID
 */
TEST_F(WhitelistBanCommandTest, OfflineUuidGenerationIsCaseSensitive)
{
    Uuid steveUpper = util::generateOfflineUuid("Steve");
    Uuid steveLower = util::generateOfflineUuid("steve");
    Uuid steveMixed = util::generateOfflineUuid("StEvE");

    EXPECT_NE(util::uuidToString(steveUpper), util::uuidToString(steveLower));
    EXPECT_NE(util::uuidToString(steveUpper), util::uuidToString(steveMixed));
    EXPECT_NE(util::uuidToString(steveLower), util::uuidToString(steveMixed));
}

// ============================================================================
// WhitelistCommand 功能测试
// ============================================================================

/**
 * @brief 验证 /whitelist on 启用白名单
 */
TEST_F(WhitelistBanCommandTest, WhitelistOnEnablesWhitelist)
{
    auto& whitelist = m_server.whitelistManager();
    EXPECT_FALSE(whitelist.isEnabled());

    const auto result = m_server.commandRegistry().execute("whitelist on", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(whitelist.isEnabled());
}

/**
 * @brief 验证 /whitelist off 禁用白名单
 */
TEST_F(WhitelistBanCommandTest, WhitelistOffDisablesWhitelist)
{
    auto& whitelist = m_server.whitelistManager();
    whitelist.setEnabled(true);
    EXPECT_TRUE(whitelist.isEnabled());

    const auto result = m_server.commandRegistry().execute("whitelist off", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(whitelist.isEnabled());
}

/**
 * @brief 验证 /whitelist add 和 remove
 */
TEST_F(WhitelistBanCommandTest, WhitelistAddAndRemove)
{
    auto& whitelist = m_server.whitelistManager();

    // 添加玩家
    const auto addResult = m_server.commandRegistry().execute("whitelist add TestPlayer", m_console);
    EXPECT_TRUE(addResult.success());
    EXPECT_TRUE(whitelist.isNameWhitelisted("TestPlayer"));

    // 重复添加应失败
    const auto addAgainResult = m_server.commandRegistry().execute("whitelist add TestPlayer", m_console);
    EXPECT_EQ(addAgainResult.value(), 0);

    // 移除玩家
    const auto removeResult = m_server.commandRegistry().execute("whitelist remove TestPlayer", m_console);
    EXPECT_TRUE(removeResult.success());
    EXPECT_FALSE(whitelist.isNameWhitelisted("TestPlayer"));
}

/**
 * @brief 验证 /whitelist list 列出玩家
 */
TEST_F(WhitelistBanCommandTest, WhitelistListShowsEntries)
{
    m_server.commandRegistry().execute("whitelist add Player1", m_console);
    m_server.commandRegistry().execute("whitelist add Player2", m_console);

    auto& whitelist = m_server.whitelistManager();
    auto names = whitelist.getAllNames();
    EXPECT_EQ(names.size(), 2u);
}

/**
 * @brief 验证 /whitelist 权限要求（权限等级 3）
 */
TEST_F(WhitelistBanCommandTest, WhitelistRequiresPermissionLevel3)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("whitelist list", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }
    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// BanCommand 功能测试
// ============================================================================

/**
 * @brief 验证 /ban 对离线玩家的封禁功能
 */
TEST_F(WhitelistBanCommandTest, BanOfflinePlayer)
{
    const auto result = m_server.commandRegistry().execute("ban BadPlayer Griefing", m_console);
    EXPECT_TRUE(result.success());

    auto& banList = m_server.bannedPlayerList();
    EXPECT_TRUE(banList.isNameBanned("BadPlayer"));

    auto entry = banList.getEntryByName("BadPlayer");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->name, "BadPlayer");
    EXPECT_EQ(entry->reason, "Griefing");
}

/**
 * @brief 验证 /ban 默认封禁原因
 */
TEST_F(WhitelistBanCommandTest, BanDefaultReason)
{
    const auto result = m_server.commandRegistry().execute("ban RuleBreaker", m_console);
    EXPECT_TRUE(result.success());

    auto& banList = m_server.bannedPlayerList();
    auto entry = banList.getEntryByName("RuleBreaker");
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->reason, "Banned by an operator");
}

/**
 * @brief 验证 /ban 封禁 UUID 格式正确（MC 原版离线 UUID）
 */
TEST_F(WhitelistBanCommandTest, BanOfflinePlayerUuidMatchesOfflineAlgorithm)
{
    m_server.commandRegistry().execute("ban OfflineBannedPlayer", m_console);

    auto& banList = m_server.bannedPlayerList();
    auto entry = banList.getEntryByName("OfflineBannedPlayer");
    ASSERT_TRUE(entry.has_value());

    // UUID 应与独立计算的离线 UUID 一致
    Uuid expectedUuid = util::generateOfflineUuid("OfflineBannedPlayer");
    std::string expectedStr = util::uuidToStringWithDashes(expectedUuid);
    EXPECT_EQ(entry->uuid, expectedStr);
}

/**
 * @brief 验证重复封禁同一玩家应失败
 */
TEST_F(WhitelistBanCommandTest, BanSamePlayerTwiceFails)
{
    m_server.commandRegistry().execute("ban DuplicatePlayer", m_console);

    const auto result = m_server.commandRegistry().execute("ban DuplicatePlayer", m_console);
    EXPECT_EQ(result.value(), 0);
}

/**
 * @brief 验证 /ban 权限要求（权限等级 3）
 */
TEST_F(WhitelistBanCommandTest, BanRequiresPermissionLevel3)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("ban SomePlayer", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }
    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// 在线玩家 UUID 回退逻辑测试
// ============================================================================

/**
 * @brief 验证在线玩家使用其真实 UUID（转为带连字符格式）
 *
 * ServerPlayerData::uuid 存储为 32 字符无连字符格式，
 * 白名单 JSON 使用带连字符的标准格式（xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）。
 * 命令应正确转换格式。
 */
TEST_F(WhitelistBanCommandTest, OnlinePlayerUsesRealUuid)
{
    // 注册在线玩家
    auto* playerData = m_server.addTestPlayer(1, "OnlineSteve");
    ASSERT_NE(playerData, nullptr);

    // 在线玩家的 UUID 应由 PlayerManager 管理（32 字符无连字符格式）
    std::string rawUuid = playerData->uuid;
    EXPECT_FALSE(rawUuid.empty());
    EXPECT_EQ(rawUuid.length(), 32u) << "PlayerData UUID should be 32 chars without dashes";

    // 通过命令添加到白名单
    const auto result = m_server.commandRegistry().execute("whitelist add OnlineSteve", m_console);
    EXPECT_TRUE(result.success());

    auto& whitelist = m_server.whitelistManager();
    auto entry = whitelist.getEntryByName("OnlineSteve");
    ASSERT_TRUE(entry.has_value());

    // 白名单中存储的 UUID 应为带连字符的标准格式
    const std::string& entryUuid = entry->uuid;
    EXPECT_EQ(entryUuid.length(), 36u) << "Whitelist UUID should be 36 chars with dashes";

    // 带连字符的 UUID 与原始无连字符 UUID 应表示同一个值
    Uuid parsed = util::uuidFromString(rawUuid);
    std::string expectedWithDashes = util::uuidToStringWithDashes(parsed);
    EXPECT_EQ(entryUuid, expectedWithDashes);
}

} // namespace mc::command
