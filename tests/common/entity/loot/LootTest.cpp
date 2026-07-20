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

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/loot/LootPool.hpp"
#include "item/loot/LootTable.hpp"
#include "item/loot/LootTableLoader.hpp"
#include "item/loot/LootTableManager.hpp"
#include "item/loot/conditions/LootConditions.hpp"
#include "item/loot/context/LootContext.hpp"
#include "item/loot/entries/EmptyLootEntry.hpp"
#include "item/loot/entries/ItemLootEntry.hpp"
#include "item/loot/entries/LootEntry.hpp"
#include "item/loot/entries/LootEntryBuilder.hpp"
#include "item/loot/functions/LootFunctions.hpp"
#include "resource/ResourceLocation.hpp"
#include "util/math/random/Random.hpp"
#include "util/math/random/RandomRanges.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace mc;
using namespace mc::loot;

namespace {

std::filesystem::path makeUniqueLootTempDir()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto dir = base / ("mc_loot_test_" + std::to_string(static_cast<long long>(now)));
    std::filesystem::create_directories(dir);
    return dir;
}

void writeLootFile(const std::filesystem::path& root, const std::string& relativePath, const std::string& json)
{
    const auto fullPath = root / relativePath;
    std::filesystem::create_directories(fullPath.parent_path());
    std::ofstream file(fullPath, std::ios::binary);
    file << json;
}

std::unique_ptr<LootContext> buildBlockLootContext(IWorld& world, math::Random& rng, const BlockState& state)
{
    static const BlockPos s_blockPos(8, 64, -5);
    return LootContextBuilder(world)
        .withRandom(rng)
        .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(&state))
        .withParameter(LootParams::BLOCK_POS, const_cast<BlockPos*>(&s_blockPos))
        .build();
}

void loadLegacyEquivalentLootTables(LootTableManager& manager)
{
    const auto tempRoot = makeUniqueLootTempDir();

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/blocks/diamond_ore.json",
        R"({
          "type": "minecraft:block",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:diamond_ore",
                  "conditions": [{ "condition": "minecraft:silk_touch" }]
                }
              ]
            },
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:diamond",
                  "conditions": [
                    {
                      "condition": "minecraft:inverted",
                      "term": { "condition": "minecraft:silk_touch" }
                    }
                  ],
                  "functions": [
                    {
                      "function": "minecraft:apply_bonus",
                      "formula": "minecraft:ore_drops"
                    }
                  ]
                }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/blocks/coal_ore.json",
        R"({
          "type": "minecraft:block",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:coal_ore",
                  "conditions": [{ "condition": "minecraft:silk_touch" }]
                }
              ]
            },
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:coal",
                  "conditions": [
                    {
                      "condition": "minecraft:inverted",
                      "term": { "condition": "minecraft:silk_touch" }
                    }
                  ],
                  "functions": [
                    {
                      "function": "minecraft:apply_bonus",
                      "formula": "minecraft:ore_drops"
                    }
                  ]
                }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/blocks/stone.json",
        R"({
          "type": "minecraft:block",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:stone",
                  "conditions": [{ "condition": "minecraft:silk_touch" }]
                }
              ]
            },
            {
              "rolls": 1,
              "entries": [
                {
                  "type": "minecraft:item",
                  "name": "minecraft:cobblestone",
                  "conditions": [
                    {
                      "condition": "minecraft:inverted",
                      "term": { "condition": "minecraft:silk_touch" }
                    }
                  ]
                }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/gameplay/fishing/fish.json",
        R"({
          "type": "minecraft:fishing",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                { "type": "minecraft:item", "name": "minecraft:cod", "weight": 60 },
                { "type": "minecraft:item", "name": "minecraft:salmon", "weight": 25 },
                { "type": "minecraft:item", "name": "minecraft:tropical_fish", "weight": 2 },
                { "type": "minecraft:item", "name": "minecraft:pufferfish", "weight": 13 }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/gameplay/fishing/junk.json",
        R"({
          "type": "minecraft:fishing",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                { "type": "minecraft:item", "name": "minecraft:leather_boots", "weight": 10, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:leather", "weight": 10, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:bone", "weight": 10, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:string", "weight": 5, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:fishing_rod", "weight": 2, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:bowl", "weight": 10, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:stick", "weight": 5, "quality": -2 },
                { "type": "minecraft:item", "name": "minecraft:ink_sac", "weight": 1, "quality": -2, "functions": [{ "function": "minecraft:set_count", "count": 10 }] },
                { "type": "minecraft:item", "name": "minecraft:rotten_flesh", "weight": 10, "quality": -2 }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/gameplay/fishing/treasure.json",
        R"({
          "type": "minecraft:fishing",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                { "type": "minecraft:item", "name": "minecraft:name_tag", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:item", "name": "minecraft:saddle", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:item", "name": "minecraft:bow", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:item", "name": "minecraft:fishing_rod", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:item", "name": "minecraft:book", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:item", "name": "minecraft:nautilus_shell", "weight": 1, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/gameplay/fishing.json",
        R"({
          "type": "minecraft:fishing",
          "pools": [
            {
              "rolls": 1,
              "entries": [
                { "type": "minecraft:loot_table", "value": "minecraft:gameplay/fishing/junk", "weight": 10, "quality": -2 },
                { "type": "minecraft:loot_table", "value": "minecraft:gameplay/fishing/treasure", "weight": 5, "quality": 2, "conditions": [{ "condition": "minecraft:fishing_hook_in_open_water" }] },
                { "type": "minecraft:loot_table", "value": "minecraft:gameplay/fishing/fish", "weight": 85, "quality": -1 }
              ]
            }
          ]
        })");

    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/entities/pig.json",
        R"({"type":"minecraft:entity","pools":[{"rolls":{"min":1,"max":3},"entries":[{"type":"minecraft:item","name":"minecraft:porkchop"}]}]})");
    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/entities/cow.json",
        R"({"type":"minecraft:entity","pools":[{"rolls":{"min":1,"max":3},"entries":[{"type":"minecraft:item","name":"minecraft:beef"}]}]})");
    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/entities/sheep.json",
        R"({"type":"minecraft:entity","pools":[{"rolls":1,"entries":[{"type":"minecraft:item","name":"minecraft:wool"}]},{"rolls":{"min":1,"max":2},"entries":[{"type":"minecraft:item","name":"minecraft:mutton"}]}]})");
    writeLootFile(tempRoot,
        "data/minecraft/loot_tables/entities/chicken.json",
        R"({"type":"minecraft:entity","pools":[{"rolls":{"min":1,"max":2},"entries":[{"type":"minecraft:item","name":"minecraft:chicken"},{"type":"minecraft:item","name":"minecraft:feather","functions":[{"function":"minecraft:set_count","count":{"min":0,"max":2}}]}]}]})");

    LootTableLoader loader(manager);
    auto result = loader.loadFromDirectory((tempRoot / "data/minecraft/loot_tables").string());
    ASSERT_TRUE(result.success());
    ASSERT_EQ(11u, result.value().successCount);

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}

} // namespace

// Test implementation of IWorld for loot testing
class LootTestWorld : public test::BaseTestWorld {
public:
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("LootTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("LootTestWorld::tickManager not implemented");
    }
};

class LootTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    LootTestWorld m_world;
};

// RandomValueRange Tests
TEST_F(LootTest, RandomValueRange_FixedValue)
{
    RandomValueRange range(5.0f);
    math::Random rng(12345);
    EXPECT_EQ(5, range.generateInt(rng));
    EXPECT_FLOAT_EQ(5.0f, range.generateFloat(rng));
    EXPECT_TRUE(range.isFixed());
}

TEST_F(LootTest, RandomValueRange_Range)
{
    RandomValueRange range(1.0f, 10.0f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 1);
        EXPECT_LE(value, 10);
    }
}

// BinomialRange Tests
TEST_F(LootTest, BinomialRange_Basic)
{
    BinomialRange range(10, 0.5f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 0);
        EXPECT_LE(value, 10);
    }
}

TEST_F(LootTest, BinomialRange_ZeroProbability)
{
    BinomialRange range(10, 0.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(0, range.generateInt(rng));
    }
}

TEST_F(LootTest, BinomialRange_FullProbability)
{
    BinomialRange range(10, 1.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(10, range.generateInt(rng));
    }
}

// ConstantRange Tests
TEST_F(LootTest, ConstantRange_Basic)
{
    ConstantRange range(42);
    math::Random rng(12345);
    // 始终返回固定值
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(42, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_ZeroValue)
{
    ConstantRange range(0);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(0, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_NegativeValue)
{
    ConstantRange range(-5);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(-5, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_LargeValue)
{
    ConstantRange range(1000000);
    math::Random rng(12345);
    EXPECT_EQ(1000000, range.generateInt(rng));
    EXPECT_EQ(1000000, range.getValue());
}

TEST_F(LootTest, ConstantRange_GetValue)
{
    ConstantRange range(123);
    EXPECT_EQ(123, range.getValue());
}

// LootContext Tests
TEST_F(LootTest, LootContext_Builder)
{
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).withLuck(1.5f).build();

    ASSERT_NE(context, nullptr);
    EXPECT_FLOAT_EQ(1.5f, context->getLuck());
}

TEST_F(LootTest, LootContext_LootingModifier)
{
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).withLootingModifier(3).build();

    ASSERT_NE(context, nullptr);
    EXPECT_EQ(3, context->getLootingModifier());
}

TEST_F(LootTest, LootParameterSet_BlockAllowsMissingTool)
{
    VanillaBlocks::initialize();

    const BlockState* state = VanillaBlocks::DIRT ? &VanillaBlocks::DIRT->getDefaultState() : nullptr;
    ASSERT_NE(state, nullptr);

    const BlockPos pos(12, 64, -3);
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
                       .withRandom(rng)
                       .withParameter(LootParams::BLOCK_STATE, const_cast<BlockState*>(state))
                       .withParameter(LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos))
                       .build();

    ASSERT_NE(context, nullptr);
    EXPECT_TRUE(LootParameterSets::block().validate(context->getParamIds()));
}

// LootEntry Tests
TEST_F(LootTest, EmptyLootEntry_GenerateNothing)
{
    EmptyLootEntry entry;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    std::vector<ItemStack> items;
    bool success = entry.generate([&items](const ItemStack& stack) { items.push_back(stack); }, *context);

    EXPECT_TRUE(success);
    EXPECT_TRUE(items.empty());
}

TEST_F(LootTest, ItemLootEntry_Weight)
{
    ItemLootEntry entry("minecraft:porkchop", RandomValueRange(1.0f), 10, 2);
    EXPECT_EQ(10, entry.getWeight());
    EXPECT_EQ(2, entry.getQuality());
    EXPECT_EQ(10, entry.getEffectiveWeight(0.0f));
    EXPECT_EQ(12, entry.getEffectiveWeight(1.0f));
    EXPECT_EQ(8, entry.getEffectiveWeight(-1.0f));
}

// LootTable Tests
TEST_F(LootTest, LootTable_Empty)
{
    LootTable table;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    auto items = table.generate(*context);
    EXPECT_TRUE(items.empty());
}

// LootTableManager Tests
TEST_F(LootTest, LootTableManager_RegisterAndGet)
{
    LootTableManager manager;
    auto table = std::make_unique<LootTable>();
    table->addPool(std::make_unique<LootPool>(RandomValueRange(1.0f)));
    manager.registerTable("test:pig", std::move(table));

    EXPECT_TRUE(manager.hasTable("test:pig"));
    EXPECT_FALSE(manager.hasTable("test:cow"));

    const LootTable* retrieved = manager.getTable("test:pig");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(static_cast<size_t>(1), retrieved->poolCount());
}

