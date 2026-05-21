#include "item/Items.hpp"
#include "item/loot/LootSerializers.hpp"
#include "item/loot/entries/DynamicLootEntry.hpp"
#include "item/loot/entries/LootEntry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::loot;

class LootJsonIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(LootJsonIntegrationTest, ParseLootTable_WithTableFunctionsPoolFunctionsAndDynamicEntry)
{
    const char* json = R"({
      "type": "minecraft:chest",
      "functions": [
        { "function": "minecraft:set_count", "count": 1 }
      ],
      "pools": [
        {
          "rolls": 1,
          "functions": [
            { "function": "minecraft:set_name", "name": "crate" }
          ],
          "entries": [
            {
              "type": "minecraft:dynamic",
              "name": "minecraft:contents"
            }
          ]
        }
      ]
    })";

    auto result = LootSerializers::parseLootTable(std::string(json));
    ASSERT_TRUE(result.success());

    const auto& table = result.value();
    ASSERT_EQ(1u, table->getFunctions().size());
    ASSERT_EQ(1u, table->getPools().size());
    ASSERT_EQ(1u, table->getPools()[0]->getFunctions().size());
    ASSERT_EQ(1u, table->getPools()[0]->getEntries().size());

    const auto* dynamicEntry = dynamic_cast<const DynamicLootEntry*>(table->getPools()[0]->getEntries()[0].get());
    ASSERT_NE(dynamicEntry, nullptr);
    EXPECT_EQ("minecraft:contents", dynamicEntry->getName());
}

TEST_F(LootJsonIntegrationTest, ParseCondition_ReferenceRejected)
{
    nlohmann::json json = {{"condition", "minecraft:reference"}, {"name", "minecraft:test"}};
    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootJsonIntegrationTest, ParseCondition_TableBonusRejected)
{
    nlohmann::json json = {
        {"condition", "minecraft:table_bonus"}, {"enchantment", "minecraft:fortune"}, {"chances", {0.1, 0.2, 0.3}}};
    auto result = LootSerializers::parseCondition(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootJsonIntegrationTest, ParseFunction_CopyNbtRejected)
{
    nlohmann::json json = {{"function", "minecraft:copy_nbt"}, {"source", "block_entity"}};
    auto result = LootSerializers::parseFunction(json);
    EXPECT_FALSE(result.success());
}

TEST_F(LootJsonIntegrationTest, ParseFunction_ExplorationMapRejected)
{
    nlohmann::json json = {{"function", "minecraft:exploration_map"}, {"destination", "minecraft:mansion"}};
    auto result = LootSerializers::parseFunction(json);
    EXPECT_FALSE(result.success());
}
