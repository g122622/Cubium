#include <gtest/gtest.h>
#include "entity/loot/LootContext.hpp"
#include "entity/loot/LootTable.hpp"
#include "entity/loot/LootPool.hpp"
#include "entity/loot/LootEntry.hpp"
#include "entity/loot/LootConditions.hpp"
#include "entity/loot/LootFunctions.hpp"
#include "entity/loot/RandomRanges.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/Items.hpp"
#include "resource/ResourceLocation.hpp"
#include "world/IWorld.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/block/Block.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "util/math/random/Random.hpp"
#include "core/Constants.hpp"

using namespace mc;
using namespace mc::loot;

// Test implementation of IWorld for loot testing
class LootTestWorld : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override { return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 12345; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("LootTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("LootTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("LootTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("LootTestWorld::getRandom not implemented");
    }
};

class LootTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }

    LootTestWorld m_world;
};

// RandomValueRange Tests
TEST_F(LootTest, RandomValueRange_FixedValue) {
    RandomValueRange range(5.0f);
    math::Random rng(12345);
    EXPECT_EQ(5, range.generateInt(rng));
    EXPECT_FLOAT_EQ(5.0f, range.generateFloat(rng));
    EXPECT_TRUE(range.isFixed());
}

TEST_F(LootTest, RandomValueRange_Range) {
    RandomValueRange range(1.0f, 10.0f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 1);
        EXPECT_LE(value, 10);
    }
}

// BinomialRange Tests
TEST_F(LootTest, BinomialRange_Basic) {
    BinomialRange range(10, 0.5f);
    math::Random rng(12345);
    for (int i = 0; i < 10; ++i) {
        i32 value = range.generateInt(rng);
        EXPECT_GE(value, 0);
        EXPECT_LE(value, 10);
    }
}

TEST_F(LootTest, BinomialRange_ZeroProbability) {
    BinomialRange range(10, 0.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(0, range.generateInt(rng));
    }
}

TEST_F(LootTest, BinomialRange_FullProbability) {
    BinomialRange range(10, 1.0f);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(10, range.generateInt(rng));
    }
}

// ConstantRange Tests
TEST_F(LootTest, ConstantRange_Basic) {
    ConstantRange range(42);
    math::Random rng(12345);
    // 始终返回固定值
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(42, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_ZeroValue) {
    ConstantRange range(0);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(0, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_NegativeValue) {
    ConstantRange range(-5);
    math::Random rng(12345);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(-5, range.generateInt(rng));
    }
}

TEST_F(LootTest, ConstantRange_LargeValue) {
    ConstantRange range(1000000);
    math::Random rng(12345);
    EXPECT_EQ(1000000, range.generateInt(rng));
    EXPECT_EQ(1000000, range.getValue());
}

TEST_F(LootTest, ConstantRange_GetValue) {
    ConstantRange range(123);
    EXPECT_EQ(123, range.getValue());
}

// LootContext Tests
TEST_F(LootTest, LootContext_Builder) {
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef)
        .withRandom(rng)
        .withLuck(1.5f)
        .build();

    ASSERT_NE(context, nullptr);
    EXPECT_FLOAT_EQ(1.5f, context->getLuck());
}

TEST_F(LootTest, LootContext_LootingModifier) {
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef)
        .withRandom(rng)
        .withLootingModifier(3)
        .build();

    ASSERT_NE(context, nullptr);
    EXPECT_EQ(3, context->getLootingModifier());
}

// LootEntry Tests
TEST_F(LootTest, EmptyLootEntry_GenerateNothing) {
    EmptyLootEntry entry;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    std::vector<ItemStack> items;
    bool success = entry.generate([&items](const ItemStack& stack) {
        items.push_back(stack);
    }, *context);

    EXPECT_TRUE(success);
    EXPECT_TRUE(items.empty());
}

TEST_F(LootTest, ItemLootEntry_Weight) {
    ItemLootEntry entry("minecraft:porkchop", RandomValueRange(1.0f), 10, 2);
    EXPECT_EQ(10, entry.getWeight());
    EXPECT_EQ(2, entry.getQuality());
    EXPECT_EQ(10, entry.getEffectiveWeight(0.0f));
    EXPECT_EQ(12, entry.getEffectiveWeight(1.0f));
    EXPECT_EQ(8, entry.getEffectiveWeight(-1.0f));
}