TEST_F(LootTest, LootTableManager_DefaultTables)
{
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    EXPECT_TRUE(manager.hasTable("minecraft:entities/pig"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/cow"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/sheep"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/chicken"));
}

// ============================================================================
// New LootFunction Tests
// ============================================================================

TEST_F(LootTest, CopyNameFunction_Creation)
{
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    EXPECT_EQ("copy_name", func.getType());
    EXPECT_EQ(CopyNameFunction::Source::KillerPlayer, func.getSource());
}

TEST_F(LootTest, CopyNameFunction_Clone)
{
    CopyNameFunction func(CopyNameFunction::Source::This);
    func.addCondition(std::make_unique<RandomChanceCondition>(0.5f));

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ("copy_name", cloned->getType());

    auto* clonedFunc = dynamic_cast<CopyNameFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNameFunction::Source::This, clonedFunc->getSource());
}

TEST_F(LootTest, CopyNameFunction_AllSources)
{
    // 测试所有来源类型
    CopyNameFunction funcThis(CopyNameFunction::Source::This);
    CopyNameFunction funcKiller(CopyNameFunction::Source::Killer);
    CopyNameFunction funcPlayer(CopyNameFunction::Source::KillerPlayer);
    CopyNameFunction funcBlock(CopyNameFunction::Source::BlockEntity);

    EXPECT_EQ(CopyNameFunction::Source::This, funcThis.getSource());
    EXPECT_EQ(CopyNameFunction::Source::Killer, funcKiller.getSource());
    EXPECT_EQ(CopyNameFunction::Source::KillerPlayer, funcPlayer.getSource());
    EXPECT_EQ(CopyNameFunction::Source::BlockEntity, funcBlock.getSource());
}

TEST_F(LootTest, CopyBlockStateFunction_Creation)
{
    CopyBlockStateFunction func("minecraft:chest");
    EXPECT_EQ("copy_block_state", func.getType());
    EXPECT_EQ("minecraft:chest", func.getBlockId());
    EXPECT_TRUE(func.getProperties().empty());
}

TEST_F(LootTest, CopyBlockStateFunction_Properties)
{
    std::vector<std::string> props = {"facing", "waterlogged"};
    CopyBlockStateFunction func("minecraft:chest", props);
    EXPECT_EQ(2, func.getProperties().size());
    EXPECT_EQ("facing", func.getProperties()[0]);
    EXPECT_EQ("waterlogged", func.getProperties()[1]);
}

TEST_F(LootTest, CopyBlockStateFunction_Clone)
{
    std::vector<std::string> props = {"facing"};
    CopyBlockStateFunction func("minecraft:furnace", props);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<CopyBlockStateFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ("minecraft:furnace", clonedFunc->getBlockId());
    EXPECT_EQ(1, clonedFunc->getProperties().size());
}

TEST_F(LootTest, CopyNbtFunction_Creation)
{
    CopyNbtFunction func(CopyNbtFunction::Source::This);
    EXPECT_EQ("copy_nbt", func.getType());
    EXPECT_EQ(CopyNbtFunction::Source::This, func.getSource());
    EXPECT_TRUE(func.getOperations().empty());
}

TEST_F(LootTest, CopyNbtFunction_AddOperation)
{
    CopyNbtFunction func(CopyNbtFunction::Source::BlockEntity);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);

    EXPECT_EQ(1, func.getOperations().size());
    EXPECT_EQ("CustomName", func.getOperations()[0].sourcePath);
    EXPECT_EQ("display.Name", func.getOperations()[0].targetPath);
    EXPECT_EQ(CopyNbtFunction::Operation::Replace, func.getOperations()[0].operation);
}

TEST_F(LootTest, CopyNbtFunction_Clone)
{
    CopyNbtFunction func(CopyNbtFunction::Source::Killer);
    func.addOperation("path1", "path2", CopyNbtFunction::Operation::Append);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<CopyNbtFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNbtFunction::Source::Killer, clonedFunc->getSource());
    EXPECT_EQ(1, clonedFunc->getOperations().size());
}

TEST_F(LootTest, CopyNbtFunction_EmptyStack)
{
    CopyNbtFunction func(CopyNbtFunction::Source::This);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);

    math::Random rng(42);
    LootContext context(m_world, rng);
    ItemStack emptyStack;
    auto result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, CopyNbtFunction_NoOperations)
{
    CopyNbtFunction func(CopyNbtFunction::Source::This);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(42);
    LootContext context(m_world, rng);
    auto result = func.apply(stack, context);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(LootTest, CopyNbtFunction_NoSourceEntity)
{
    CopyNbtFunction func(CopyNbtFunction::Source::This);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 没有设置 THIS_ENTITY，应该返回原物品
    math::Random rng(42);
    LootContext context(m_world, rng);
    auto result = func.apply(stack, context);
    EXPECT_FALSE(result.isEmpty());
    // 物品不应有自定义数据标签
    EXPECT_FALSE(result.hasTag());
}

TEST_F(LootTest, CopyNbtFunction_ReplaceOperation)
{
    CopyNbtFunction func(CopyNbtFunction::Source::BlockEntity);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 设置方块实体到 LootContext，并给它一个自定义名称
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    BlockEntity* blockEntity = &chest;
    chest.setCustomName("Test Chest");

    math::Random rng(42);
    LootContext context(m_world, rng);
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    auto result = func.apply(stack, context);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(LootTest, CopyNbtFunction_MultipleOperations)
{
    CopyNbtFunction func(CopyNbtFunction::Source::BlockEntity);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);
    func.addOperation("Items", "Items", CopyNbtFunction::Operation::Merge);

    EXPECT_EQ(2, func.getOperations().size());
    EXPECT_EQ(CopyNbtFunction::Operation::Replace, func.getOperations()[0].operation);
    EXPECT_EQ(CopyNbtFunction::Operation::Merge, func.getOperations()[1].operation);
}

TEST_F(LootTest, CopyNbtFunction_AllSources)
{
    // 测试所有来源类型的构造
    CopyNbtFunction funcThis(CopyNbtFunction::Source::This);
    EXPECT_EQ(CopyNbtFunction::Source::This, funcThis.getSource());

    CopyNbtFunction funcKiller(CopyNbtFunction::Source::Killer);
    EXPECT_EQ(CopyNbtFunction::Source::Killer, funcKiller.getSource());

    CopyNbtFunction funcKillerPlayer(CopyNbtFunction::Source::KillerPlayer);
    EXPECT_EQ(CopyNbtFunction::Source::KillerPlayer, funcKillerPlayer.getSource());

    CopyNbtFunction funcBlockEntity(CopyNbtFunction::Source::BlockEntity);
    EXPECT_EQ(CopyNbtFunction::Source::BlockEntity, funcBlockEntity.getSource());
}

TEST_F(LootTest, CopyNbtFunction_AllOperations)
{
    CopyNbtFunction func(CopyNbtFunction::Source::This);
    func.addOperation("a", "b", CopyNbtFunction::Operation::Replace);
    func.addOperation("c", "d", CopyNbtFunction::Operation::Append);
    func.addOperation("e", "f", CopyNbtFunction::Operation::Merge);

    EXPECT_EQ(3, func.getOperations().size());
    EXPECT_EQ(CopyNbtFunction::Operation::Replace, func.getOperations()[0].operation);
    EXPECT_EQ(CopyNbtFunction::Operation::Append, func.getOperations()[1].operation);
    EXPECT_EQ(CopyNbtFunction::Operation::Merge, func.getOperations()[2].operation);
}

TEST_F(LootTest, CopyNbtFunction_CloneWithOperations)
{
    CopyNbtFunction func(CopyNbtFunction::Source::BlockEntity);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);
    func.addOperation("Items", "Items", CopyNbtFunction::Operation::Merge);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<CopyNbtFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNbtFunction::Source::BlockEntity, clonedFunc->getSource());
    EXPECT_EQ(2, clonedFunc->getOperations().size());
    EXPECT_EQ("CustomName", clonedFunc->getOperations()[0].sourcePath);
    EXPECT_EQ("display.Name", clonedFunc->getOperations()[0].targetPath);
    EXPECT_EQ(CopyNbtFunction::Operation::Replace, clonedFunc->getOperations()[0].operation);
    EXPECT_EQ("Items", clonedFunc->getOperations()[1].sourcePath);
    EXPECT_EQ("Items", clonedFunc->getOperations()[1].targetPath);
    EXPECT_EQ(CopyNbtFunction::Operation::Merge, clonedFunc->getOperations()[1].operation);
}

TEST_F(LootTest, FillPlayerHeadFunction_Creation)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    EXPECT_EQ("fill_player_head", func.getType());
    EXPECT_EQ(CopyNameFunction::Source::KillerPlayer, func.getSource());
}

TEST_F(LootTest, FillPlayerHeadFunction_Clone)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::This);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<FillPlayerHeadFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNameFunction::Source::This, clonedFunc->getSource());
}

// ============================================================================
// FillPlayerHeadFunction Apply 测试
// ============================================================================

TEST_F(LootTest, FillPlayerHeadFunction_EmptyStack)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setUuid("550e8400-e29b-41d4-a716-446655440000");
    context.set(LootParams::KILLER_PLAYER, &player);

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, context);

    // 空物品堆应保持空
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, FillPlayerHeadFunction_NoPlayerInContext)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 不设置玩家，使用玩家头颅物品
    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // 没有玩家信息，不应写入 SkullOwner 标签
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));
}

TEST_F(LootTest, FillPlayerHeadFunction_KillerPlayerWithUUID)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家并设置 UUID
    Player player(EntityInstanceId(1), "Steve");
    player.setUuid("550e8400-e29b-41d4-a716-446655440000");
    context.set(LootParams::KILLER_PLAYER, &player);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // 检查 SkullOwner 标签
    EXPECT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("SkullOwner"));

    const auto& skullOwner = (*tag)["SkullOwner"];
    EXPECT_TRUE(skullOwner.is_object());
    EXPECT_EQ("Steve", skullOwner["Name"].get<std::string>());
    EXPECT_EQ("550e8400-e29b-41d4-a716-446655440000", skullOwner["Id"].get<std::string>());
}

TEST_F(LootTest, FillPlayerHeadFunction_KillerPlayerNoUUID)
{
    // 注意：Entity 构造函数会自动生成随机 UUID，所以 Player 总是有有效 UUID
    // 这个测试验证当 Player 构造后，UUID 已自动设置
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家，不手动设置 UUID（但 Entity 构造函数会自动生成）
    Player player(EntityInstanceId(1), "AutoUUIDPlayer");
    context.set(LootParams::KILLER_PLAYER, &player);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // Entity 构造函数自动生成了 UUID，所以 SkullOwner 会被写入
    EXPECT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("SkullOwner"));

    const auto& skullOwner = (*tag)["SkullOwner"];
    EXPECT_EQ("AutoUUIDPlayer", skullOwner["Name"].get<std::string>());
    // UUID 是自动生成的，应该是有效的格式
    EXPECT_TRUE(skullOwner.contains("Id"));
}

