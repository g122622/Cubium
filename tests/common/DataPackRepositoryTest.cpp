#include "common/resource/repository/DataPackRepository.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mc::resource;

namespace {

std::filesystem::path createDataPackDir()
{
    const auto dir = std::filesystem::temp_directory_path() / "mc_datapack_list_test";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir / "example_pack" / "data/minecraft/loot_tables/blocks");

    std::ofstream mcmeta(dir / "example_pack/pack.mcmeta", std::ios::binary);
    mcmeta << R"({"pack":{"pack_format":6,"description":"test"}})";
    mcmeta.close();

    std::ofstream loot(dir / "example_pack/data/minecraft/loot_tables/blocks/stone.json", std::ios::binary);
    loot << R"({"type":"minecraft:block","pools":[]})";
    loot.close();

    return dir;
}

} // namespace

TEST(DataPackRepositoryTest, ScanDirectoryFindsFolderPack)
{
    const auto dir = createDataPackDir();
    DataPackRepository list;

    const auto result = list.scanDirectory(dir);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value(), static_cast<size_t>(1));
    EXPECT_EQ(list.packCount(), static_cast<size_t>(1));
}

TEST(DataPackRepositoryTest, ReadResourceUsesServerDataRoot)
{
    const auto dir = createDataPackDir();
    DataPackRepository list;
    ASSERT_TRUE(list.scanDirectory(dir).success());

    const auto readResult = list.readTextResource("minecraft/loot_tables/blocks/stone.json");
    ASSERT_TRUE(readResult.success());
    EXPECT_NE(readResult.value().find("\"minecraft:block\""), std::string::npos);
}