// LootTable Tests
TEST_F(LootTest, LootTable_Empty) {
    LootTable table;
    math::Random rng(12345);
    IWorld& worldRef = m_world;
    auto context = LootContextBuilder(worldRef).withRandom(rng).build();

    auto items = table.generate(*context);
    EXPECT_TRUE(items.empty());
}

// LootTableManager Tests
TEST_F(LootTest, LootTableManager_RegisterAndGet) {
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

TEST_F(LootTest, LootTableManager_DefaultTables) {
    LootTableManager manager;
    manager.initializeDefaultTables();

    EXPECT_TRUE(manager.hasTable("minecraft:entities/pig"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/cow"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/sheep"));
    EXPECT_TRUE(manager.hasTable("minecraft:entities/chicken"));
}

// ============================================================================
// New LootFunction Tests
// ============================================================================

TEST_F(LootTest, CopyNameFunction_Creation) {
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    EXPECT_EQ("copy_name", func.getType());
    EXPECT_EQ(CopyNameFunction::Source::KillerPlayer, func.getSource());
}

TEST_F(LootTest, CopyNameFunction_Clone) {
    CopyNameFunction func(CopyNameFunction::Source::This);
    func.addCondition(std::make_unique<RandomChanceCondition>(0.5f));

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ("copy_name", cloned->getType());

    auto* clonedFunc = dynamic_cast<CopyNameFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNameFunction::Source::This, clonedFunc->getSource());
}

TEST_F(LootTest, CopyNameFunction_AllSources) {
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

TEST_F(LootTest, CopyBlockStateFunction_Creation) {
    CopyBlockStateFunction func("minecraft:chest");
    EXPECT_EQ("copy_block_state", func.getType());
    EXPECT_EQ("minecraft:chest", func.getBlockId());
    EXPECT_TRUE(func.getProperties().empty());
}

TEST_F(LootTest, CopyBlockStateFunction_Properties) {
    std::vector<std::string> props = {"facing", "waterlogged"};
    CopyBlockStateFunction func("minecraft:chest", props);
    EXPECT_EQ(2, func.getProperties().size());
    EXPECT_EQ("facing", func.getProperties()[0]);
    EXPECT_EQ("waterlogged", func.getProperties()[1]);
}

TEST_F(LootTest, CopyBlockStateFunction_Clone) {
    std::vector<std::string> props = {"facing"};
    CopyBlockStateFunction func("minecraft:furnace", props);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<CopyBlockStateFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ("minecraft:furnace", clonedFunc->getBlockId());
    EXPECT_EQ(1, clonedFunc->getProperties().size());
}

TEST_F(LootTest, CopyNbtFunction_Creation) {
    CopyNbtFunction func(CopyNbtFunction::Source::This);
    EXPECT_EQ("copy_nbt", func.getType());
    EXPECT_EQ(CopyNbtFunction::Source::This, func.getSource());
    EXPECT_TRUE(func.getOperations().empty());
}

TEST_F(LootTest, CopyNbtFunction_AddOperation) {
    CopyNbtFunction func(CopyNbtFunction::Source::BlockEntity);
    func.addOperation("CustomName", "display.Name", CopyNbtFunction::Operation::Replace);

    EXPECT_EQ(1, func.getOperations().size());
    EXPECT_EQ("CustomName", func.getOperations()[0].sourcePath);
    EXPECT_EQ("display.Name", func.getOperations()[0].targetPath);
    EXPECT_EQ(CopyNbtFunction::Operation::Replace, func.getOperations()[0].operation);
}

TEST_F(LootTest, CopyNbtFunction_Clone) {
    CopyNbtFunction func(CopyNbtFunction::Source::Killer);
    func.addOperation("path1", "path2", CopyNbtFunction::Operation::Append);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<CopyNbtFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNbtFunction::Source::Killer, clonedFunc->getSource());
    EXPECT_EQ(1, clonedFunc->getOperations().size());
}

TEST_F(LootTest, FillPlayerHeadFunction_Creation) {
    FillPlayerHeadFunction func(CopyNameFunction::Source::KillerPlayer);
    EXPECT_EQ("fill_player_head", func.getType());
    EXPECT_EQ(CopyNameFunction::Source::KillerPlayer, func.getSource());
}