TEST_F(LootTest, FillPlayerHeadFunction_SourceThis)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家作为 THIS_ENTITY
    Player player(EntityInstanceId(2), "Alex");
    player.setUuid("12345678-1234-1234-1234-123456789abc");
    Entity* entity = &player; // 显式转换为 Entity*
    context.set(LootParams::THIS_ENTITY, entity);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // 检查 SkullOwner 标签
    EXPECT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("SkullOwner"));

    const auto& skullOwner = (*tag)["SkullOwner"];
    EXPECT_EQ("Alex", skullOwner["Name"].get<std::string>());
    EXPECT_EQ("12345678-1234-1234-1234-123456789abc", skullOwner["Id"].get<std::string>());
}

TEST_F(LootTest, FillPlayerHeadFunction_SourceKiller)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::Killer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家作为 KILLER_ENTITY
    Player player(EntityInstanceId(3), "KillerSteve");
    player.setUuid("abcdef12-3456-7890-abcd-ef1234567890");
    Entity* entity = &player; // 显式转换为 Entity*
    context.set(LootParams::KILLER_ENTITY, entity);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // 检查 SkullOwner 标签
    EXPECT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->contains("SkullOwner"));

    const auto& skullOwner = (*tag)["SkullOwner"];
    EXPECT_EQ("KillerSteve", skullOwner["Name"].get<std::string>());
}

TEST_F(LootTest, FillPlayerHeadFunction_SourceKillerNonPlayer)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::Killer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建非玩家实体作为 KILLER_ENTITY
    Entity zombie(EntityInstanceId(4));
    context.set(LootParams::KILLER_ENTITY, &zombie);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // 非玩家实体不应写入 SkullOwner
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));
}

TEST_F(LootTest, FillPlayerHeadFunction_SourceBlockEntity)
{
    // BlockEntity 来源不支持玩家头颅填充
    FillPlayerHeadFunction func(CopyNameFunction::Source::BlockEntity);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建方块实体
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    BlockEntity* blockEntity = &chest; // 显式转换为 BlockEntity*
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);
    ItemStack result = func.apply(stack, context);

    // BlockEntity 不应写入 SkullOwner
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));
}

TEST_F(LootTest, FillPlayerHeadFunction_OverwritesExistingSkullOwner)
{
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家
    Player player(EntityInstanceId(1), "NewOwner");
    player.setUuid("00000000-0000-0000-0000-000000000001");
    context.set(LootParams::KILLER_PLAYER, &player);

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    ItemStack stack(*Items::PLAYER_HEAD, 1);

    // 设置已有的 SkullOwner
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["SkullOwner"] = "OldOwner";

    ItemStack result = func.apply(stack, context);

    // 应该覆盖旧的 SkullOwner
    EXPECT_TRUE(result.hasTag());
    const nlohmann::json* resultTag = result.getTag();
    ASSERT_NE(resultTag, nullptr);
    EXPECT_TRUE(resultTag->contains("SkullOwner"));

    // SkullOwner 应该是新玩家的信息（JSON 对象格式），不是旧的字符串格式
    const auto& skullOwner = (*resultTag)["SkullOwner"];
    EXPECT_TRUE(skullOwner.is_object());
    EXPECT_EQ("NewOwner", skullOwner["Name"].get<std::string>());
}

TEST_F(LootTest, FillPlayerHeadFunction_NonPlayerHeadItemUnchanged)
{
    // 非玩家头颅物品应被原样返回，不写入 SkullOwner
    // 参考 MC Java: FillPlayerHead.run() 中 stack.is(Items.PLAYER_HEAD) 检查
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家
    Player player(EntityInstanceId(1), "Steve");
    player.setUuid("550e8400-e29b-41d4-a716-446655440000");
    context.set(LootParams::KILLER_PLAYER, &player);

    // 使用钻石物品（非玩家头颅），函数应原样返回不修改
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 钻石物品不应写入 SkullOwner 标签
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));
}

TEST_F(LootTest, FillPlayerHeadFunction_OtherSkullItemsUnchanged)
{
    // 其他头颅物品（骷髅头颅、僵尸头等）也不应被 fill_player_head 修改，
    // 只有 PLAYER_HEAD 才应被修改
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    Player player(EntityInstanceId(1), "Steve");
    player.setUuid("550e8400-e29b-41d4-a716-446655440000");
    context.set(LootParams::KILLER_PLAYER, &player);

    // 骷髅头颅不应被修改
    ASSERT_NE(Items::SKELETON_SKULL, nullptr);
    ItemStack skeletonStack(*Items::SKELETON_SKULL, 1);
    ItemStack result = func.apply(skeletonStack, context);
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));

    // 凋灵骷髅头颅不应被修改
    ASSERT_NE(Items::WITHER_SKELETON_SKULL, nullptr);
    ItemStack witherStack(*Items::WITHER_SKELETON_SKULL, 1);
    result = func.apply(witherStack, context);
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));

    // 僵尸头不应被修改
    ASSERT_NE(Items::ZOMBIE_HEAD, nullptr);
    ItemStack zombieStack(*Items::ZOMBIE_HEAD, 1);
    result = func.apply(zombieStack, context);
    EXPECT_FALSE(result.hasTag() && result.getTag()->contains("SkullOwner"));
}

TEST_F(LootTest, SkullItems_Registration)
{
    // 验证所有 7 种头颅物品已正确注册
    ASSERT_NE(Items::SKELETON_SKULL, nullptr);
    EXPECT_EQ("minecraft:skeleton_skull", Items::SKELETON_SKULL->itemLocation().toString());

    ASSERT_NE(Items::WITHER_SKELETON_SKULL, nullptr);
    EXPECT_EQ("minecraft:wither_skeleton_skull", Items::WITHER_SKELETON_SKULL->itemLocation().toString());

    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    EXPECT_EQ("minecraft:player_head", Items::PLAYER_HEAD->itemLocation().toString());

    ASSERT_NE(Items::ZOMBIE_HEAD, nullptr);
    EXPECT_EQ("minecraft:zombie_head", Items::ZOMBIE_HEAD->itemLocation().toString());

    ASSERT_NE(Items::CREEPER_HEAD, nullptr);
    EXPECT_EQ("minecraft:creeper_head", Items::CREEPER_HEAD->itemLocation().toString());

    ASSERT_NE(Items::DRAGON_HEAD, nullptr);
    EXPECT_EQ("minecraft:dragon_head", Items::DRAGON_HEAD->itemLocation().toString());

    ASSERT_NE(Items::PIGLIN_HEAD, nullptr);
    EXPECT_EQ("minecraft:piglin_head", Items::PIGLIN_HEAD->itemLocation().toString());
}

TEST_F(LootTest, SkullItems_MaxStackSize)
{
    // 头颅物品最大堆叠数为 64（与 MC Java 一致）
    ASSERT_NE(Items::PLAYER_HEAD, nullptr);
    EXPECT_EQ(64, Items::PLAYER_HEAD->maxStackSize());

    ASSERT_NE(Items::SKELETON_SKULL, nullptr);
    EXPECT_EQ(64, Items::SKELETON_SKULL->maxStackSize());

    ASSERT_NE(Items::WITHER_SKELETON_SKULL, nullptr);
    EXPECT_EQ(64, Items::WITHER_SKELETON_SKULL->maxStackSize());
}

TEST_F(LootTest, SkullItems_RegistryLookup)
{
    // 验证头颅物品可通过 ItemRegistry 通过 ResourceLocation 查找
    auto* playerHead = ItemRegistry::instance().getItem(ResourceLocation("minecraft:player_head"));
    ASSERT_NE(playerHead, nullptr);
    EXPECT_EQ(playerHead, Items::PLAYER_HEAD);

    auto* skeletonSkull = ItemRegistry::instance().getItem(ResourceLocation("minecraft:skeleton_skull"));
    ASSERT_NE(skeletonSkull, nullptr);
    EXPECT_EQ(skeletonSkull, Items::SKELETON_SKULL);

    auto* witherSkull = ItemRegistry::instance().getItem(ResourceLocation("minecraft:wither_skeleton_skull"));
    ASSERT_NE(witherSkull, nullptr);
    EXPECT_EQ(witherSkull, Items::WITHER_SKELETON_SKULL);
}

TEST_F(LootTest, SetAttributesFunction_Creation)
{
    SetAttributesFunction func;
    EXPECT_EQ("set_attributes", func.getType());
    EXPECT_TRUE(func.getModifiers().empty());
}

TEST_F(LootTest, SetAttributesFunction_AddModifier)
{
    SetAttributesFunction func;
    SetAttributesFunction::Modifier mod("generic.attack_damage", // name
        "minecraft:generic.attack_damage",                       // attributeId
        math::RandomValueRange(5.0f),                            // amount (fixed)
        0,                                                       // operation (Addition)
        std::vector<std::string>{"mainhand"},                    // slots
        ""                                                       // uuid (auto-generated)
    );

    func.addModifier(mod);
    EXPECT_EQ(1, func.getModifiers().size());
    EXPECT_EQ("generic.attack_damage", func.getModifiers()[0].name);
    EXPECT_EQ("minecraft:generic.attack_damage", func.getModifiers()[0].attributeId);
    EXPECT_TRUE(func.getModifiers()[0].amount.isFixed());
    EXPECT_FLOAT_EQ(5.0f, func.getModifiers()[0].amount.getMin());
}

TEST_F(LootTest, SetAttributesFunction_Apply)
{
    // 测试 SetAttributesFunction::apply() 方法
    SetAttributesFunction func;
    SetAttributesFunction::Modifier mod("generic.attack_damage", // name
        "minecraft:generic.attack_damage",                       // attributeId
        math::RandomValueRange(5.0f, 10.0f),                     // amount (range)
        0,                                                       // operation (Addition)
        std::vector<std::string>{"mainhand", "offhand"},         // slots (multiple)
        "d4d5c2a0-6b1c-4e3d-8f2a-1b2c3d4e5f6g"                   // uuid
    );
    func.addModifier(mod);

    // 创建物品栈（使用钻石，任何物品都可以）
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond == nullptr) {
        // 如果物品未注册，使用空测试验证 apply 逻辑不会崩溃
        ItemStack emptyStack;
        math::Random rng(12345);
        LootContext context(m_world, rng);
        // 对空栈应用函数应该返回空栈
        ItemStack result = func.apply(emptyStack, context);
        EXPECT_TRUE(result.isEmpty());
        return;
    }
    ItemStack stack(*diamond, 1);

    // 创建掉落上下文
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 应用函数
    ItemStack result = func.apply(stack, context);
    EXPECT_FALSE(result.isEmpty());

    // 验证 AttributeModifiers 标签
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("AttributeModifiers"));
    const nlohmann::json& attrModifiers = (*tag)["AttributeModifiers"];
    ASSERT_TRUE(attrModifiers.is_array());
    EXPECT_EQ(1, attrModifiers.size());

    // 验证属性修饰符内容
    const auto& entry = attrModifiers[0];
    EXPECT_EQ("minecraft:generic.attack_damage", entry["AttributeName"]);
    EXPECT_EQ("generic.attack_damage", entry["Name"]);
    EXPECT_EQ(0, entry["Operation"]);
    EXPECT_EQ("d4d5c2a0-6b1c-4e3d-8f2a-1b2c3d4e5f6g", entry["UUID"]);
    // 金额应该在范围内
    double amount = entry["Amount"].get<double>();
    EXPECT_GE(amount, 5.0);
    EXPECT_LE(amount, 10.0);
    // 槽位应该是 mainhand 或 offhand 中的一个
    int slot = entry["Slot"].get<int>();
    EXPECT_TRUE(slot == static_cast<int>(EquipmentSlot::MainHand) || slot == static_cast<int>(EquipmentSlot::OffHand));
}

