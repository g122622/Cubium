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
#include "world/border/WorldBorder.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/block/Block.hpp"
#include "world/fluid/Fluid.hpp"
#include "world/tick/manager/TickManager.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
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

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("LootTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("LootTestWorld::worldBorder not implemented");
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

// ============================================================================
// CopyNameFunction::apply() 测试
// ============================================================================

TEST_F(LootTest, CopyNameFunction_EmptyStack) {
    // 空 ItemStack 应该直接返回
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, CopyNameFunction_NoEntityInContext) {
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

TEST_F(LootTest, CopyNameFunction_EntityWithoutCustomName) {
    // 实体没有自定义名称时，不应复制名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建一个没有自定义名称的实体
    Entity entity(LegacyEntityType::Pig, EntityId(1), nullptr);
    context.set(LootParams::THIS_ENTITY, &entity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 实体没有自定义名称，物品不应获得名称
    EXPECT_FALSE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_EntityWithCustomName) {
    // 从有自定义名称的实体复制名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建一个有自定义名称的实体
    Entity entity(LegacyEntityType::Pig, EntityId(1), nullptr);
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

TEST_F(LootTest, CopyNameFunction_KillerEntity) {
    // 从 KILLER_ENTITY 复制名称
    CopyNameFunction func(CopyNameFunction::Source::Killer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建击杀者实体
    Entity killer(LegacyEntityType::Zombie, EntityId(2), nullptr);
    killer.setCustomName("Killer Zombie");
    context.set(LootParams::KILLER_ENTITY, &killer);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("Killer Zombie", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_KillerPlayer) {
    // 从 KILLER_PLAYER 复制名称（玩家总是有名称）
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建玩家
    Player player(EntityId(3), "TestPlayer");
    context.set(LootParams::KILLER_PLAYER, &player);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 玩家即使没有自定义名称也应该复制显示名称
    EXPECT_TRUE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_KillerPlayerWithCustomName) {
    // 从有自定义名称的玩家复制
    CopyNameFunction func(CopyNameFunction::Source::KillerPlayer);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建有自定义名称的玩家
    Player player(EntityId(4), "OriginalName");
    player.setCustomName("CustomPlayerName");
    context.set(LootParams::KILLER_PLAYER, &player);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("CustomPlayerName", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_BlockEntityWithoutCustomName) {
    // 方块实体没有自定义名称时不应复制
    CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建箱子（没有自定义名称）
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    BlockEntity* blockEntity = &chest;  // 显式转换为基类指针
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    // 没有自定义名称，物品不应获得名称
    EXPECT_FALSE(result.hasCustomName());
}

TEST_F(LootTest, CopyNameFunction_BlockEntityWithCustomName) {
    // 从有自定义名称的方块实体复制
    CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建有自定义名称的箱子
    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    chest.setCustomName("My Special Chest");
    BlockEntity* blockEntity = &chest;  // 显式转换为基类指针
    context.set(LootParams::BLOCK_ENTITY, blockEntity);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);
    ItemStack result = func.apply(stack, context);

    EXPECT_TRUE(result.hasCustomName());
    EXPECT_EQ("My Special Chest", result.getCustomName());
}

TEST_F(LootTest, CopyNameFunction_DifferentSourcesIndependent) {
    // 不同来源应该独立工作
    math::Random rng(12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 创建多个实体
    Entity thisEntity(LegacyEntityType::Pig, EntityId(1), nullptr);
    thisEntity.setCustomName("This Pig");

    Entity killerEntity(LegacyEntityType::Zombie, EntityId(2), nullptr);
    killerEntity.setCustomName("Killer Zombie");

    Player player(EntityId(3), "PlayerName");
    player.setCustomName("Custom Player");

    blockentity::ChestEntity chest(BlockPos(0, 64, 0));
    chest.setCustomName("Named Chest");

    // 测试 THIS_ENTITY 来源
    {
        LootContext context(m_world, rng);
        context.set(LootParams::THIS_ENTITY, &thisEntity);
        context.set(LootParams::KILLER_ENTITY, &killerEntity);  // 设置另一个来源，确保不影响

        CopyNameFunction func(CopyNameFunction::Source::This);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("This Pig", result.getCustomName());
    }

    // 测试 KILLER_ENTITY 来源
    {
        LootContext context(m_world, rng);
        context.set(LootParams::KILLER_ENTITY, &killerEntity);
        context.set(LootParams::THIS_ENTITY, &thisEntity);  // 设置另一个来源，确保不影响

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
        BlockEntity* blockEntity = &chest;  // 显式转换为基类指针
        context.set(LootParams::BLOCK_ENTITY, blockEntity);

        CopyNameFunction func(CopyNameFunction::Source::BlockEntity);
        ItemStack stack(*diamond, 1);
        ItemStack result = func.apply(stack, context);

        EXPECT_EQ("Named Chest", result.getCustomName());
    }
}

TEST_F(LootTest, CopyNameFunction_OverwritesExistingName) {
    // 应该覆盖物品已有的自定义名称
    CopyNameFunction func(CopyNameFunction::Source::This);
    math::Random rng(12345);
    LootContext context(m_world, rng);

    // 创建实体
    Entity entity(LegacyEntityType::Pig, EntityId(1), nullptr);
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

TEST_F(LootTest, LootEntry_AddFunction) {
    // 测试 LootEntry 添加函数
    ItemLootEntry entry("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 添加函数
    entry.addFunction(std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops));
    entry.addFunction(std::make_unique<SetCountFunction>(RandomValueRange(2.0f, 4.0f)));

    // 验证函数数量
    EXPECT_EQ(2, entry.getFunctions().size());
}

TEST_F(LootTest, LootEntry_ApplyFunctionsCorrectly) {
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

TEST_F(LootTest, LootEntry_ApplyFunctionsInOrder) {
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

TEST_F(LootTest, LootEntry_ApplyFunctionsWithCondition) {
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

TEST_F(LootTest, LootEntry_CloneCopiesFunctions) {
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

TEST_F(LootTest, ItemLootEntry_GenerateAppliesFunctions) {
    // 测试 ItemLootEntry::generate 在条件检查后应用函数
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success = entry->generate([&generatedItems](const ItemStack& stack) {
        generatedItems.push_back(stack);
    }, *context);

    EXPECT_TRUE(success);
    ASSERT_EQ(1, generatedItems.size());
    // 函数应该将数量设置为 10
    EXPECT_EQ(10, generatedItems[0].getCount());
}

TEST_F(LootTest, ItemLootEntry_GenerateWithConditionAndFunction) {
    // 测试带条件的条目和函数
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 条件：50% 概率
    entry->addCondition(std::make_unique<RandomChanceCondition>(1.0f)); // 总是触发

    // 函数：设置数量
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(7.0f, 7.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success = entry->generate([&generatedItems](const ItemStack& stack) {
        generatedItems.push_back(stack);
    }, *context);

    EXPECT_TRUE(success);
    ASSERT_EQ(1, generatedItems.size());
    EXPECT_EQ(7, generatedItems[0].getCount());
}

TEST_F(LootTest, ItemLootEntry_GenerateConditionFails) {
    // 测试条件不满足时不生成物品
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 条件：永远不满足
    entry->addCondition(std::make_unique<RandomChanceCondition>(0.0f));

    // 函数：设置数量（不应该被应用）
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(10.0f, 10.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success = entry->generate([&generatedItems](const ItemStack& stack) {
        generatedItems.push_back(stack);
    }, *context);

    EXPECT_FALSE(success);
    EXPECT_TRUE(generatedItems.empty());
}

TEST_F(LootTest, ItemLootEntry_GenerateFunctionReturnsEmpty) {
    // 测试函数返回空堆时不生成物品
    auto entry = std::make_unique<ItemLootEntry>("minecraft:diamond", RandomValueRange(1.0f), 1, 0);

    // 函数：设置数量为 0（空堆）
    entry->addFunction(std::make_unique<SetCountFunction>(RandomValueRange(0.0f, 0.0f)));

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world).withRandom(rng).build();

    std::vector<ItemStack> generatedItems;
    bool success = entry->generate([&generatedItems](const ItemStack& stack) {
        generatedItems.push_back(stack);
    }, *context);

    // 条件满足，但函数返回空堆
    EXPECT_TRUE(success);
    EXPECT_TRUE(generatedItems.empty());
}

// ============================================================================
// LootEntryBuilder::function Tests
// ============================================================================

TEST_F(LootTest, LootEntryBuilder_FunctionChainCall) {
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

TEST_F(LootTest, LootEntryBuilder_BuildCopiesFunctions) {
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

TEST_F(LootTest, LootEntryBuilder_WithConditionAndFunction) {
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

TEST_F(LootTest, ApplyBonusFunction_OreDropsNoFortune) {
    // 测试没有时运时的 OreDrops 公式
    math::Random rng(12345);

    // 没有时运，应该返回基础数量
    for (int i = 0; i < 10; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(1, 0, rng);
        EXPECT_EQ(1, result);
    }
}

TEST_F(LootTest, ApplyBonusFunction_OreDropsWithFortune) {
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

TEST_F(LootTest, ApplyBonusFunction_OreDropsMultiplicative) {
    // 验证 OreDrops 是乘法式，不是加法式
    math::Random rng(12345);

    // 基础数量 2，Fortune III，最大应该是 2 * 4 = 8
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateOreDrops(2, 3, rng);
        EXPECT_GE(result, 2);  // 最小 2 * 1 = 2
        EXPECT_LE(result, 8);  // 最大 2 * 4 = 8
    }
}

TEST_F(LootTest, ApplyBonusFunction_UniformBonus) {
    // 测试均匀分布加成
    math::Random rng(12345);

    // Uniform: count + random(0, bonusMultiplier * fortune)
    // bonusMultiplier=1, fortune=3 -> 加成范围 [0, 3]
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateUniformBonus(5, 3, 1, rng);
        EXPECT_GE(result, 5);   // 5 + 0
        EXPECT_LE(result, 8);   // 5 + 3
    }
}

TEST_F(LootTest, ApplyBonusFunction_BinomialBonus) {
    // 测试二项分布加成
    math::Random rng(12345);

    // Binomial: count + binomial(fortune + extra, probability)
    // fortune=3, extra=1, probability=0.5 -> 4 次试验，每次 50% 概率
    for (int i = 0; i < 100; ++i) {
        i32 result = ApplyBonusFunction::calculateBinomialBonus(1, 3, 1, 0.5f, rng);
        EXPECT_GE(result, 1);   // 1 + 0
        EXPECT_LE(result, 5);   // 1 + 4
    }
}

TEST_F(LootTest, ApplyBonusFunction_IntegrationWithLootContext) {
    // 测试 ApplyBonusFunction 与 LootContext 的时运参数集成
    auto func = std::make_unique<ApplyBonusFunction>(ApplyBonusFunction::BonusType::OreDrops);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .withLootingModifier(3)  // Fortune III
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

TEST_F(LootTest, LootTable_DiamondOreWithSilkTouch) {
    // 测试钻石矿精准采集掉落
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/diamond_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    // 设置精准采集
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    // 精准采集应该掉落钻石矿石
    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:diamond_ore", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_DiamondOreWithFortune) {
    // 测试钻石矿时运加成
    // 注意：ApplyBonusFunction 需要 TOOL 参数才能应用时运加成
    // 这个测试验证在没有工具的情况下，掉落数量固定为 1
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/diamond_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    // 没有设置 TOOL，所以 ApplyBonusFunction 不会应用时运加成
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .withLootingModifier(3)
        .build();

    // 多次生成验证在没有工具时掉落数量固定为 1
    for (int i = 0; i < 10; ++i) {
        auto items = table->generate(*context);
        ASSERT_EQ(1, items.size());
        EXPECT_EQ("minecraft:diamond", items[0].getItem()->toString());
        // 没有工具时，数量固定为 1
        EXPECT_EQ(1, items[0].getCount());
    }
}

TEST_F(LootTest, LootTable_CoalOreWithSilkTouch) {
    // 测试煤矿精准采集掉落
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/coal_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:coal_ore", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_CoalOreWithFortune) {
    // 测试煤矿时运加成
    // 注意：ApplyBonusFunction 需要 TOOL 参数才能应用时运加成
    // 这个测试验证在没有工具的情况下，掉落数量固定为 1
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/coal_ore");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    // 没有设置 TOOL，所以 ApplyBonusFunction 不会应用时运加成
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .withLootingModifier(3)
        .build();

    // 多次生成验证在没有工具时掉落数量固定为 1
    for (int i = 0; i < 10; ++i) {
        auto items = table->generate(*context);
        ASSERT_EQ(1, items.size());
        EXPECT_EQ("minecraft:coal", items[0].getItem()->toString());
        // 没有工具时，数量固定为 1
        EXPECT_EQ(1, items[0].getCount());
    }
}

TEST_F(LootTest, LootTable_StoneWithSilkTouch) {
    // 测试石头精准采集掉落石头
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/stone");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();
    context->setOwnedValue(LootParams::SILK_TOUCH_LEVEL, 1);

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:stone", items[0].getItem()->toString());
}

TEST_F(LootTest, LootTable_StoneWithoutSilkTouch) {
    // 测试石头普通挖掘掉落圆石
    LootTableManager manager;
    manager.initializeDefaultTables();

    const LootTable* table = manager.getTable("minecraft:blocks/stone");
    ASSERT_NE(table, nullptr);

    math::Random rng(12345);

    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

    auto items = table->generate(*context);

    ASSERT_EQ(1, items.size());
    EXPECT_EQ("minecraft:cobblestone", items[0].getItem()->toString());
}

// ============================================================================
// CopyBlockStateFunction Apply Tests
// ============================================================================

TEST_F(LootTest, CopyBlockStateFunction_EmptyStack) {
    // 空物品堆不应该崩溃
    CopyBlockStateFunction func("minecraft:chest");

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, *context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, CopyBlockStateFunction_NoBlockStateInContext) {
    // 没有 BlockState 参数时应该返回原物品
    CopyBlockStateFunction func("minecraft:chest");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();
    // 不设置 BLOCK_STATE 参数

    ItemStack result = func.apply(stack, *context);
    EXPECT_EQ(stack.getItem(), result.getItem());
    EXPECT_EQ(stack.getCount(), result.getCount());
    // 不应该有 BlockStateTag
    EXPECT_FALSE(result.hasTag());
}

TEST_F(LootTest, CopyBlockStateFunction_BlockIdMismatch) {
    // 方块 ID 不匹配时不应复制
    CopyBlockStateFunction func("minecraft:chest");

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

    // 创建一个假的 BlockState（这里用空指针模拟不匹配情况）
    // 由于实际需要真正的 BlockState，这个测试验证函数不会崩溃
    ItemStack result = func.apply(stack, *context);
    EXPECT_EQ(stack.getItem(), result.getItem());
}

TEST_F(LootTest, CopyBlockStateFunction_EmptyPropertiesList) {
    // 空属性列表（应该复制所有属性）
    // 这个测试验证函数能正常处理空属性列表
    CopyBlockStateFunction func("minecraft:furnace", {});  // 空属性列表

    EXPECT_TRUE(func.getProperties().empty());
    EXPECT_EQ("minecraft:furnace", func.getBlockId());
}

TEST_F(LootTest, CopyBlockStateFunction_SpecifiedProperties) {
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

TEST_F(LootTest, SetLootTableFunction_EmptyStack) {
    // 空物品堆不应该崩溃
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

    ItemStack emptyStack;
    ItemStack result = func.apply(emptyStack, *context);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(LootTest, SetLootTableFunction_EmptyLootTableId) {
    // 空掉落表 ID 应该返回原物品
    SetLootTableFunction func("", 12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

    ItemStack result = func.apply(stack, *context);
    EXPECT_FALSE(result.hasTag());  // 不应该有标签
}

TEST_F(LootTest, SetLootTableFunction_BasicApply) {
    // 基本功能测试：设置掉落表 ID
    SetLootTableFunction func("minecraft:chests/simple_dungeon", 12345);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

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

TEST_F(LootTest, SetLootTableFunction_ZeroSeedNotStored) {
    // 种子为 0 时不应该存储
    SetLootTableFunction func("minecraft:chests/spawn_bonus_chest", 0);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

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

TEST_F(LootTest, SetLootTableFunction_OverwriteExistingTag) {
    // 测试覆盖现有的 BlockEntityTag
    SetLootTableFunction func1("minecraft:chests/first", 100);
    SetLootTableFunction func2("minecraft:chests/second", 200);

    const Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    ItemStack stack(*diamond, 1);

    math::Random rng(12345);
    auto context = LootContextBuilder(m_world)
        .withRandom(rng)
        .build();

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
