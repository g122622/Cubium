#include "common/core/DefaultValues.hpp"

#include <gtest/gtest.h>

TEST(DefaultValuesTest, GameDirectoryNamesAreExpected)
{
    EXPECT_STREQ(mc::defaults::game::clientOptionsFile, "client_options.json");
    EXPECT_STREQ(mc::defaults::game::serverOptionsFile, "server_options.json");
    EXPECT_STREQ(mc::defaults::game::resourcePacksDirName, "resourcepacks");
    EXPECT_STREQ(mc::defaults::game::dataPacksDirName, "datapacks");
}

TEST(DefaultValuesTest, ServerCoreDefaultsMatchExpectedValues)
{
    EXPECT_EQ(mc::defaults::serverCore::tickDurationMs, 50);
}