TEST_F(LootTest, SetAttributesFunction_MultipleModifiers)
{
    SetAttributesFunction func;

    // 添加攻击伤害修饰符
    func.addModifier(SetAttributesFunction::Modifier("generic.attack_damage",
        "minecraft:generic.attack_damage",
        math::RandomValueRange(4.0f),
        0,
        std::vector<std::string>{"mainhand"}));

    // 添加攻击速度修饰符
    func.addModifier(SetAttributesFunction::Modifier("generic.attack_speed",
        "minecraft:generic.attack_speed",
        math::RandomValueRange(-2.4f),
        0,
        std::vector<std::string>{"mainhand"}));

    EXPECT_EQ(2, func.getModifiers().size());

    // 创建物品栈
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond == nullptr) {
        GTEST_SKIP() << "Item not registered, skipping test";
    }
    ItemStack stack(*diamond, 1);

    // 应用函数
    math::Random rng(12345);
    LootContext context(m_world, rng);
    ItemStack result = func.apply(stack, context);

    // 验证两个修饰符都被添加
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("AttributeModifiers"));
    const nlohmann::json& attrModifiers = (*tag)["AttributeModifiers"];
    EXPECT_EQ(2, attrModifiers.size());
}

TEST_F(LootTest, SetAttributesFunction_EmptyStack)
{
    SetAttributesFunction func;
    func.addModifier(SetAttributesFunction::Modifier("generic.attack_damage",
        "minecraft:generic.attack_damage",
        math::RandomValueRange(5.0f),
        0,
        std::vector<std::string>{"mainhand"}));

    ItemStack emptyStack;
    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, SetAttributesFunction_NoModifiers)
{
    SetAttributesFunction func;

    const Item* sword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(sword, nullptr);
    ItemStack stack(*sword, 1);

    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack result = func.apply(stack, context);
    EXPECT_FALSE(result.isEmpty());
    // 没有修饰符时不应创建 AttributeModifiers 标签
    const nlohmann::json* attrModifiers = result.getChildTag("AttributeModifiers");
    EXPECT_EQ(attrModifiers, nullptr);
}

TEST_F(LootTest, SetAttributesFunction_AllOperations)
{
    // 测试所有操作类型
    SetAttributesFunction func;

    // Addition (0)
    func.addModifier(SetAttributesFunction::Modifier("addition_test",
        "minecraft:generic.attack_damage",
        math::RandomValueRange(10.0f),
        0, // Addition
        std::vector<std::string>{"mainhand"}));

    // MultiplyBase (1)
    func.addModifier(SetAttributesFunction::Modifier("multiply_base_test",
        "minecraft:generic.attack_damage",
        math::RandomValueRange(0.5f),
        1, // MultiplyBase
        std::vector<std::string>{"mainhand"}));

    // MultiplyTotal (2)
    func.addModifier(SetAttributesFunction::Modifier("multiply_total_test",
        "minecraft:generic.attack_damage",
        math::RandomValueRange(0.2f),
        2, // MultiplyTotal
        std::vector<std::string>{"mainhand"}));

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    if (diamond == nullptr) {
        GTEST_SKIP() << "Diamond item not registered, skipping test";
    }
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    LootContext context(m_world, rng);
    ItemStack result = func.apply(stack, context);

    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->contains("AttributeModifiers"));
    const nlohmann::json& attrModifiers = (*tag)["AttributeModifiers"];
    EXPECT_EQ(3, attrModifiers.size());

    // 验证操作类型
    EXPECT_EQ(0, attrModifiers[0]["Operation"].get<int>());
    EXPECT_EQ(1, attrModifiers[1]["Operation"].get<int>());
    EXPECT_EQ(2, attrModifiers[2]["Operation"].get<int>());
}

TEST_F(LootTest, SetContentsFunction_Creation)
{
    SetContentsFunction func;
    EXPECT_EQ("set_contents", func.getType());
    EXPECT_TRUE(func.getEntries().empty());
}

TEST_F(LootTest, SetContentsFunction_AddEntry)
{
    SetContentsFunction func;

    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f, 3.0f), 10, 0);
    func.addEntry(std::move(entry));

    EXPECT_EQ(1, func.getEntries().size());
}

TEST_F(LootTest, SetContentsFunction_Clone)
{
    SetContentsFunction func;
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:iron_ingot", RandomValueRange(2.0f, 5.0f), 2, 0));

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<SetContentsFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(2, clonedFunc->getEntries().size());
}

TEST_F(LootTest, SetContentsFunction_EmptyStack)
{
    SetContentsFunction func;
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack emptyStack;
    ItemStack result = func.apply(std::move(emptyStack), *context);

    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, SetContentsFunction_EmptyEntries)
{
    SetContentsFunction func; // No entries

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 应该返回原堆，没有 BlockEntityTag
    EXPECT_FALSE(result.isEmpty());
    EXPECT_FALSE(result.hasTag());
}

TEST_F(LootTest, SetContentsFunction_SingleItem)
{
    SetContentsFunction func;
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(5.0f), 1, 0));

    // 使用钻石作为容器物品（测试函数逻辑，任何物品都可以）
    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(item, nullptr);
    ItemStack stack(*item, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.hasTag());

    // 检查 BlockEntityTag.Items
    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);
    ASSERT_TRUE(blockEntityTag->contains("Items"));
    EXPECT_TRUE((*blockEntityTag)["Items"].is_array());

    // 应该有一个物品（5个钻石）
    const auto& items = (*blockEntityTag)["Items"];
    EXPECT_EQ(1, items.size());
    EXPECT_EQ("minecraft:diamond", items[0]["id"].get<std::string>());
    EXPECT_EQ(5, items[0]["Count"].get<int>());
}

TEST_F(LootTest, SetContentsFunction_MultipleItems)
{
    SetContentsFunction func;
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:iron_ingot", RandomValueRange(2.0f), 1, 0));
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:gold_ingot", RandomValueRange(3.0f), 1, 0));

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(item, nullptr);
    ItemStack stack(*item, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(result.hasTag());

    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);
    ASSERT_TRUE(blockEntityTag->contains("Items"));

    const auto& items = (*blockEntityTag)["Items"];
    EXPECT_EQ(3, items.size());

    // 验证所有物品都有 Slot 字段
    for (const auto& itemJson : items) {
        EXPECT_TRUE(itemJson.contains("Slot"));
        EXPECT_TRUE(itemJson.contains("id"));
        EXPECT_TRUE(itemJson.contains("Count"));
    }
}

TEST_F(LootTest, SetContentsFunction_MergesWithExistingTag)
{
    SetContentsFunction func;
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0));

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(item, nullptr);
    ItemStack stack(*item, 1);

    // 预设一个 BlockEntityTag（模拟已有数据）
    nlohmann::json& existingTag = stack.getOrCreateChildTag("BlockEntityTag");
    existingTag["CustomName"] = "Test Container";
    existingTag["Lock"] = "secret";

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    EXPECT_FALSE(result.isEmpty());

    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);

    // Items 应该被添加
    EXPECT_TRUE(blockEntityTag->contains("Items"));

    // 原有的数据应该被保留
    EXPECT_TRUE(blockEntityTag->contains("CustomName"));
    EXPECT_EQ("Test Container", (*blockEntityTag)["CustomName"].get<std::string>());
    EXPECT_TRUE(blockEntityTag->contains("Lock"));
    EXPECT_EQ("secret", (*blockEntityTag)["Lock"].get<std::string>());
}

TEST_F(LootTest, SetContentsFunction_StackSplitting)
{
    SetContentsFunction func;
    // 生成 128 个钻石，超过最大堆叠数 64
    func.addEntry(std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(128.0f), 1, 0));

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(item, nullptr);
    ItemStack stack(*item, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);

    const auto& items = (*blockEntityTag)["Items"];
    // 应该被拆分成两个堆：64 + 64
    EXPECT_EQ(2, items.size());
    EXPECT_EQ(64, items[0]["Count"].get<int>());
    EXPECT_EQ(64, items[1]["Count"].get<int>());
}

TEST_F(LootTest, SetContentsFunction_WithCondition)
{
    SetContentsFunction func;

    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);
    entry->addCondition(std::make_unique<RandomChanceCondition>(0.0f)); // 永远不满足
    func.addEntry(std::move(entry));

    const Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(item, nullptr);
    ItemStack stack(*item, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 条件不满足，不会生成物品
    EXPECT_FALSE(result.isEmpty());
    // 没有 BlockEntityTag 或者没有 Items
    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    if (blockEntityTag != nullptr) {
        EXPECT_FALSE(blockEntityTag->contains("Items") && (*blockEntityTag)["Items"].size() > 0);
    }
}

TEST_F(LootTest, SetLootTableFunction_Creation)
{
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);
    EXPECT_EQ("set_loot_table", func.getType());
    EXPECT_EQ("minecraft:chests/simple_dungeon", func.getLootTableId());
    EXPECT_EQ(12345, func.getSeed());
}

TEST_F(LootTest, SetLootTableFunction_Clone)
{
    SetLootTableFunction func("minecraft:chests/spawn_bonus_chest");

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<SetLootTableFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ("minecraft:chests/spawn_bonus_chest", clonedFunc->getLootTableId());
}

TEST_F(LootTest, ExplorationMapFunction_Creation)
{
    ExplorationMapFunction func(ExplorationMapFunction::Destination::Mansion, std::nullopt, 2, 50, true);
    EXPECT_EQ("exploration_map", func.getType());
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func.getDestination());
    EXPECT_EQ(2, func.getZoom());
    EXPECT_EQ(50, func.getSearchRadius());
    EXPECT_TRUE(func.shouldSkipKnownStructures());
    EXPECT_FALSE(func.getDecoration().has_value());
}

TEST_F(LootTest, ExplorationMapFunction_AllDestinations)
{
    ExplorationMapFunction func1(ExplorationMapFunction::Destination::BuriedTreasure, std::nullopt, 2, 50, true);
    ExplorationMapFunction func2(ExplorationMapFunction::Destination::Mansion, std::nullopt, 2, 50, true);
    ExplorationMapFunction func3(ExplorationMapFunction::Destination::Monument, std::nullopt, 2, 50, true);

    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func1.getDestination());
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func2.getDestination());
    EXPECT_EQ(ExplorationMapFunction::Destination::Monument, func3.getDestination());
}

TEST_F(LootTest, ExplorationMapFunction_CustomDecoration)
{
    // 测试自定义装饰类型覆盖目标默认装饰
    ExplorationMapFunction func(
        ExplorationMapFunction::Destination::Mansion, world::map::DecorationType::RED_X, 1, 100, false);
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func.getDestination());
    EXPECT_TRUE(func.getDecoration().has_value());
    EXPECT_EQ(world::map::DecorationType::RED_X, func.getDecoration().value());
    EXPECT_EQ(world::map::DecorationType::RED_X, func.getEffectiveDecoration());
    EXPECT_EQ(1, func.getZoom());
    EXPECT_EQ(100, func.getSearchRadius());
    EXPECT_FALSE(func.shouldSkipKnownStructures());
}

