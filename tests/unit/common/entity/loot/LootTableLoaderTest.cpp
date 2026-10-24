#include "item/loot/LootTableLoader.hpp"
#include "common/TempDirHelper.hpp"
#include "common/resource/repository/PackRepository.hpp"
#include "item/Items.hpp"
#include "item/enchantment/EnchantmentRegistry.hpp"
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
    void SetUp() override
    {
        Items::ensureInitialized();
        item::enchant::EnchantmentRegistry::initialize();
    }
};

TEST_F(LootTableLoaderTest, PathToLootTableId_StandardPath)
{
    LootTableManager manager;
    LootTableLoader loader(manager);

    // <namespace>/loot_table/<category>/<name>.json（MC 1.21+ 单数目录名）
    EXPECT_EQ("minecraft:blocks/stone", loader.pathToLootTableId("minecraft/loot_table/blocks/stone.json"));
    EXPECT_EQ("mod_id:entities/boss", loader.pathToLootTableId("mod_id/loot_table/entities/boss.json"));
}

TEST_F(LootTableLoaderTest, LoadFromDirectory_LoadsJsonFiles)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_loot_table_loader_test");
    writeTextFile(tempRoot / "data/minecraft/loot_table/blocks/test_block.json", kSimpleLootTableJson);

    LootTableManager manager;
    LootTableLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/loot_table").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(1u, result.value().successCount);
    EXPECT_EQ(0u, result.value().failedCount);
    EXPECT_TRUE(manager.hasTable("minecraft:blocks/test_block"));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

TEST_F(LootTableLoaderTest, LoadFromDirectory_AllVanillaFunctionTypesSupported)
{
    // 回归：原版数据包中使用的全部函数类型（set_potion/copy_components/set_components 等）
    // 都必须能加载，本文件涵盖其中每一种，期望 0 失败。
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_loot_table_loader_test");
    writeTextFile(tempRoot / "data/minecraft/loot_table/chests/test_chest.json",
        R"({
  "type": "minecraft:chest",
  "pools": [
    {
      "rolls": 1,
      "entries": [
        {
          "type": "minecraft:item",
          "name": "minecraft:potion",
          "functions": [
            { "function": "minecraft:set_potion", "id": "minecraft:water" },
            { "function": "minecraft:set_count", "count": 2 },
            { "function": "minecraft:set_enchantments", "enchantments": { "minecraft:fortune": 2 } },
            { "function": "minecraft:set_components", "components": { "minecraft:enchantment_glint_override": true } },
            { "function": "minecraft:copy_components", "source": "block_entity", "include": [ "minecraft:custom_name" ] }
          ]
        }
      ]
    }
  ]
})");
    writeTextFile(tempRoot / "data/minecraft/loot_table/chests/test_invalid.json",
        R"({
  "type": "minecraft:chest",
  "pools": [
    { "rolls": 1, "entries": [ { "type": "minecraft:item", "name": "minecraft:stone" } ],
      "functions": [ { "function": "minecraft:set_custom_data", "tag": "{broken" } ] }
  ]
})");

    LootTableManager manager;
    LootTableLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/loot_table").string());

    ASSERT_TRUE(result.success());
    EXPECT_EQ(1u, result.value().successCount);
    EXPECT_EQ(1u, result.value().failedCount);

    // 表可用，全部函数按顺序保留
    ASSERT_TRUE(manager.hasTable("minecraft:chests/test_chest"));
    const LootTable* table = manager.getTable("minecraft:chests/test_chest");
    ASSERT_NE(table, nullptr);
    ASSERT_EQ(table->getPools().size(), 1u);
    const auto& entries = table->getPools()[0]->getEntries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(5u, entries[0]->getFunctions().size());

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
    writeTextFile(packDir / "data/minecraft/loot_table/blocks/test.json", kSimpleLootTableJson);

    FolderResourcePack pack(packDir.string());
    ASSERT_TRUE(pack.initialize().success());

    auto listResult = pack.listResources(resource::PackType::ServerData, "minecraft/loot_table", ".json");
    ASSERT_TRUE(listResult.success());
    ASSERT_EQ(1u, listResult.value().size());
    EXPECT_NE(std::string::npos, listResult.value()[0].find("loot_table"));

    auto readResult = pack.readTextResource(resource::PackType::ServerData, listResult.value()[0]);
    ASSERT_TRUE(readResult.success());
    EXPECT_NE(std::string::npos, readResult.value().find("\"minecraft:diamond\""));

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
