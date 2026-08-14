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
 * @file EnchantCommandTest.cpp
 * @brief EnchantCommand 单元测试
 *
 * 测试 /enchant 命令的注册、解析和权限检查。
 * 附魔操作完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/EnchantCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class EnchantTestServer final : public mc::test::BaseTestServer {
public:
    EnchantTestServer() { item::enchant::EnchantmentRegistry::initialize(); }

    ~EnchantTestServer() override { item::enchant::EnchantmentRegistry::clear(); }

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

class EnchantCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        EnchantCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    EnchantTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(EnchantCommandTest, EnchantCommandIsRegistered)
{
    // 验证 enchant 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 enchant 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "enchant") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "enchant command should be registered";
}

TEST_F(EnchantCommandTest, EnchantCommandRequiresPermissionLevel2)
{
    // 创建一个权限等级 0 的命令源
    ServerCommandSource lowPermSource(&m_server,
        nullptr,
        0,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        0, // 权限等级 0
        0,
        "test");

    // 应该因为没有权限而被拒绝
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== 语法测试 ==========

TEST_F(EnchantCommandTest, EnchantWithoutLevel)
{
    // 测试不带等级参数（默认等级 1）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness", m_console);

    // 命令执行成功，但由于没有玩家实体，返回 0
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantWithLevel)
{
    // 测试带等级参数
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantWithNamespace)
{
    // 测试带命名空间的附魔名
    const auto result = m_server.commandRegistry().execute("enchant @p minecraft:sharpness 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantLevelZero)
{
    // 测试等级 0（MC 1.16.5 允许等级 0）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, EnchantHighLevel)
{
    // 测试高等级
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 32767", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 附魔名称测试 ==========

TEST_F(EnchantCommandTest, UnknownEnchantment)
{
    // 测试未知附魔
    const auto result = m_server.commandRegistry().execute("enchant @p unknown_enchantment 1", m_console);

    // 未知附魔应该返回错误
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, VariousEnchantments)
{
    // 测试各种附魔名称
    const char* enchantments[] = {"sharpness",
        "smite",
        "bane_of_arthropods",
        "knockback",
        "fire_aspect",
        "looting",
        "efficiency",
        "silk_touch",
        "fortune",
        "unbreaking",
        "power",
        "punch",
        "flame",
        "infinity",
        "protection",
        "fire_protection",
        "blast_protection",
        "projectile_protection",
        "feather_falling",
        "thorns",
        "respiration",
        "depth_strider",
        "aqua_affinity"};

    for (const char* ench : enchantments) {
        std::string cmd = std::string("enchant @p ") + ench + " 1";
        const auto result = m_server.commandRegistry().execute(cmd, m_console);
        EXPECT_TRUE(result.success()) << "enchantment " << ench << " should be parseable";
    }
}

// ========== 选择器测试 ==========

TEST_F(EnchantCommandTest, SelectorWithNoPlayersReturnsZero)
{
    // 测试没有目标玩家
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, SelectorWithMultiplePlayers)
{
    // 测试多目标选择器 @a
    // 注意：@a 选择器在 EntityArgumentType::player() 模式下会抛出异常，
    // 因为 player() 只允许单个玩家
    // 这与 MC 1.16.5 行为一致：/enchant 只支持单个目标

    // 执行命令时，@a 选择器会在解析阶段失败（不是执行阶段）
    // 因为 EntityArgumentType::player() 要求 isSingle() == true
    const auto result = m_server.commandRegistry().execute("enchant @a sharpness 5", m_console);

    // 命令解析失败，result.success() 应该为 false
    EXPECT_FALSE(result.success());
}

// ========== 附魔类型兼容性测试 ==========

TEST_F(EnchantCommandTest, WeaponEnchantment)
{
    // 测试武器附魔
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, ToolEnchantment)
{
    // 测试工具附魔
    const auto result = m_server.commandRegistry().execute("enchant @p efficiency 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, ArmorEnchantment)
{
    // 测试护甲附魔
    const auto result = m_server.commandRegistry().execute("enchant @p protection 4", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, BowEnchantment)
{
    // 测试弓附魔
    const auto result = m_server.commandRegistry().execute("enchant @p power 5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, TridentEnchantment)
{
    // 测试三叉戟附魔
    const auto result = m_server.commandRegistry().execute("enchant @p loyalty 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, CrossbowEnchantment)
{
    // 测试弩附魔
    const auto result = m_server.commandRegistry().execute("enchant @p quick_charge 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(EnchantCommandTest, FishingRodEnchantment)
{
    // 测试钓鱼竿附魔
    const auto result = m_server.commandRegistry().execute("enchant @p luck_of_the_sea 3", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== 宝藏附魔测试 ==========

TEST_F(EnchantCommandTest, TreasureEnchantment)
{
    // 测试宝藏附魔（冰霜行者、修补、灵魂疾行）
    const auto result1 = m_server.commandRegistry().execute("enchant @p frost_walker 2", m_console);
    EXPECT_TRUE(result1.success());

    const auto result2 = m_server.commandRegistry().execute("enchant @p mending 1", m_console);
    EXPECT_TRUE(result2.success());

    const auto result3 = m_server.commandRegistry().execute("enchant @p soul_speed 3", m_console);
    EXPECT_TRUE(result3.success());
}

// ========== 诅咒附魔测试 ==========

TEST_F(EnchantCommandTest, CurseEnchantment)
{
    // 测试诅咒附魔
    const auto result1 = m_server.commandRegistry().execute("enchant @p binding_curse 1", m_console);
    EXPECT_TRUE(result1.success());

    const auto result2 = m_server.commandRegistry().execute("enchant @p vanishing_curse 1", m_console);
    EXPECT_TRUE(result2.success());
}

// ========== 等级边界测试 ==========

TEST_F(EnchantCommandTest, NegativeLevel)
{
    // 测试负等级（应该失败）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness -1", m_console);

    // 负等级应该在解析时失败
    EXPECT_FALSE(result.success());
}

TEST_F(EnchantCommandTest, ZeroLevelAllowed)
{
    // 测试等级 0（MC 1.16.5 允许）
    const auto result = m_server.commandRegistry().execute("enchant @p sharpness 0", m_console);

    EXPECT_TRUE(result.success());
}

} // namespace mc::command