TEST_F(LootTest, ExplorationMapFunction_DefaultDecorationFromDestination)
{
    // 测试从 destination 推导默认装饰类型
    ExplorationMapFunction mansionFunc(ExplorationMapFunction::Destination::Mansion, std::nullopt, 2, 50, true);
    EXPECT_FALSE(mansionFunc.getDecoration().has_value());
    EXPECT_EQ(world::map::DecorationType::MANSION, mansionFunc.getEffectiveDecoration());

    ExplorationMapFunction monumentFunc(ExplorationMapFunction::Destination::Monument, std::nullopt, 2, 50, true);
    EXPECT_EQ(world::map::DecorationType::MONUMENT, monumentFunc.getEffectiveDecoration());

    ExplorationMapFunction treasureFunc(ExplorationMapFunction::Destination::BuriedTreasure, std::nullopt, 2, 50, true);
    EXPECT_EQ(world::map::DecorationType::RED_X, treasureFunc.getEffectiveDecoration());
}

TEST_F(LootTest, ExplorationMapFunction_DestinationStringConversion)
{
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure,
        ExplorationMapFunction::destinationFromString("minecraft:buried_treasure").value());
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion,
        ExplorationMapFunction::destinationFromString("minecraft:mansion").value());
    EXPECT_EQ(ExplorationMapFunction::Destination::Monument,
        ExplorationMapFunction::destinationFromString("minecraft:monument").value());
    EXPECT_EQ(ExplorationMapFunction::Destination::Shipwreck,
        ExplorationMapFunction::destinationFromString("minecraft:shipwreck").value());
    EXPECT_EQ(ExplorationMapFunction::Destination::RuinedPortal,
        ExplorationMapFunction::destinationFromString("minecraft:ruined_portal").value());
    // 不带命名空间前缀
    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure,
        ExplorationMapFunction::destinationFromString("buried_treasure").value());
    EXPECT_FALSE(ExplorationMapFunction::destinationFromString("unknown").has_value());
}

TEST_F(LootTest, ExplorationMapFunction_DestinationToResourceLocation)
{
    // 验证 Destination 到 ResourceLocation 的映射
    EXPECT_EQ(ResourceLocation("minecraft", "buried_treasure"),
        ExplorationMapFunction::destinationToResourceLocation(ExplorationMapFunction::Destination::BuriedTreasure));
    EXPECT_EQ(ResourceLocation("minecraft", "mansion"),
        ExplorationMapFunction::destinationToResourceLocation(ExplorationMapFunction::Destination::Mansion));
    EXPECT_EQ(ResourceLocation("minecraft", "monument"),
        ExplorationMapFunction::destinationToResourceLocation(ExplorationMapFunction::Destination::Monument));
    EXPECT_EQ(ResourceLocation("minecraft", "shipwreck"),
        ExplorationMapFunction::destinationToResourceLocation(ExplorationMapFunction::Destination::Shipwreck));
    EXPECT_EQ(ResourceLocation("minecraft", "ruined_portal"),
        ExplorationMapFunction::destinationToResourceLocation(ExplorationMapFunction::Destination::RuinedPortal));
}

TEST_F(LootTest, SetStewEffectFunction_Creation)
{
    SetStewEffectFunction func;
    EXPECT_EQ("set_stew_effect", func.getType());
    EXPECT_TRUE(func.getEffects().empty());
}

TEST_F(LootTest, SetStewEffectFunction_AddEffect)
{
    SetStewEffectFunction func;
    func.addEffect("minecraft:regeneration", RandomValueRange(5.0f, 10.0f));

    EXPECT_EQ(1, func.getEffects().size());
    EXPECT_EQ("minecraft:regeneration", func.getEffects()[0].effectId);
    EXPECT_FLOAT_EQ(5.0f, func.getEffects()[0].duration.getMin());
    EXPECT_FLOAT_EQ(10.0f, func.getEffects()[0].duration.getMax());
}

// ============================================================================
// LootFunctionBuilder Tests for New Functions
// ============================================================================

TEST_F(LootTest, LootFunctionBuilder_NewFunctions)
{
    // 测试所有新增的工厂方法
    auto copyName = LootFunctionBuilder::copyName(CopyNameFunction::Source::KillerPlayer);
    ASSERT_NE(copyName, nullptr);
    EXPECT_EQ("copy_name", copyName->getType());

    auto copyBlockState = LootFunctionBuilder::copyBlockState("minecraft:chest");
    ASSERT_NE(copyBlockState, nullptr);
    EXPECT_EQ("copy_block_state", copyBlockState->getType());

    auto copyNbt = LootFunctionBuilder::copyNbt(CopyNbtFunction::Source::This);
    ASSERT_NE(copyNbt, nullptr);
    EXPECT_EQ("copy_nbt", copyNbt->getType());

    auto fillHead = LootFunctionBuilder::fillPlayerHead();
    ASSERT_NE(fillHead, nullptr);
    EXPECT_EQ("fill_player_head", fillHead->getType());

    auto setAttr = LootFunctionBuilder::setAttributes();
    ASSERT_NE(setAttr, nullptr);
    EXPECT_EQ("set_attributes", setAttr->getType());

    auto setContents = LootFunctionBuilder::setContents();
    ASSERT_NE(setContents, nullptr);
    EXPECT_EQ("set_contents", setContents->getType());

    auto setLootTable = LootFunctionBuilder::setLootTable("minecraft:chests/test");
    ASSERT_NE(setLootTable, nullptr);
    EXPECT_EQ("set_loot_table", setLootTable->getType());

    auto exploreMap = LootFunctionBuilder::explorationMap();
    ASSERT_NE(exploreMap, nullptr);
    EXPECT_EQ("exploration_map", exploreMap->getType());

    auto setStew = LootFunctionBuilder::setStewEffect();
    ASSERT_NE(setStew, nullptr);
    EXPECT_EQ("set_stew_effect", setStew->getType());
}

// ============================================================================
// FurnaceSmeltFunction Tests
// ============================================================================

TEST_F(LootTest, FurnaceSmeltFunction_EmptyStack)
{
    // 空物品栈应返回空栈
    FurnaceSmeltFunction func;
    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, FurnaceSmeltFunction_Type)
{
    FurnaceSmeltFunction func;
    EXPECT_EQ("furnace_smelt", func.getType());
}

TEST_F(LootTest, FurnaceSmeltFunction_Clone)
{
    FurnaceSmeltFunction func;
    func.addCondition(std::make_unique<RandomChanceCondition>(0.5f));

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ("furnace_smelt", cloned->getType());
    EXPECT_EQ(1, cloned->getConditions().size());
}

TEST_F(LootTest, FurnaceSmeltFunction_NoRecipe)
{
    // 没有对应熔炼配方的物品应返回原物品
    FurnaceSmeltFunction func;
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建一个物品（钻石没有熔炼配方）
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    ItemStack stack(*diamond, 5);
    ItemStack result = func.apply(stack, context);

    // 没有熔炼配方时返回原物品
    EXPECT_EQ(stack.getItem(), result.getItem());
    EXPECT_EQ(5, result.getCount());
}

TEST_F(LootTest, FurnaceSmeltFunction_Builder)
{
    auto func = LootFunctionBuilder::furnaceSmelt();
    ASSERT_NE(func, nullptr);
    EXPECT_EQ("furnace_smelt", func->getType());
}

// ============================================================================
// CopyNameFunction::apply() 测试
// ============================================================================

