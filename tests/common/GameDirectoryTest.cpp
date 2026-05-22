#include "common/core/GameDirectory.hpp"

#include <gtest/gtest.h>

using namespace mc;

TEST(GameDirectoryTest, FromConfigPathUsesParentDirectory)
{
    const auto dir = GameDirectory::fromConfigPath("C:/games/minecraft_reborn/client_options.json");
    EXPECT_EQ(dir.root().generic_string(), "C:/games/minecraft_reborn");
    EXPECT_EQ(dir.clientOptionsPath().generic_string(), "C:/games/minecraft_reborn/client_options.json");
    EXPECT_EQ(dir.serverOptionsPath().generic_string(), "C:/games/minecraft_reborn/server_options.json");
}

TEST(GameDirectoryTest, FromRootBuildsExpectedSubdirectories)
{
    const auto dir = GameDirectory::fromRoot("C:/games/minecraft_reborn");
    EXPECT_EQ(dir.resourcePacksDir().generic_string(), "C:/games/minecraft_reborn/resourcepacks");
    EXPECT_EQ(dir.dataPacksDir().generic_string(), "C:/games/minecraft_reborn/datapacks");
    EXPECT_EQ(dir.savesDir().generic_string(), "C:/games/minecraft_reborn/saves");
    EXPECT_EQ(dir.backupsDir().generic_string(), "C:/games/minecraft_reborn/backups");
    EXPECT_EQ(dir.logsDir().generic_string(), "C:/games/minecraft_reborn/logs");
}
