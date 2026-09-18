#include "item/loot/LootTableLoader.hpp"
#include "common/TempDirHelper.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "item/Items.hpp"
#include "item/loot/LootTable.hpp"
#include "resource/pack/FolderResourcePack.hpp"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

using namespace mc;
using namespace mc::loot;

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

constexpr const char* kSimpleLootTableJson = R"({
  "type": "minecraft:block",
  "pools": [
    {
      "rolls": 1,
      "entries": [
        {
          "type": "minecraft:item",
          "name": "minecraft:diamond"
        }
      ]
    }
  ]
})";

} // namespace

class LootTableLoaderTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(LootTableLoaderTest, PathToLootTableId_StandardPath)
{
    LootTableManager manager;
    LootTableLoader loader(manager);

    // 旧格式（包含 data/ 前缀）
    EXPECT_EQ("minecraft:blocks/stone", loader.pathToLootTableId("data/minecraft/loot_tables/blocks/stone.json"));
    EXPECT_EQ("mod_id:entities/boss", loader.pathToLootTableId("data/mod_id/loot_tables/entities/boss.json"));

    // 新格式（相对于 data/ 根目录，不含 data/ 前缀）
    EXPECT_EQ("minecraft:blocks/stone", loader.pathToLootTableId("minecraft/loot_tables/blocks/stone.json"));
    EXPECT_EQ("mod_id:entities/boss", loader.pathToLootTableId("mod_id/loot_tables/entities/boss.json"));
}

TEST_F(LootTableLoaderTest, LoadFromDirectory_LoadsJsonFiles)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_loot_table_loader_test");
    writeTextFile(tempRoot / "data/minecraft/loot_tables/blocks/test_block.json", kSimpleLootTableJson);

    LootTableManager manager;
    LootTableLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/loot_tables").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(1u, result.value().successCount);
    EXPECT_EQ(0u, result.value().failedCount);
    EXPECT_TRUE(manager.hasTable("minecraft:blocks/test_block"));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(LootTableLoaderTest, LoadFromResourcePacks_EmptyPackRepositoryReturnsSuccess)
{
    PackRepository packList;

    LootTableManager manager;
    LootTableLoader loader(manager);
    auto result = loader.loadFromResourcePacks(packList);

    ASSERT_TRUE(result.success());
    EXPECT_EQ(0u, result.value().successCount);
    EXPECT_EQ(0u, result.value().failedCount);
}

TEST_F(LootTableLoaderTest, FolderResourcePack_ListResourcesAndReadTextForLootTables)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_loot_table_loader_test");
    const auto packDir = tempRoot / "pack_dir";

    writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":6,"description":"pack"}})");
    writeTextFile(packDir / "data/minecraft/loot_tables/blocks/test.json", kSimpleLootTableJson);

    FolderResourcePack pack(packDir.string());
    ASSERT_TRUE(pack.initialize().success());

    auto listResult = pack.listResources(resource::PackType::ServerData, "minecraft/loot_tables", ".json");
    ASSERT_TRUE(listResult.success());
    ASSERT_EQ(1u, listResult.value().size());
    EXPECT_NE(std::string::npos, listResult.value()[0].find("loot_tables"));

    auto readResult = pack.readTextResource(resource::PackType::ServerData, listResult.value()[0]);
    ASSERT_TRUE(readResult.success());
    EXPECT_NE(std::string::npos, readResult.value().find("\"minecraft:diamond\""));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