TEST_F(LootTest, CopyNameFunction_EmptyStack)
{
    // 空 ItemStack 应该直接返回
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, CopyNameFunction_NoEntityInContext)
{
    // 没有 THIS_ENTITY 参数时不应崩溃
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 没有实体，名称不应改变
    EXPECT_FALSE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_EntityWithoutCustomName)
{
    // 实体没有自定义名称时，不应复制名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建一个没有自定义名称的实体
    Entity entity(EntityInstanceId(1), nullptr);
    context.set(LootParams::THIS_ENTITY, &entity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 实体没有自定义名称，物品不应获得名称
    EXPECT_FALSE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_EntityWithCustomName)
{
    // 从有自定义名称的实体复制名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建一个有自定义名称的实体
    Entity entity(EntityInstanceId(1), nullptr);
    entity.setCustomName("Custom Pig Name");
    context.set(LootParams::THIS_ENTITY, &entity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 应该复制名称
    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("Custom Pig Name", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_KillerEntity)
{
    // 从 KILLER_ENTITY 复制名称
    CopyNameFunction func(CopyNameFunction::Source::Killer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建击杀者实体
    Entity killer(EntityInstanceId(2), nullptr);
    killer.setCustomName("Killer Zombie");
    context.set(LootParams::KILLER_ENTITY, &killer);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("Killer Zombie", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_KillerPlayer)
{
    // 从 KILLER_PLAYER 复制名称（玩家总是有名称）
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家
    Player player(EntityInstanceId(3), "TestPlayer");
    context.set(LootParams::KILLER_PLAYER, &player);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 玩家即使没有自定义名称也应该复制显示名称
    EXPECT_TRUE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_KillerPlayerWithCustomName)
{
    // 从有自定义名称的玩家复制
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建有自定义名称的玩家
    Player player(EntityInstanceId(4), "OriginalName");
    player.setCustomName("CustomPlayerName");
    context.set(LootParams::KILLER_PLAYER, &player);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("CustomPlayerName", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_BlockEntityWithoutCustomName)
{
    // 方块实体没有自定义名称时不应复制
    CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建箱子（没有自定义名称）
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    BlockEntity* blockEntity = &chest; // 显式转换为基类指针
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 没有自定义名称，物品不应获得名称
    EXPECT_FALSE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_BlockEntityWithCustomName)
{
    // 从有自定义名称的方块实体复制
    CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建有自定义名称的箱子
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    chest.setCustomName("My Special Chest");
    BlockEntity* blockEntity = &chest; // 显式转换为基类指针
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("My Special Chest", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_DifferentSourcesIndependent)
{
    // 不同来源应该独立工作
    math::Random rng(12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 创建多个实体
    Entity thisEntity(EntityInstanceId(1), nullptr);
    thisEntity.setCustomName("This Pig");

    Entity killerEntity(EntityInstanceId(2), nullptr);
    killerEntity.setCustomName("Killer Zombie");

    Player player(EntityInstanceId(3), "PlayerName");
    player.setCustomName("Custom Player");

    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    chest.setCustomName("Named Chest");

    // 测试 THIS_ENTITY 来源
    {
        LootContext context(m_world, rng);
        context.set(LootParams::THIS_ENTITY, &thisEntity);
        context.set(LootParams::KILLER_ENTITY, &killerEntity); // 设置另一个来源，确保不影响

        CopyNameFunction func(CopyNameFunction::Source::This);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("This Pig", result.getCustomName());
    }

    // 测试 KILLER_ENTITY 来源
    {
        LootContext context(m_world, rng);
        context.set(LootParams::KILLER_ENTITY, &killerEntity);
        context.set(LootParams::THIS_ENTITY, &thisEntity); // 设置另一个来源，确保不影响

        CopyNameFunction func(CopyNameFunction::Source::Killer);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("Killer Zombie", result.getCustomName());
    }

    // 测试 KILLER_PLAYER 来源
    {
        LootContext context(m_world, rng);
        context.set(LootParams::KILLER_PLAYER, &player);

        CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("Custom Player", result.getCustomName());
    }

    // 测试 BLOCK_ENTITY 来源
    {
        LootContext context(m_world, rng);
        BlockEntity* blockEntity = &chest; // 显式转换为基类指针
        context.set(LootParams::BLOCK_ENTITY, blockEntity);

        CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("Named Chest", result.getCustomName());
    }
}

TEST_F(LootTest, CopyNameFunction_OverwritesExistingName)
{
    // 应该覆盖物品已有的自定义名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建实体
    Entity entity(EntityInstanceId(1), nullptr);
    entity.setCustomName("New Name");
    context.set(LootParams::THIS_ENTITY, &entity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 创建有旧名称的物品
    ItemStack stack(*diamond, 1);
    stack.setCustomName("Old Name");
    EXPECT_EQ("Old Name", stack.getCustomName());

    // 应用函数后应该被覆盖
    ItemStack result = func.apply(stack, context);
    EXPECT_EQ("New Name", result.getCustomName());
}

// ============================================================================
// LootEntry Function List Tests
// ============================================================================

TEST_F(LootTest, LootEntry_AddFunction)
{
    // 测试 LootEntry 添加函数
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 添加函数
    entry.addFunction(std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops));
    entry.addFunction(std::make_unique<SetCountFunction>(RandomValueRange(2.0f, 4.0f)));

    // 验证函数数量
    EXPECT_EQ(2, entry.getFunctions().size());
}

TEST_F(LootTest, LootEntry_ApplyFunctionsCorrectly)
{
    // 测试 LootEntry::applyFunctions 正确应用函数
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 添加设置数量函数
    entry.addFunction(std::make_unique<SetCountFunction>(RandomValueRange(5.0f, 5.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 应用函数
    ItemStack result = entry.applyFunctions(stack, *context);

    // 数量应该被设置为 5
    EXPECT_EQ(5, result.getCount());
}

TEST_F(LootTest, LootEntry_ApplyFunctionsInOrder)
{
    // 测试多个函数按顺序应用
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 第一个函数：设置数量为 2
    entry.addFunction(std::make_unique<SetCountFunction>(RandomValueRange(2.0f, 2.0f)));
    // 第二个函数：限制数量最大为 5（当原数量 > max 时会截断）
    // LimitCountFunction 会将数量限制在 [min, max] 范围内
    entry.addFunction(std::make_unique<LimitCountFunction>(-1, 5)); // 无下限，最大5

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    ItemStack result = entry.applyFunctions(stack, *context);

    // SetCount 设置数量为 2，LimitCount 不会改变它（因为 2 < 5）
    EXPECT_EQ(2, result.getCount());
}

TEST_F(LootTest, LootEntry_ApplyFunctionsWithCondition)
{
    // 测试带条件的函数
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 创建带条件的函数（条件永远不满足）
    auto func = std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f));
    func->addCondition(std::make_unique<RandomChanceCondition>(0.0f)); // 永远不触发
    entry.addFunction(std::move(func));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    ItemStack result = entry.applyFunctions(stack, *context);

    // 条件不满足，数量应该保持不变
    EXPECT_EQ(1, result.getCount());
}

TEST_F(LootTest, LootEntry_CloneCopiesFunctions)
{
    // 测试 clone 正确复制函数
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);
    entry.addFunction(std::make_unique<SetCountFunction>(RandomValueRange(5.0f, 5.0f)));

    auto cloned = entry.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedEntry = dynamic_cast<ItemLootEntry*>(cloned.get());
    ASSERT_NE(clonedEntry, nullptr);

    // 验证克隆的条目有相同数量的函数
    EXPECT_EQ(1, clonedEntry->getFunctions().size());

    // 验证函数类型正确
    EXPECT_EQ("set_count", clonedEntry->getFunctions()[0]->getType());
}

// ============================================================================
// ItemLootEntry::generate with Functions Tests
// ============================================================================

TEST_F(LootTest, ItemLootEntry_GenerateAppliesFunctions)
{
    // 测试 ItemLootEntry::generate 在条件检查后应用函数
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success =
        entry->generate([&generatedItems](const ItemStack& stack) { generatedItems.push_back(stack); }, *context);

    EXPECT_TRUE(success);
    ASSERT_EQ(1, generatedItems.size());
    // 函数应该将数量设置为 10
    EXPECT_EQ(10, generatedItems[0].getCount());
}

TEST_F(LootTest, ItemLootEntry_GenerateWithConditionAndFunction)
{
    // 测试带条件的条目和函数
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 条件：50% 概率
    entry->addCondition(std::make_unique<RandomChanceCondition>(1.0f)); // 总是触发

    // 函数：设置数量
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(7.0f, 7.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success =
        entry->generate([&generatedItems](const ItemStack& stack) { generatedItems.push_back(stack); }, *context);

    EXPECT_TRUE(success);
    ASSERT_EQ(1, generatedItems.size());
    EXPECT_EQ(7, generatedItems[0].getCount());
}

TEST_F(LootTest, ItemLootEntry_GenerateConditionFails)
{
    // 测试条件不满足时不生成物品
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 条件：永远不满足
    entry->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    // 函数：设置数量（不应该被应用）
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success =
        entry->generate([&generatedItems](const ItemStack& stack) { generatedItems.push_back(stack); }, *context);

    EXPECT_FALSE(success);
    EXPECT_TRUE(generatedItems.empty());
}

TEST_F(LootTest, ItemLootEntry_GenerateFunctionReturnsEmpty)
{
    // 测试函数返回空堆时不生成物品
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 函数：设置数量为 0（空堆）
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(0.0f, 0.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success =
        entry->generate([&generatedItems](const ItemStack& stack) { generatedItems.push_back(stack); }, *context);

    // 条件满足，但函数返回空堆
    EXPECT_TRUE(success);
    EXPECT_TRUE(generatedItems.empty());
}

// ============================================================================
// LootEntryBuilder::function Tests
// ============================================================================

TEST_F(LootTest, LootEntryBuilder_FunctionChainCall)
{
    // 测试 LootEntryBuilder::function 链式调用
    auto entry = LootEntryBuilder::item("minecraft:diamond")
                     .weight(5)
                     .quality(2)
                     .count(1, 3)
                     .function(std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f)))
                     .function(std::make_unique<FurnaceSmeltFunction>())
                     .build();

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(LootEntryType::Item, entry->getType());
    EXPECT_EQ(5, entry->getWeight());
    EXPECT_EQ(2, entry->getQuality());
    EXPECT_EQ(2, entry->getFunctions().size());
}

TEST_F(LootTest, LootEntryBuilder_BuildCopiesFunctions)
{
    // 测试 build 正确复制函数
    // 直接链式调用，避免复制 builder
    auto entry = LootEntryBuilder::item("minecraft:diamond")
                     .function(std::make_unique<SetCountFunction>(RandomValueRange(5.0f, 5.0f)))
                     .function(std::make_unique<FurnaceSmeltFunction>())
                     .build();

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(2, entry->getFunctions().size());

    // 验证函数类型
    EXPECT_EQ("set_count", entry->getFunctions()[0]->getType());
    EXPECT_EQ("furnace_smelt", entry->getFunctions()[1]->getType());
}

TEST_F(LootTest, LootEntryBuilder_WithConditionAndFunction)
{
    // 测试同时添加条件和函数
    auto entry = LootEntryBuilder::item("minecraft:diamond")
                     .condition(std::make_unique<RandomChanceCondition>(0.5f))
                     .function(std::make_unique<SetCountFunction>(RandomValueRange(3.0f, 3.0f)))
                     .build();

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(1, entry->getConditions().size());
    EXPECT_EQ(1, entry->getFunctions().size());
}

// ============================================================================
// ApplyBonusFunction Tests
// ============================================================================

TEST_F(LootTest, ApplyBonusFunction_OreDropsNoFortune)
{
    // 测试没有时运时的 OreDrops 公式
    math::Random rng(12345);

    // 没有时运，应该返回基础数量
    for (int i = 0; i < 10; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(1, 0, rng);
        EXPECT_EQ(1, result);
    }
}

TEST_F(LootTest, ApplyBonusFunction_OreDropsWithFortune)
{
    // 测试有时运时的 OreDrops 公式
    math::Random rng(12345);

    // Fortune I: random.nextInt(3) - 1 -> -1, 0, 1 (修正后 0, 0, 1) -> multiplier: 1, 1, 2
    // 结果范围: 1 * (0+1) = 1 到 1 * (1+1) = 2
    bool sawOne = false;
    bool sawTwo = false;
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(1, 1, rng);
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 2);
        if (result == 1) sawOne = true;
        if (result == 2) sawTwo = true;
    }
    EXPECT_TRUE(sawOne);
    EXPECT_TRUE(sawTwo);

    // Fortune III: random.nextInt(5) - 1 -> -1, 0, 1, 2, 3 (修正后 0, 0, 1, 2, 3) -> multiplier: 1, 1, 2, 3, 4
    // 结果范围: 1 * (0+1) = 1 到 1 * (3+1) = 4
    sawOne = false;
    bool sawFour = false;
    for (int i = 0; i < 200; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(1, 3, rng);
        EXPECT_GE(result, 1);
        EXPECT_LE(result, 4);
        if (result == 1) sawOne = true;
        if (result == 4) sawFour = true;
    }
    EXPECT_TRUE(sawOne);
    EXPECT_TRUE(sawFour);
}

TEST_F(LootTest, ApplyBonusFunction_OreDropsMultiplicative)
{
    // 验证 OreDrops 是乘法式，不是加法式
    math::Random rng(12345);

    // 基础数量 2，Fortune III，最大应该是 2 * 4 = 8
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(2, 3, rng);
        EXPECT_GE(result, 2); // 最小 2 * 1 = 2
        EXPECT_LE(result, 8); // 最大 2 * 4 = 8
    }
}

TEST_F(LootTest, ApplyBonusFunction_UniformBonus)
{
    // 测试均匀分布加成
    math::Random rng(12345);

    // Uniform: count + random(0, bonusMultiplier * fortune)
    // bonusMultiplier=1, fortune=3 -> 加成范围 [0, 3]
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateUniformBonus(5, 3, 1, rng);
        EXPECT_GE(result, 5); // 5 + 0
        EXPECT_LE(result, 8); // 5 + 3
    }
}

TEST_F(LootTest, ApplyBonusFunction_BinomialBonus)
{
    // 测试二项分布加成
    math::Random rng(12345);

    // Binomial: count + binomial(fortune + extra, probability)
    // fortune=3, extra=1, probability=0.5 -> 4 次试验，每次 50% 概率
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateBinomialBonus(1, 3, 1, 0.5f, rng);
        EXPECT_GE(result, 1); // 1 + 0
        EXPECT_LE(result, 5); // 1 + 4
    }
}

TEST_F(LootTest, ApplyBonusFunction_IntegrationWithLootContext)
{
    // 测试 ApplyBonusFunction 与 LootContext 的时运参数集成
    auto func = std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
                       .withRandom(rng)
                       .withLootingModifier(3) // Fortune III
                       .build();

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 注意：当前 ApplyBonusFunction 使用 getLootingModifier() 作为时运等级
    // 这是一个设计选择，因为 MC 中时运和掠夺都使用 looting modifier 参数
    ItemStack result = func->apply(stack, *context);

    // 应该有时运加成
    EXPECT_GE(result.getCount(), 1);
    EXPECT_LE(result.getCount(), 4);
}

// ============================================================================
// LootTable Integration Tests with Fortune
// ============================================================================

TEST_F(LootTest, LootTable_DiamondOreWithSilkTouch)
{
    // 测试钻石矿精准采集掉落
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/diamond_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::DIAMOND_ORE, nullptr);

    // 设置精准采集
    auto context = buildBlockLootContext(m_world, rng, VanillaBlocks::DIAMOND_ORE->getDefaultState());
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    // 精准采集应该掉落钻石矿石
    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:diamond_ore", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_DiamondOreWithFortune)
{
    // 测试钻石矿时运加成
    // 注意：ApplyBonusFunction 需要 TOOL 参数才能应用时运加成
    // 这个测试验证在没有工具的情况下，掉落数量固定为 1
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/diamond_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::DIAMOND_ORE, nullptr);

    // 没有设置 TOOL，所以 ApplyBonusFunction 不会应用时运加成
    auto context = buildBlockLootContext(m_world, rng, VanillaBlocks::DIAMOND_ORE->getDefaultState());
    context->setLootingModifier(3);

    // 多次生成验证在没有工具时掉落数量固定为 1
    for (int i = 0; i < 10; ++i) {
        auto items = table->generate(*context);
        ASSERT_EQ(1, items.size());
        EXPECT_EQ("minecraft:diamond", items[0].getItem()->toString());
        // 没有工具时，数量固定为 1
        EXPECT_EQ(1, items[0].getCount());
    }
}

TEST_F(LootTest, LootTable_CoalOreWithSilkTouch)
{
    // 测试煤矿精准采集掉落
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/coal_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::COAL_ORE, nullptr);

    auto context = buildBlockLootContext(m_world, rng, VanillaBlocks::COAL_ORE->getDefaultState());
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:coal_ore", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_CoalOreWithFortune)
{
    // 测试煤矿时运加成
    // 注意：ApplyBonusFunction 需要 TOOL 参数才能应用时运加成
    // 这个测试验证在没有工具的情况下，掉落数量固定为 1
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/coal_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::COAL_ORE, nullptr);

    // 没有设置 TOOL，所以 ApplyBonusFunction 不会应用时运加成
    auto context = buildBlockLootContext(m_world, rng, VanillaBlocks::COAL_ORE->getDefaultState());
    context->setLootingModifier(3);

    // 多次生成验证在没有工具时掉落数量固定为 1
    for (int i = 0; i < 10; ++i) {
        auto items = table->generate(*context);
        ASSERT_EQ(1, items.size());
        EXPECT_EQ("minecraft:coal", items[0].getItem()->toString());
        // 没有工具时，数量固定为 1
        EXPECT_EQ(1, items[0].getCount());
    }
}

TEST_F(LootTest, LootTable_StoneWithSilkTouch)
{
    // 测试石头精准采集掉落石头
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/stone");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);
    VanillaBlocks::initialize();
    ASSERT_NE(VanillaBlocks::STONE, nullptr);

    auto context = buildBlockLootContext(m_world, rng, VanillaBlocks::STONE->getDefaultState());
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:stone", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_StoneWithoutSilkTouch)
{
    // 测试石头普通挖掘掉落圆石
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* table = manager.getTable("minecraft:blocks/stone");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:cobblestone", items[0].getItem()->toString());
}

// ============================================================================
// CopyBlockStateFunction Apply Tests
// ============================================================================

TEST_F(LootTest, CopyBlockStateFunction_EmptyStack)
{
    // 空物品堆不应该崩溃
    CopyBlockStateFunction func("minecraft:chest");

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, *context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, CopyBlockStateFunction_NoBlockStateInContext)
{
    // 没有 BlockState 参数时应该返回原物品
    CopyBlockStateFunction func("minecraft:chest");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();
    // 不设置 BLOCK_STATE 参数

    ItemStack result = func.apply(stack, *context);
    EXPECT_EQ(stack.getItem(), result.getItem());
    EXPECT_EQ(stack.getCount(), result.getCount());
    // 不应该有 BlockStateTag
    EXPECT_FALSE(result.hasTag());
}

TEST_F(LootTest, CopyBlockStateFunction_BlockIdMismatch)
{
    // 方块 ID 不匹配时不应复制
    CopyBlockStateFunction func("minecraft:chest");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    // 创建一个假的 BlockState（这里用空指针模拟不匹配情况）
    // 由于实际需要真正的 BlockState，这个测试验证函数不会崩溃
    ItemStack result = func.apply(stack, *context);
    EXPECT_EQ(stack.getItem(), result.getItem());
}

TEST_F(LootTest, CopyBlockStateFunction_EmptyPropertiesList)
{
    // 空属性列表（应该复制所有属性）
    // 这个测试验证函数能正常处理空属性列表
    CopyBlockStateFunction func("minecraft:furnace", {}); // 空属性列表

    EXPECT_TRUE(func.getProperties().empty());
    EXPECT_EQ("minecraft:furnace", func.getBlockId());
}

TEST_F(LootTest, CopyBlockStateFunction_SpecifiedProperties)
{
    // 指定属性列表
    std::vector<std::string> props = {"facing", "lit"};
    CopyBlockStateFunction func("minecraft:furnace", props);

    EXPECT_EQ(2, func.getProperties().size());
    EXPECT_EQ("facing", func.getProperties()[0]);
    EXPECT_EQ("lit", func.getProperties()[1]);
}

// ============================================================================
// SetLootTableFunction Apply Tests
// ============================================================================

TEST_F(LootTest, SetLootTableFunction_EmptyStack)
{
    // 空物品堆不应该崩溃
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, *context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, SetLootTableFunction_EmptyLootTableId)
{
    // 空掉落表 ID 应该返回原物品
    SetLootTableFunction func("", 12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);
    EXPECT_FALSE(result.hasTag()); // 不应该有标签
}

TEST_F(LootTest, SetLootTableFunction_BasicApply)
{
    // 基本功能测试：设置掉落表 ID
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证物品仍然存在
    EXPECT_EQ(diamond, result.getItem());
    EXPECT_EQ(1, result.getCount());

    // 验证设置了 BlockEntityTag
    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);

    // 验证掉落表 ID
    auto lootTableIt = blockEntityTag->find("LootTable");
    ASSERT_NE(lootTableIt, blockEntityTag->end());
    EXPECT_EQ("minecraft:chests/simple_dungeon", lootTableIt->get<std::string>());

    // 验证种子
    auto seedIt = blockEntityTag->find("LootTableSeed");
    ASSERT_NE(seedIt, blockEntityTag->end());
    EXPECT_EQ(12345, seedIt->get<i64>());
}

TEST_F(LootTest, SetLootTableFunction_ZeroSeedNotStored)
{
    // 种子为 0 时不应该存储
    SetLootTableFunction func("minecraft:chests/spawn_bonus_chest", 0);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证设置了 BlockEntityTag
    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* blockEntityTag = result.getChildTag("BlockEntityTag");
    ASSERT_NE(blockEntityTag, nullptr);

    // 验证掉落表 ID 存在
    auto lootTableIt = blockEntityTag->find("LootTable");
    ASSERT_NE(lootTableIt, blockEntityTag->end());
    EXPECT_EQ("minecraft:chests/spawn_bonus_chest", lootTableIt->get<std::string>());

    // 种子为 0 时不应该存储
    auto seedIt = blockEntityTag->find("LootTableSeed");
    EXPECT_EQ(seedIt, blockEntityTag->end());
}

TEST_F(LootTest, SetLootTableFunction_OverwriteExistingTag)
{
    // 测试覆盖现有的 BlockEntityTag
    SetLootTableFunction func1("minecraft:chests/first", 100);
    SetLootTableFunction func2("minecraft:chests/second", 200);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    // 第一次设置
    ItemStack result1 = func1.apply(stack, *context);
    const nlohmann::json* tag1 = result1.getChildTag("BlockEntityTag");
    ASSERT_NE(tag1, nullptr);
    EXPECT_EQ("minecraft:chests/first", (*tag1)["LootTable"].get<std::string>());

    // 第二次设置应该覆盖
    ItemStack result2 = func2.apply(result1, *context);
    const nlohmann::json* tag2 = result2.getChildTag("BlockEntityTag");
    ASSERT_NE(tag2, nullptr);
    EXPECT_EQ("minecraft:chests/second", (*tag2)["LootTable"].get<std::string>());
    EXPECT_EQ(200, (*tag2)["LootTableSeed"].get<i64>());
}

// ============================================================================
// SetNbtFunction Tests
// ============================================================================

TEST_F(LootTest, SetNbtFunction_EmptyString)
{
    // 空字符串不应该修改物品
    SetNbtFunction func("");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);
    EXPECT_FALSE(result.hasTag()); // 不应该有标签
}

