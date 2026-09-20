#include "common/core/GameDirectory.hpp"
#include "common/TempDirHelper.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace mc;

/**
 * @brief 游戏目录路径推导测试
 *
 * GameDirectory 的工厂方法会对路径做弱规范化：解析已存在部分的符号链接、
 * 相对路径基于当前工作目录展开。因此用例必须在真实存在的临时目录上验证，
 * 不能断言硬编码的绝对路径字符串——形如 "C:/games/minecraft_reborn" 的路径
 * 只在 Windows 上是绝对路径，在 POSIX 上会被当成相对路径拼到工作目录之后。
 */
class GameDirectoryTest : public ::testing::Test {
protected:
    void SetUp() override { m_tempDir = mc::test::makeUniqueTestDir("mc_game_directory_test"); }

    void TearDown() override { mc::test::removeTestDir(m_tempDir); }

    /// 规范化后的临时目录，用作期望值（弱规范化的结果与之相等）
    [[nodiscard]] fs::path canonicalRoot() const { return fs::canonical(m_tempDir); }

    fs::path m_tempDir;
};

TEST_F(GameDirectoryTest, FromConfigPathUsesParentDirectory)
{
    const fs::path root = canonicalRoot();

    // 配置文件所在目录即为游戏根目录
    const auto dir = GameDirectory::fromConfigPath(m_tempDir / "client_options.json");

    EXPECT_TRUE(dir.isValid());
    EXPECT_EQ(dir.root().generic_string(), root.generic_string());
    EXPECT_EQ(dir.clientOptionsPath().generic_string(), (root / "client_options.json").generic_string());
    EXPECT_EQ(dir.serverOptionsPath().generic_string(), (root / "server_options.json").generic_string());
}

TEST_F(GameDirectoryTest, FromRootBuildsExpectedSubdirectories)
{
    const fs::path root = canonicalRoot();

    const auto dir = GameDirectory::fromRoot(m_tempDir);

    EXPECT_EQ(dir.root().generic_string(), root.generic_string());
    EXPECT_EQ(dir.resourcePacksDir().generic_string(), (root / "resourcepacks").generic_string());
    EXPECT_EQ(dir.dataPacksDir().generic_string(), (root / "datapacks").generic_string());
    EXPECT_EQ(dir.savesDir().generic_string(), (root / "saves").generic_string());
    EXPECT_EQ(dir.backupsDir().generic_string(), (root / "backups").generic_string());
    EXPECT_EQ(dir.logsDir().generic_string(), (root / "logs").generic_string());
}