TEST_F(LootTest, FillPlayerHeadFunction_Clone) {
    FillPlayerHeadFunction func(CopyNameFunction::Source::This);

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<FillPlayerHeadFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ(CopyNameFunction::Source::This, clonedFunc->getSource());
}

TEST_F(LootTest, SetAttributesFunction_Creation) {
    SetAttributesFunction func;
    EXPECT_EQ("set_attributes", func.getType());
    EXPECT_TRUE(func.getModifiers().empty());
}

TEST_F(LootTest, SetAttributesFunction_AddModifier) {
    SetAttributesFunction func;
    SetAttributesFunction::AttributeModifier mod;
    mod.name = "generic.attack_damage";
    mod.attributeId = "minecraft:generic.attack_damage";
    mod.value = 5.0f;
    mod.operation = 0;
    mod.slot = "mainhand";

    func.addModifier(mod);
    EXPECT_EQ(1, func.getModifiers().size());
    EXPECT_EQ("generic.attack_damage", func.getModifiers()[0].name);
}

TEST_F(LootTest, SetContentsFunction_Creation) {
    SetContentsFunction func;
    EXPECT_EQ("set_contents", func.getType());
}

TEST_F(LootTest, SetLootTableFunction_Creation) {
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);
    EXPECT_EQ("set_loot_table", func.getType());
    EXPECT_EQ("minecraft:chests/simple_dungeon", func.getLootTableId());
    EXPECT_EQ(12345, func.getSeed());
}

TEST_F(LootTest, SetLootTableFunction_Clone) {
    SetLootTableFunction func("minecraft:chests/spawn_bonus_chest");

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedFunc = dynamic_cast<SetLootTableFunction*>(cloned.get());
    ASSERT_NE(clonedFunc, nullptr);
    EXPECT_EQ("minecraft:chests/spawn_bonus_chest", clonedFunc->getLootTableId());
}

TEST_F(LootTest, ExplorationMapFunction_Creation) {
    ExplorationMapFunction func(ExplorationMapFunction::Destination::Mansion);
    EXPECT_EQ("exploration_map", func.getType());
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func.getDestination());
}

TEST_F(LootTest, ExplorationMapFunction_AllDestinations) {
    ExplorationMapFunction func1(ExplorationMapFunction::Destination::BuriedTreasure);
    ExplorationMapFunction func2(ExplorationMapFunction::Destination::Mansion);
    ExplorationMapFunction func3(ExplorationMapFunction::Destination::Monument);

    EXPECT_EQ(ExplorationMapFunction::Destination::BuriedTreasure, func1.getDestination());
    EXPECT_EQ(ExplorationMapFunction::Destination::Mansion, func2.getDestination());
    EXPECT_EQ(ExplorationMapFunction::Destination::Monument, func3.getDestination());
}

TEST_F(LootTest, SetStewEffectFunction_Creation) {
    SetStewEffectFunction func;
    EXPECT_EQ("set_stew_effect", func.getType());
    EXPECT_TRUE(func.getEffects().empty());
}

TEST_F(LootTest, SetStewEffectFunction_AddEffect) {
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

TEST_F(LootTest, LootFunctionBuilder_NewFunctions) {
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

TEST_F(LootTest, FurnaceSmeltFunction_EmptyStack) {
    // 空物品栈应返回空栈
    FurnaceSmeltFunction func;
    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, FurnaceSmeltFunction_Type) {
    FurnaceSmeltFunction func;
    EXPECT_EQ("furnace_smelt", func.getType());
}

TEST_F(LootTest, FurnaceSmeltFunction_Clone) {
    FurnaceSmeltFunction func;
    func.addCondition(std::make_unique<RandomChanceCondition>(0.5f));

    auto cloned = func.clone();
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ("furnace_smelt", cloned->getType());
    EXPECT_EQ(1, cloned->getConditions().size());
}

TEST_F(LootTest, FurnaceSmeltFunction_NoRecipe) {
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

TEST_F(LootTest, FurnaceSmeltFunction_Builder) {
    auto func = LootFunctionBuilder::furnaceSmelt();
    ASSERT_NE(func, nullptr);
    EXPECT_EQ("furnace_smelt", func->getType());
}