TEST_F(LootTest, SetNbtFunction_EmptyStack)
{
    // 空物品堆不应该被修改
    SetNbtFunction func("{display:{Name:\"Test\"}}");

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, *context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, SetNbtFunction_Builder)
{
    auto func = LootFunctionBuilder::setNbt("{display:{Name:\"Custom Item\"}}");
    ASSERT_NE(func, nullptr);
    EXPECT_EQ("set_nbt", func->getType());

    auto* setNbtFunc = dynamic_cast<SetNbtFunction*>(func.get());
    ASSERT_NE(setNbtFunc, nullptr);
    EXPECT_EQ("{display:{Name:\"Custom Item\"}}", setNbtFunc->getNbtString());
}

TEST_F(LootTest, SetNbtFunction_SimpleTag)
{
    // 测试简单的 NBT 标签
    SetNbtFunction func("{Damage:10}");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证物品仍然存在
    EXPECT_EQ(diamond, result.getItem());
    EXPECT_EQ(1, result.getCount());

    // 验证设置了标签
    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);
    ASSERT_TRUE(tag->is_object());

    // 验证 Damage 值
    auto damageIt = tag->find("Damage");
    ASSERT_NE(damageIt, tag->end());
    EXPECT_EQ(10, damageIt->get<i32>());
}

TEST_F(LootTest, SetNbtFunction_NestedTag)
{
    // 测试嵌套的 NBT 标签
    SetNbtFunction func("{display:{Name:\"Custom Sword\",color:16711680}}");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证设置了嵌套标签
    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);

    // 验证 display 子标签
    auto displayIt = tag->find("display");
    ASSERT_NE(displayIt, tag->end());
    ASSERT_TRUE(displayIt->is_object());

    // 验证 Name 和 color
    EXPECT_EQ("Custom Sword", (*displayIt)["Name"].get<std::string>());
    EXPECT_EQ(16711680, (*displayIt)["color"].get<i32>());
}

TEST_F(LootTest, SetNbtFunction_MergeTag)
{
    // 测试合并到现有标签
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 首先设置一个初始标签
    stack.getOrCreateTag()["initial_value"] = 100;

    // 然后应用 SetNbtFunction
    SetNbtFunction func("{new_value:200}");

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证两个标签都存在
    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);

    // 初始值应该保留
    auto initialIt = tag->find("initial_value");
    ASSERT_NE(initialIt, tag->end());
    EXPECT_EQ(100, initialIt->get<i32>());

    // 新值应该添加
    auto newIt = tag->find("new_value");
    ASSERT_NE(newIt, tag->end());
    EXPECT_EQ(200, newIt->get<i32>());
}

