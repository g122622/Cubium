#include <gtest/gtest.h>

#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/blocks/HopperBlock.hpp"
#include "world/block/blocks/TrappedChestBlock.hpp"
#include "world/block/blocks/functional/BarrelBlock.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "world/blockentity/interactive/PistonBlockEntity.hpp"
#include "world/blockentity/interactive/SignEntity.hpp"
#include "world/blockentity/processing/BeaconEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "world/blockentity/storage/BarrelEntity.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/blockentity/storage/EnderChestEntity.hpp"
#include "world/blockentity/storage/ShulkerBoxEntity.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/blockentity/transport/HopperEntity.hpp"
#include "world/chunk/ChunkData.hpp"
#include "world/block/VanillaBlocks.hpp"

#include <unordered_map>

using namespace mc;

namespace {

class DummyWorld final : public IWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override {
        const BlockPos pos(x, y, z);
        const auto it = m_statesByPos.find(pos);
        if (it != m_statesByPos.end()) {
            return it->second;
        }
        return m_state;
    }

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override {
        const BlockPos pos(x, y, z);
        m_statesByPos[pos] = state;
        m_state = state;
        ++m_setBlockCalls;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override {
        const BlockPos pos(x, y, z);
        m_statesByPos[pos] = state;
        m_state = state;
        m_lastSetBlockState = state;
        m_lastSetFlags = flags;
        ++m_setBlockStateCalls;
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override { return nullptr; }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return m_height; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity*) const override {
        MC_UNUSED(box);
        return m_entitiesInAabb;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override {
        return m_entitiesInRange;
    }

    [[nodiscard]] DimensionId dimension() const override { return 0; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override {
        m_entities[pos] = entity;
    }

    /**
     * @brief 设置 getEntitiesInAABB 的返回内容，便于构造阻挡场景。
     */
    void setEntitiesInAabbResult(const std::vector<Entity*>& entities) {
        m_entitiesInAabb = entities;
    }

    /**
     * @brief 设置 getEntitiesInRange 的返回内容，便于附魔台动画测试。
     */
    void setEntitiesInRangeResult(const std::vector<Entity*>& entities) {
        m_entitiesInRange = entities;
    }

    /**
     * @brief 设置世界列高，用于测试需要向上扫描的方块实体逻辑。
     */
    void setHeight(i32 height) {
        m_height = height;
    }

    [[nodiscard]] i32 setBlockCalls() const { return m_setBlockCalls; }
    [[nodiscard]] i32 setBlockStateCalls() const { return m_setBlockStateCalls; }
    [[nodiscard]] i32 lastSetFlags() const { return m_lastSetFlags; }
    [[nodiscard]] const BlockState* lastSetBlockState() const { return m_lastSetBlockState; }

    // TickManager interface (stubbed)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }

private:
    const BlockState* m_state = nullptr;
    const BlockState* m_lastSetBlockState = nullptr;
    i32 m_setBlockCalls = 0;
    i32 m_setBlockStateCalls = 0;
    i32 m_lastSetFlags = -1;
    std::unordered_map<BlockPos, BlockEntity*> m_entities;
    std::unordered_map<BlockPos, const BlockState*> m_statesByPos;
    std::vector<Entity*> m_entitiesInAabb;
    std::vector<Entity*> m_entitiesInRange;
    i32 m_height = world::MAX_BUILD_HEIGHT;
};

class BlockEntityTodoTestHelper : public ::testing::Test {
protected:
    static const BlockState* makeChestState() {
        static const blocks::ChestBlock s_chestBlock(
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state =
            &s_chestBlock.defaultState().with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
        return s_state;
    }

    static const BlockState* makeTrappedChestState() {
        static const blocks::TrappedChestBlock s_trappedChestBlock(
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state =
            &s_trappedChestBlock.defaultState().with(BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
        return s_state;
    }

    static const BlockState* makeHopperEnabledState() {
        static const blocks::HopperBlock s_hopperBlock(
            BlockProperties(Material::WOOD).hardness(3.0f).resistance(4.8f));
        static const BlockState* s_state =
            &s_hopperBlock.defaultState().with(BlockStateProperties::ENABLED(), true);
        return s_state;
    }

    static const BlockState* makeHopperDisabledState() {
        static const blocks::HopperBlock s_hopperBlock(
            BlockProperties(Material::WOOD).hardness(3.0f).resistance(4.8f));
        static const BlockState* s_state =
            &s_hopperBlock.defaultState().with(BlockStateProperties::ENABLED(), false);
        return s_state;
    }

    static const BlockState* makeBarrelOpenState() {
        static const blocks::BarrelBlock s_barrelBlock(
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state =
            &s_barrelBlock.defaultState().with(BlockStateProperties::OPEN(), true);
        return s_state;
    }
};

} // namespace

TEST(BlockEntityTodoTest, FurnaceInventoryOutputSlotRejectsManualPlacement) {
    Items::initialize();
    blockentity::FurnaceInventory inventory;
    const ItemStack stone(*Items::STONE, 1);

    EXPECT_FALSE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_OUTPUT, stone));
}

TEST(BlockEntityTodoTest, FurnaceInventoryInputAndFuelSlotsAcceptItems) {
    Items::initialize();
    blockentity::FurnaceInventory inventory;
    const ItemStack stone(*Items::STONE, 1);
    const ItemStack coal(*Items::COAL, 1);

    EXPECT_TRUE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_INPUT, stone));
    EXPECT_TRUE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_FUEL, coal));
}

