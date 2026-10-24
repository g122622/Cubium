/**
 * @file BossBarCommandTest.cpp
 * @brief /bossbar 命令测试
 *
 * 覆盖 add/remove/list/set/get 全部子命令：命名空间 ID 解析、重复创建拒绝、
 * 未变化设置的失败路径、值/最大值/可见性读写、以及 remove 后查询失败。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/bossbar/BossInfo.hpp"
#include "server/bossbar/CustomServerBossInfo.hpp"
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/BossBarCommand.hpp"

using namespace mc;
using namespace mc::command;
using namespace mc::server;

namespace {

/// bossbar 命令测试用的最小服务器：BaseTestServer 未实现 bossBarManager，此处提供真实实例
class BossBarCommandTestServer final : public mc::test::BaseTestServer {
public:
    BossBarCommandTestServer()
        : m_bossBarManager(*this)
    {}

    ~BossBarCommandTestServer() override = default;

    [[nodiscard]] server::CustomServerBossInfoManager& bossBarManager() override
    {
        return m_bossBarManager;
    }

    [[nodiscard]] const server::CustomServerBossInfoManager& bossBarManager() const override
    {
        return m_bossBarManager;
    }

private:
    server::CustomServerBossInfoManager m_bossBarManager;
};

} // namespace

class BossBarCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        BossBarCommand::registerTo(m_server.commandRegistry().dispatcher());
        m_source = ServerCommandSource::forConsole(&m_server);
    }

    /// 执行命令并返回结果码（失败时返回 0）
    i32 _run(const std::string& input)
    {
        const auto result = m_server.commandRegistry().execute(input, m_source);
        return result.success() ? result.value() : 0;
    }

    BossBarCommandTestServer m_server;
    ServerCommandSource m_source = ServerCommandSource::forConsole(nullptr);
};

TEST_F(BossBarCommandTest, Add_CreatesBossBarAndStoresName)
{
    // 名称参数为单 token 短语：含空格的名称需加引号（与原版 QuotablePhrase 语义一致）
    EXPECT_EQ(_run("bossbar add test:bar \"My Bar\""), 1);

    auto* bossInfo = m_server.bossBarManager().get(ResourceLocation("test:bar"));
    ASSERT_NE(bossInfo, nullptr);
    EXPECT_EQ(bossInfo->name().getFormattedText(), "My Bar");
}

TEST_F(BossBarCommandTest, Add_DuplicateId_ReturnsZeroAndKeepsOriginal)
{
    EXPECT_EQ(_run("bossbar add test:bar First"), 1);
    EXPECT_EQ(_run("bossbar add test:bar Second"), 0);

    auto* bossInfo = m_server.bossBarManager().get(ResourceLocation("test:bar"));
    ASSERT_NE(bossInfo, nullptr);
    // 重复创建不覆盖既有名称
    EXPECT_EQ(bossInfo->name().getFormattedText(), "First");
}

TEST_F(BossBarCommandTest, SetAndGet_ValueRoundTrip)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    EXPECT_EQ(_run("bossbar set test:bar max 200"), 200);
    EXPECT_EQ(_run("bossbar set test:bar value 150"), 150);
    EXPECT_EQ(_run("bossbar get test:bar value"), 150);
    EXPECT_EQ(_run("bossbar get test:bar max"), 200);
}

TEST_F(BossBarCommandTest, SetValue_Unchanged_ReturnsZero)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    // 默认值与目标值相同时：原版抛 unchanged 异常，此处返回 0
    EXPECT_EQ(_run("bossbar set test:bar value 0"), 0);
}

TEST_F(BossBarCommandTest, SetVisible_TogglesVisibility)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    EXPECT_EQ(_run("bossbar set test:bar visible false"), 0);

    const auto* bossInfo = m_server.bossBarManager().get(ResourceLocation("test:bar"));
    ASSERT_NE(bossInfo, nullptr);
    EXPECT_FALSE(bossInfo->visible());

    // 重复设置为同一值：unchanged
    EXPECT_EQ(_run("bossbar set test:bar visible false"), 0);
    EXPECT_EQ(_run("bossbar get test:bar visible"), 0);

    EXPECT_EQ(_run("bossbar set test:bar visible true"), 0);
    EXPECT_EQ(_run("bossbar get test:bar visible"), 1);
}

TEST_F(BossBarCommandTest, SetColor_UpdatesColor)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    // 默认颜色 Pink，改为 Red 应成功
    EXPECT_EQ(_run("bossbar set test:bar color red"), 0);

    const auto* bossInfo = m_server.bossBarManager().get(ResourceLocation("test:bar"));
    ASSERT_NE(bossInfo, nullptr);
    EXPECT_EQ(bossInfo->color(), BossInfoColor::Red);

    // 再次设为 red：unchanged
    EXPECT_EQ(_run("bossbar set test:bar color red"), 0);
}

TEST_F(BossBarCommandTest, SetStyle_UpdatesOverlay)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    EXPECT_EQ(_run("bossbar set test:bar style notched_10"), 0);

    const auto* bossInfo = m_server.bossBarManager().get(ResourceLocation("test:bar"));
    ASSERT_NE(bossInfo, nullptr);
    EXPECT_EQ(bossInfo->overlay(), BossInfoOverlay::Notched10);
}

TEST_F(BossBarCommandTest, List_CountsRegisteredBars)
{
    EXPECT_EQ(_run("bossbar add test:a A"), 1);
    EXPECT_EQ(_run("bossbar add test:b B"), 2);
    EXPECT_EQ(_run("bossbar list"), 2);
}

TEST_F(BossBarCommandTest, Remove_DeletesBarAndLaterQueriesFail)
{
    EXPECT_EQ(_run("bossbar add test:bar Bar"), 1);
    EXPECT_EQ(_run("bossbar remove test:bar"), 0);
    EXPECT_EQ(m_server.bossBarManager().get(ResourceLocation("test:bar")), nullptr);

    // 已移除的 Boss 栏再次操作：未知 ID
    EXPECT_EQ(_run("bossbar remove test:bar"), 0);
    EXPECT_EQ(_run("bossbar get test:bar value"), 0);
}

TEST_F(BossBarCommandTest, UnknownId_SetFails)
{
    EXPECT_EQ(_run("bossbar set nope:missing value 1"), 0);
}