TEST_F(LootTest, SetNbtFunction_MergeNestedObject)
{
    // 测试合并嵌套对象（递归合并）
    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    // 设置初始嵌套对象
    nlohmann::json& display = stack.getOrCreateChildTag("display");
    display["Name"] = "Original Name";
    display["existing_value"] = 50;

    // 应用函数合并嵌套对象
    SetNbtFunction func("{display:{color:16711680,new_value:100}}");

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    // 验证合并结果
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);

    auto displayIt = tag->find("display");
    ASSERT_NE(displayIt, tag->end());

    // 原有的 Name 应该保留
    auto nameIt = displayIt->find("Name");
    ASSERT_NE(nameIt, displayIt->end());
    EXPECT_EQ("Original Name", nameIt->get<std::string>());

    // 原有的 existing_value 应该保留
    auto existingIt = displayIt->find("existing_value");
    ASSERT_NE(existingIt, displayIt->end());
    EXPECT_EQ(50, existingIt->get<i32>());

    // 新的 color 应该添加
    auto colorIt = displayIt->find("color");
    ASSERT_NE(colorIt, displayIt->end());
    EXPECT_EQ(16711680, colorIt->get<i32>());

    // 新的 new_value 应该添加
    auto newIt = displayIt->find("new_value");
    ASSERT_NE(newIt, displayIt->end());
    EXPECT_EQ(100, newIt->get<i32>());
}

TEST_F(LootTest, SetNbtFunction_InvalidNbt)
{
    // 测试无效的 NBT 字符串
    SetNbtFunction func("{invalid nbt string"); // 缺少闭合大括号

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    // 无效的 NBT 应该被忽略，物品保持不变
    ItemStack result = func.apply(stack, *context);
    EXPECT_FALSE(result.hasTag());
}

TEST_F(LootTest, SetNbtFunction_WithTypeSuffixes)
{
    // 测试带类型后缀的 NBT 值
    SetNbtFunction func(
        "{byte_val:10b,short_val:100s,int_val:1000,long_val:10000l,float_val:3.14f,double_val:3.14159d}");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    ItemStack result = func.apply(stack, *context);

    ASSERT_TRUE(result.hasTag());
    const nlohmann::json* tag = result.getTag();
    ASSERT_NE(tag, nullptr);

    // 验证各种类型的值都被正确解析
    EXPECT_EQ(10, (*tag)["byte_val"].get<i32>());
    EXPECT_EQ(100, (*tag)["short_val"].get<i32>());
    EXPECT_EQ(1000, (*tag)["int_val"].get<i32>());
    EXPECT_EQ(10000, (*tag)["long_val"].get<i64>());
    EXPECT_FLOAT_EQ(3.14f, (*tag)["float_val"].get<f32>());
    EXPECT_DOUBLE_EQ(3.14159, (*tag)["double_val"].get<f64>());
}

TEST_F(LootTest, SetNbtFunction_Clone)
{
    SetNbtFunction func("{display:{Name:\"Test\"}}");

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<SetNbtFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ("{display:{Name:\"Test\"}}", clonedFunc->getNbtString());
    EXPECT_EQ("set_nbt", clonedFunc->getType());
}

// ============================================================================
// 钓鱼掉落表测试
// ============================================================================

TEST_F(LootTest, FishingLootTable_FishTableExists)
{
    // 测试鱼表存在
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* fishTable = manager.getTable("minecraft:gameplay/fishing/fish");
    ASSERT_NE(fishTable, nullptr);

    // 鱼表应该有 4 种鱼
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();
    auto items = fishTable->generate(*context);

    ASSERT_EQ(1, items.size());
    // 应该是四种鱼之一
    const std::string itemId = items[0].getItem()->toString();
    EXPECT_TRUE(itemId == "minecraft:cod" || itemId == "minecraft:salmon" || itemId == "minecraft:tropical_fish" ||
        itemId == "minecraft:pufferfish");
}

TEST_F(LootTest, FishingLootTable_JunkTableExists)
{
    // 测试垃圾表存在
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* junkTable = manager.getTable("minecraft:gameplay/fishing/junk");
    ASSERT_NE(junkTable, nullptr);

    // 垃圾表应该能生成物品
    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();
    auto items = junkTable->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_FALSE(items[0].isEmpty());
}

TEST_F(LootTest, FishingLootTable_TreasureTableExists)
{
    // 测试宝藏表存在
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* treasureTable = manager.getTable("minecraft:gameplay/fishing/treasure");
    ASSERT_NE(treasureTable, nullptr);

    math::Random rng(12345);

    // 必须设置开放水域为 true 才能生成宝藏
    auto context =
        LootContextBuilder(m_world).withRandom(rng).withOwnedValue(LootParams::IS_IN_OPEN_WATER, true).build();
    auto items = treasureTable->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_FALSE(items[0].isEmpty());
}

TEST_F(LootTest, FishingLootTable_MainTableExists)
{
    // 测试主表存在
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* fishingTable = manager.getTable("minecraft:gameplay/fishing");
    ASSERT_NE(fishingTable, nullptr);

    math::Random rng(12345);

    // 设置掉落表解析器，因为主表使用 TableLootEntry 引用子表
    auto context = LootContextBuilder(m_world)
                       .withRandom(rng)
                       .withLuck(0.0f)
                       .withOwnedValue(LootParams::IS_IN_OPEN_WATER, true)
                       .withLootTableResolver(
                           [&manager](const std::string& id) -> const LootTable* { return manager.getTable(id); })
                       .build();
    auto items = fishingTable->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_FALSE(items[0].isEmpty());
}

TEST_F(LootTest, FishingLootTable_TreasureRequiresOpenWater)
{
    // 测试宝藏条目需要开放水域条件
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* treasureTable = manager.getTable("minecraft:gameplay/fishing/treasure");
    ASSERT_NE(treasureTable, nullptr);

    // 宝藏表中的所有条目都应该有开放水域条件
    // 由于宝藏表中的条目有开放水域条件，在非开放水域时不应该生成宝藏
    math::Random rng(12345);

    // 在非开放水域中测试
    auto closedWaterContext =
        LootContextBuilder(m_world).withRandom(rng).withOwnedValue(LootParams::IS_IN_OPEN_WATER, false).build();
    auto items = treasureTable->generate(*closedWaterContext);

    // 在非开放水域，宝藏表不应该生成任何物品（所有条目都有开放水域条件）
    EXPECT_TRUE(items.empty());
}

TEST_F(LootTest, FishingLootTable_TreasureInOpenWater)
{
    // 测试在开放水域中可以钓到宝藏
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* treasureTable = manager.getTable("minecraft:gameplay/fishing/treasure");
    ASSERT_NE(treasureTable, nullptr);

    math::Random rng(12345);

    // 在开放水域中测试
    auto openWaterContext =
        LootContextBuilder(m_world).withRandom(rng).withOwnedValue(LootParams::IS_IN_OPEN_WATER, true).build();
    auto items = treasureTable->generate(*openWaterContext);

    // 在开放水域，宝藏表应该能生成物品
    ASSERT_EQ(1, items.size());
    EXPECT_FALSE(items[0].isEmpty());

    // 应该是宝藏物品之一
    const std::string itemId = items[0].getItem()->toString();
    EXPECT_TRUE(itemId == "minecraft:name_tag" || itemId == "minecraft:saddle" || itemId == "minecraft:bow" ||
        itemId == "minecraft:fishing_rod" || itemId == "minecraft:book" || itemId == "minecraft:nautilus_shell");
}

TEST_F(LootTest, FishingLootTable_LuckAffectsQuality)
{
    // 测试幸运值影响掉落质量
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* fishingTable = manager.getTable("minecraft:gameplay/fishing");
    ASSERT_NE(fishingTable, nullptr);

    // 统计高幸运值时宝藏物品的出现次数
    i32 treasureCount = 0;
    const i32 iterations = 100;

    for (i32 i = 0; i < iterations; ++i) {
        math::Random rng(i);

        // 高幸运值（海之眷顾 III 提供约 0.06 的幸运值）
        auto context = LootContextBuilder(m_world)
                           .withRandom(rng)
                           .withLuck(0.06f)
                           .withOwnedValue(LootParams::IS_IN_OPEN_WATER, true)
                           .withLootTableResolver(
                               [&manager](const std::string& id) -> const LootTable* { return manager.getTable(id); })
                           .build();
        auto items = fishingTable->generate(*context);

        if (items.size() == 1) {
            const std::string itemId = items[0].getItem()->toString();
            // 宝藏物品
            if (itemId == "minecraft:name_tag" || itemId == "minecraft:saddle" || itemId == "minecraft:bow" ||
                itemId == "minecraft:fishing_rod" || itemId == "minecraft:book" ||
                itemId == "minecraft:nautilus_shell") {
                treasureCount++;
            }
        }
    }

    // 高幸运值应该增加宝藏概率
    // 主表宝藏权重 5 + 幸运*2 = 5 + 0.12 = 5.12
    // 鱼表权重 85 - 幸运*1 = 85 - 0.06 = 84.94
    // 宝藏概率约 5.12 / (5.12 + 84.94 + 9.88) ≈ 5.1%
    // 但由于随机性，只检查至少钓到一些宝藏
    EXPECT_GT(treasureCount, 0);
}

TEST_F(LootTest, FishingLootTable_FishJunkTreasureDistribution)
{
    // 测试鱼/垃圾/宝藏的分布
    LootTableManager manager;
    loadLegacyEquivalentLootTables(manager);

    const LootTable* fishingTable = manager.getTable("minecraft:gameplay/fishing");
    ASSERT_NE(fishingTable, nullptr);

    i32 fishCount = 0;
    i32 junkCount = 0;
    i32 treasureCount = 0;
    const i32 iterations = 1000;

    for (i32 i = 0; i < iterations; ++i) {
        math::Random rng(i);
        auto context = LootContextBuilder(m_world)
                           .withRandom(rng)
                           .withLuck(0.0f)
                           .withOwnedValue(LootParams::IS_IN_OPEN_WATER, true)
                           .withLootTableResolver(
                               [&manager](const std::string& id) -> const LootTable* { return manager.getTable(id); })
                           .build();
        auto items = fishingTable->generate(*context);

        if (items.size() == 1) {
            const std::string itemId = items[0].getItem()->toString();

            // 鱼类
            if (itemId == "minecraft:cod" || itemId == "minecraft:salmon" || itemId == "minecraft:tropical_fish" ||
                itemId == "minecraft:pufferfish") {
                fishCount++;
            }
            // 宝藏
            else if (itemId == "minecraft:name_tag" || itemId == "minecraft:saddle" || itemId == "minecraft:bow" ||
                itemId == "minecraft:fishing_rod" || itemId == "minecraft:book" ||
                itemId == "minecraft:nautilus_shell") {
                treasureCount++;
            }
            // 垃圾
            else {
                junkCount++;
            }
        }
    }

    // 验证分布大致正确：
    // 鱼表权重 85，垃圾表权重 10，宝藏表权重 5
    // 鱼应该占主导地位
    EXPECT_GT(fishCount, junkCount);
    EXPECT_GT(junkCount, treasureCount);

    // 比例检查（允许一定误差）
    f32 fishRatio = static_cast<f32>(fishCount) / iterations;
    f32 junkRatio = static_cast<f32>(junkCount) / iterations;
    f32 treasureRatio = static_cast<f32>(treasureCount) / iterations;

    // 鱼约 85%，垃圾约 10%，宝藏约 5%
    EXPECT_GT(fishRatio, 0.75f);     // 允许 10% 误差
    EXPECT_GT(junkRatio, 0.05f);     // 允许 5% 误差
    EXPECT_GT(treasureRatio, 0.01f); // 允许 4% 误差
}