TEST_F(BlockEntityTodoTestHelper, BarrelEntityTickResetsSyncCounterAndPersistsOpenCount) {
    blockentity::BarrelEntity barrel(BlockPos(1, 2, 3));
    DummyWorld world;
    world.setBlock(1, 2, 3, makeBarrelOpenState());

    barrel.setWorld(&world);
    barrel.openContainer(nullptr);
    ASSERT_EQ(barrel.getOpenCount(), 1);

    for (int i = 0; i < 12; ++i) {
        barrel.tick(world);
    }

    EXPECT_GE(world.setBlockStateCalls(), 1);

    nlohmann::json data;
    barrel.save(data);
    ASSERT_TRUE(data.contains("open_count"));
    EXPECT_EQ(data["open_count"].get<i32>(), 1);

    blockentity::BarrelEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));
    EXPECT_EQ(loaded.getOpenCount(), 1);
}

TEST(BlockEntityTodoTest, EnderChestOpenCloseAndTickAnimationBehavesCorrectly) {
    blockentity::EnderChestEntity enderChest(BlockPos(2, 3, 4));

    EXPECT_FALSE(enderChest.openContainer(nullptr));
    EXPECT_FALSE(enderChest.canPlayerAccess(nullptr));

    Player player(1, "tester");
    EXPECT_TRUE(enderChest.canPlayerAccess(&player));
    ASSERT_TRUE(enderChest.openContainer(&player));
    EXPECT_EQ(enderChest.getOpenCount(), 1);

    DummyWorld world;
    for (int i = 0; i < 12; ++i) {
        enderChest.tick(world);
    }

    EXPECT_GT(enderChest.getLidAngle(), 0.0f);

    enderChest.closeContainer(&player);
    EXPECT_EQ(enderChest.getOpenCount(), 0);

    nlohmann::json data;
    enderChest.save(data);
    ASSERT_TRUE(data.contains("open_count"));
    EXPECT_EQ(data["open_count"].get<i32>(), 0);
}

TEST(BlockEntityTodoTest, SignEntityRejectsControlCharactersAndTruncatesText) {
    blockentity::SignEntity sign(BlockPos(3, 4, 5));

    EXPECT_FALSE(sign.setLine(0, String("bad") + static_cast<char>(1) + "text"));

    const String longText = "0123456789abcdef";
    EXPECT_TRUE(sign.setLine(1, longText));
    EXPECT_EQ(sign.getLine(1), "0123456789abcde");
}

TEST(BlockEntityTodoTest, SignEntityLoadRejectsInvalidControlCharacters) {
    blockentity::SignEntity sign(BlockPos(3, 4, 5));

    nlohmann::json data;
    data["id"] = "minecraft:sign";
    data["x"] = 3;
    data["y"] = 4;
    data["z"] = 5;
    data["lines"] = nlohmann::json::array({String("ok"), String("x") + static_cast<char>(2)});

    EXPECT_FALSE(sign.load(data));
}

TEST(BlockEntityTodoTest, EnchantingTableAnimationOpensWhenNearbyPlayerExists) {
    blockentity::EnchantingTableEntity table(BlockPos(8, 64, 8));
    DummyWorld world;

    Player nearbyPlayer(42, "nearby");
    nearbyPlayer.setPosition(8.5f, 64.0f, 9.5f);
    world.setEntitiesInRangeResult({&nearbyPlayer});

    table.updateAnimation(world, 1.0f / 20.0f);
    EXPECT_GT(table.getBookOpenAmount(), 0.0f);
}

