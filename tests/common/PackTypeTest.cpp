#include "common/resource/PackType.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

using namespace mc;

TEST(PackTypeTest, DirectoryNamesMatchMinecraftLayout)
{
    EXPECT_EQ(std::string(resource::packTypeDirectoryName(resource::PackType::ClientResources)), "assets");
    EXPECT_EQ(std::string(resource::packTypeDirectoryName(resource::PackType::ServerData)), "data");
}

TEST(PackTypeTest, ResourceLocationBuildsTypedPaths)
{
    const ResourceLocation location("minecraft:textures/block/stone");
    EXPECT_EQ(location.toFilePath(resource::PackType::ClientResources), "assets/minecraft/textures/block/stone");
    EXPECT_EQ(
        location.toFilePath(resource::PackType::ClientResources, "png"), "assets/minecraft/textures/block/stone.png");

    const ResourceLocation dataLocation("minecraft:loot_tables/blocks/stone");
    EXPECT_EQ(dataLocation.toFilePath(resource::PackType::ServerData), "data/minecraft/loot_tables/blocks/stone");
    EXPECT_EQ(dataLocation.toFilePath(resource::PackType::ServerData, "json"),
        "data/minecraft/loot_tables/blocks/stone.json");
}