TEST(BlockEntityTodoTest, EnchantingTableAnimationClosesWithoutNearbyPlayers) {
    blockentity::EnchantingTableEntity table(BlockPos(8, 64, 8));
    DummyWorld world;

    Player nearbyPlayer(42, "nearby");
    nearbyPlayer.setPosition(8.5f, 64.0f, 9.5f);
    world.setEntitiesInRangeResult({&nearbyPlayer});
    table.updateAnimation(world, 1.0f / 20.0f);
    ASSERT_GT(table.getBookOpenAmount(), 0.0f);

    world.setEntitiesInRangeResult({});
    for (int i = 0; i < 30; ++i) {
        table.updateAnimation(world, 1.0f / 20.0f);
    }

    EXPECT_LT(table.getBookOpenAmount(), 0.1f);
}

TEST(BlockEntityTodoTest, ShulkerBoxCanOpenReturnsFalseWhenEntityBlocksTopSpace) {
    blockentity::ShulkerBoxEntity shulker(BlockPos(8, 20, 8));
    DummyWorld world;

    world.setEntitiesInAabbResult({static_cast<Entity*>(nullptr)});

    EXPECT_FALSE(shulker.canOpen(world));
}

TEST(BlockEntityTodoTest, ShulkerBoxCanOpenReturnsTrueWhenTopSpaceIsClear) {
    blockentity::ShulkerBoxEntity shulker(BlockPos(8, 20, 8));
    DummyWorld world;

    world.setEntitiesInAabbResult({});

    EXPECT_TRUE(shulker.canOpen(world));
}

TEST_F(BlockEntityTodoTestHelper, ChestEntityOpenCloseBroadcastsWhenWorldAttached) {
    DummyWorld world;
    world.setBlock(0, 0, 0, makeChestState());

    blockentity::ChestEntity chest(BlockPos(0, 0, 0));
    chest.setWorld(&world);

    chest.openContainer(nullptr);
    chest.closeContainer(nullptr);

    EXPECT_EQ(chest.getOpenCount(), 0);
    EXPECT_GE(world.setBlockStateCalls(), 2);
}

TEST_F(BlockEntityTodoTestHelper, ChestEntityTickPerformsPeriodicStateSync) {
    DummyWorld world;
    world.setBlock(0, 0, 0, makeChestState());

    blockentity::ChestEntity chest(BlockPos(0, 0, 0));
    chest.setWorld(&world);

    for (int i = 0; i < 205; ++i) {
        chest.tick(world);
    }

    EXPECT_GE(world.setBlockStateCalls(), 1);
}

TEST_F(BlockEntityTodoTestHelper, TrappedChestOpenCloseTriggersNeighborUpdatePath) {
    DummyWorld world;
    world.setBlock(4, 5, 6, makeTrappedChestState());

    blockentity::TrappedChestEntity trapped(BlockPos(4, 5, 6));
    trapped.setWorld(&world);
    world.setBlockEntity(trapped.getPos(), &trapped);

    trapped.openContainer(nullptr);
    trapped.openContainer(nullptr);
    EXPECT_EQ(trapped.getOpenCount(), 2);
    EXPECT_EQ(trapped.getRedstoneSignal(world), 2);

    trapped.closeContainer(nullptr);
    EXPECT_EQ(trapped.getOpenCount(), 1);
    EXPECT_EQ(trapped.getRedstoneSignal(world), 1);
    EXPECT_GE(world.setBlockStateCalls(), 1);
}

TEST_F(BlockEntityTodoTestHelper, HopperTickSkipsTransferWhenDisabledByState) {
    blockentity::HopperEntity hopper(BlockPos(10, 20, 30));
    DummyWorld world;
    world.setBlock(10, 20, 30, makeHopperDisabledState());

    hopper.tick(world);

    EXPECT_EQ(hopper.getTransferCooldown(), -1);
}

TEST_F(BlockEntityTodoTestHelper, HopperTickResetsCooldownWhenEnabledByState) {
    blockentity::HopperEntity hopper(BlockPos(10, 20, 30));
    DummyWorld world;
    world.setBlock(10, 20, 30, makeHopperEnabledState());

    hopper.tick(world);

    EXPECT_GE(hopper.getTransferCooldown(), 0);
}

TEST(BlockEntityTodoTest, HopperGetInventoryAtPositionEntityFallbackWithoutInventoryReturnsNull) {
    DummyWorld world;
    world.setEntitiesInAabbResult({static_cast<Entity*>(nullptr)});

    IInventory* found = blockentity::HopperEntity::getInventoryAtPosition(&world, BlockPos(1, 2, 3));

    EXPECT_EQ(found, nullptr);
}





TEST(BlockEntityTodoTest, PistonMovesCollidedEntitiesAlongFacingDirection) {
    VanillaBlocks::initialize();

    blockentity::PistonBlockEntity piston(
        BlockPos(0, 64, 0),
        VanillaBlocks::getState(VanillaBlocks::STONE),
        Direction::East,
        true,
        false);

    DummyWorld world;
    Entity pushedEntity(LegacyEntityType::Item, 101, &world);
    pushedEntity.setPosition(0.5f, 64.1f, 0.5f);
    const f32 beforeX = pushedEntity.x();

    world.setEntitiesInAabbResult({&pushedEntity});
    piston.tick(world);

    EXPECT_GT(pushedEntity.x(), beforeX);
}

TEST(BlockEntityTodoTest, BeaconPaymentRoundTripAndClonePreservePaymentItem) {
    VanillaBlocks::initialize();
    Items::initialize();

    blockentity::BeaconEntity beacon(BlockPos(5, 70, 5));
    beacon.setPaymentItem(ItemStack(*Items::IRON_INGOT, 1));

    nlohmann::json data;
    beacon.save(data);
    ASSERT_TRUE(data.contains("payment_item"));

    blockentity::BeaconEntity loaded(BlockPos(5, 70, 5));
    ASSERT_TRUE(loaded.load(data));
    ASSERT_FALSE(loaded.getPaymentItem().isEmpty());
    ASSERT_NE(loaded.getPaymentItem().getItem(), nullptr);
    EXPECT_EQ(loaded.getPaymentItem().getItem(), Items::IRON_INGOT);

    const auto cloned = loaded.clone();
    const auto* clonedBeacon = dynamic_cast<const blockentity::BeaconEntity*>(cloned.get());
    ASSERT_NE(clonedBeacon, nullptr);
    ASSERT_FALSE(clonedBeacon->getPaymentItem().isEmpty());
    ASSERT_NE(clonedBeacon->getPaymentItem().getItem(), nullptr);
    EXPECT_EQ(clonedBeacon->getPaymentItem().getItem(), Items::IRON_INGOT);
}

TEST(BlockEntityTodoTest, BeaconDetectsThreeLevelPyramidWithoutSkyCheck) {
    VanillaBlocks::initialize();

    DummyWorld world;
    world.setHeight(70);

    const BlockPos beaconPos(10, 64, 10);
    blockentity::BeaconEntity beacon(beaconPos);
    const auto primaryEffect = entity::effect::EffectType::Speed;
    beacon.setPrimaryEffect(&primaryEffect);

    const BlockState* baseState = VanillaBlocks::getState(VanillaBlocks::IRON_BLOCK);
    ASSERT_NE(baseState, nullptr);

    // Build a 3-level pyramid
    for (int level = 1; level <= 3; ++level) {
        const int y = beaconPos.y - level;
        for (int dx = -level; dx <= level; ++dx) {
            for (int dz = -level; dz <= level; ++dz) {
                world.setBlock(beaconPos.x + dx, y, beaconPos.z + dz, baseState);
            }
        }
    }

    world.setBlock(beaconPos.x, beaconPos.y + 1, beaconPos.z, &VanillaBlocks::AIR->defaultState());

    for (int i = 0; i < 80; ++i) {
        beacon.tick(world);
    }
    EXPECT_EQ(beacon.getLevel(), 3);

    // MC 1.16.5: Beacon pyramid detection does NOT check sky visibility
    // Placing a block above the beacon should NOT affect pyramid level
    const BlockState* blocking = VanillaBlocks::getState(VanillaBlocks::STONE);
    ASSERT_NE(blocking, nullptr);
    world.setBlock(beaconPos.x, beaconPos.y + 1, beaconPos.z, blocking);

    for (int i = 0; i < 80; ++i) {
        beacon.tick(world);
    }
    // Beacon level should still be 3 (sky visibility is not checked in MC 1.16.5)
    EXPECT_EQ(beacon.getLevel(), 3);
}

