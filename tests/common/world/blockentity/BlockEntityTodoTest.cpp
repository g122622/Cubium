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

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "util/math/random/Random.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/WorldConstants.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockPos.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/blocks/BookshelfBlock.hpp"
#include "world/block/blocks/ChestBlock.hpp"
#include "world/block/blocks/EnchantingTableBlock.hpp"
#include "world/block/blocks/HopperBlock.hpp"
#include "world/block/blocks/TrappedChestBlock.hpp"
#include "world/block/blocks/functional/BarrelBlock.hpp"
#include "world/block/blocks/functional/LecternBlock.hpp"
#include "world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "world/blockentity/interactive/LecternEntity.hpp"
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
#include "world/border/WorldBorder.hpp"
#include "world/chunk/data/ChunkData.hpp"

#include <unordered_map>

using namespace mc;

namespace {

class DummyWorld final : public mc::test::BaseTestWorld {
public:
    using IWorld::getBlockState;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const BlockPos pos(x, y, z);
        const auto it = m_statesByPos.find(pos);
        if (it != m_statesByPos.end()) {
            return it->second;
        }
        return m_state;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        const BlockPos pos(x, y, z);
        m_statesByPos[pos] = state;
        m_state = state;
        ++m_setBlockCalls;
        return true;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state, i32 flags) override
    {
        const BlockPos pos(x, y, z);
        m_statesByPos[pos] = state;
        m_state = state;
        m_lastSetBlockState = state;
        m_lastSetFlags = flags;
        ++m_setBlockStateCalls;
        return true;
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override { return m_height; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32, i32) const override { return true; }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& box, const Entity*) const override
    {
        MC_UNUSED(box);
        return m_entitiesInAabb;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return m_entitiesInRange;
    }

    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second;
    }

    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override
    {
        const auto it = m_entities.find(pos);
        return it == m_entities.end() ? nullptr : it->second;
    }

    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override { m_entities[pos] = entity; }

    /**
     * @brief 设置 getEntitiesInAABB 的返回内容，便于构造阻挡场景。
     */
    void setEntitiesInAabbResult(const std::vector<Entity*>& entities) { m_entitiesInAabb = entities; }

    /**
     * @brief 设置 getEntitiesInRange 的返回内容，便于附魔台动画测试。
     */
    void setEntitiesInRangeResult(const std::vector<Entity*>& entities) { m_entitiesInRange = entities; }

    /**
     * @brief 设置世界列高，用于测试需要向上扫描的方块实体逻辑。
     */
    void setHeight(i32 height) { m_height = height; }

    [[nodiscard]] i32 setBlockCalls() const { return m_setBlockCalls; }
    [[nodiscard]] i32 setBlockStateCalls() const { return m_setBlockStateCalls; }
    [[nodiscard]] i32 lastSetFlags() const { return m_lastSetFlags; }
    [[nodiscard]] const BlockState* lastSetBlockState() const { return m_lastSetBlockState; }

    // notifyBlockUpdate 计数：c20a7597c 后 ChestEntity/BarrelEntity 等改用 notifyBlockUpdate
    // 通知客户端（替代 setBlockState(pos,state,3)，因方块状态未变时 setBlockState 会被跳过）。
    void notifyBlockUpdate(const BlockPos& pos) override
    {
        ++m_notifyBlockUpdateCalls;
        (void)pos;
    }
    [[nodiscard]] i32 notifyBlockUpdateCalls() const { return m_notifyBlockUpdateCalls; }

    void resetSetBlockStateCalls() { m_setBlockStateCalls = 0; }

    // TickManager interface (stubbed)
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("DummyWorld::tickManager not implemented");
    }

private:
    const BlockState* m_state = nullptr;
    const BlockState* m_lastSetBlockState = nullptr;
    i32 m_setBlockCalls = 0;
    i32 m_setBlockStateCalls = 0;
    i32 m_lastSetFlags = -1;
    i32 m_notifyBlockUpdateCalls = 0;
    std::unordered_map<BlockPos, BlockEntity*> m_entities;
    std::unordered_map<BlockPos, const BlockState*> m_statesByPos;
    std::vector<Entity*> m_entitiesInAabb;
    std::vector<Entity*> m_entitiesInRange;
    i32 m_height = world::MAX_BUILD_HEIGHT;
};

class BlockEntityTodoTestHelper : public ::testing::Test {
protected:
    static const BlockState* makeChestState()
    {
        static const blocks::ChestBlock s_chestBlock(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state = &s_chestBlock.defaultState().with(
            BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
        return s_state;
    }

    static const BlockState* makeTrappedChestState()
    {
        static const blocks::TrappedChestBlock s_trappedChestBlock(
            BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state = &s_trappedChestBlock.defaultState().with(
            BlockStateProperties::CHEST_TYPE(), BlockStateProperties::ChestType::Single);
        return s_state;
    }

    static const BlockState* makeHopperEnabledState()
    {
        static const blocks::HopperBlock s_hopperBlock(BlockProperties(Material::WOOD).hardness(3.0f).resistance(4.8f));
        static const BlockState* s_state = &s_hopperBlock.defaultState().with(BlockStateProperties::ENABLED(), true);
        return s_state;
    }

    static const BlockState* makeHopperDisabledState()
    {
        static const blocks::HopperBlock s_hopperBlock(BlockProperties(Material::WOOD).hardness(3.0f).resistance(4.8f));
        static const BlockState* s_state = &s_hopperBlock.defaultState().with(BlockStateProperties::ENABLED(), false);
        return s_state;
    }

    static const BlockState* makeBarrelOpenState()
    {
        static const blocks::BarrelBlock s_barrelBlock(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
        static const BlockState* s_state = &s_barrelBlock.defaultState().with(BlockStateProperties::OPEN(), true);
        return s_state;
    }
};

} // namespace

TEST(BlockEntityTodoTest, FurnaceInventoryOutputSlotRejectsManualPlacement)
{
    Items::initialize();
    blockentity::FurnaceInventory inventory;
    const ItemStack stone(*Items::STONE, 1);

    EXPECT_FALSE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_OUTPUT, stone));
}

TEST(BlockEntityTodoTest, FurnaceInventoryInputAndFuelSlotsAcceptItems)
{
    Items::initialize();
    blockentity::FurnaceInventory inventory;
    const ItemStack stone(*Items::STONE, 1);
    const ItemStack coal(*Items::COAL, 1);

    EXPECT_TRUE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_INPUT, stone));
    EXPECT_TRUE(inventory.canPlaceItem(blockentity::FurnaceInventory::SLOT_FUEL, coal));
}

TEST_F(BlockEntityTodoTestHelper, BarrelEntityTickResetsSyncCounterAndPersistsOpenCount)
{
    blockentity::BarrelEntity barrel(BlockPos(1, 2, 3));
    DummyWorld world;
    world.setBlockState(1, 2, 3, makeBarrelOpenState());

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

TEST(BlockEntityTodoTest, EnderChestOpenCloseAndTickAnimationBehavesCorrectly)
{
    blockentity::EnderChestEntity enderChest(BlockPos(2, 3, 4));

    EXPECT_FALSE(enderChest.openContainer(nullptr));
    EXPECT_FALSE(enderChest.canPlayerAccess(nullptr));

    Player player(1, "tester", mc::test::testEcsRegistry());
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

TEST(BlockEntityTodoTest, SignEntityRejectsControlCharactersAndTruncatesText)
{
    blockentity::SignEntity sign(BlockPos(3, 4, 5));

    EXPECT_FALSE(sign.setLineFromLegacy(0, std::string("bad") + static_cast<char>(1) + "text"));

    const std::string longText = "0123456789abcdef";
    EXPECT_TRUE(sign.setLineFromLegacy(1, longText));
    EXPECT_EQ(sign.getLineText(1), "0123456789abcde");
}

TEST(BlockEntityTodoTest, SignEntityLoadRejectsInvalidControlCharacters)
{
    blockentity::SignEntity sign(BlockPos(3, 4, 5));

    nlohmann::json data;
    data["id"] = "minecraft:sign";
    data["x"] = 3;
    data["y"] = 4;
    data["z"] = 5;
    data["lines"] = nlohmann::json::array({std::string("ok"), std::string("x") + static_cast<char>(2)});

    // load 会清理无效控制字符而非失败，仍然返回 true
    EXPECT_TRUE(sign.load(data));
    // 第一行应该正常加载
    EXPECT_EQ(sign.getLineText(0), "ok");
    // 第二行应该被清理为空（包含控制字符）
    EXPECT_EQ(sign.getLineText(1), "");
}

TEST(BlockEntityTodoTest, EnchantingTableAnimationOpensWhenNearbyPlayerExists)
{
    blockentity::EnchantingTableEntity table(BlockPos(8, 64, 8));
    DummyWorld world;

    Player nearbyPlayer(42, "nearby", mc::test::testEcsRegistry());
    nearbyPlayer.setPosition(8.5f, 64.0f, 9.5f);
    world.setEntitiesInRangeResult({&nearbyPlayer});

    table.updateAnimation(world, 1.0f / 20.0f);
    EXPECT_GT(table.getBookOpenAmount(), 0.0f);
}

TEST(BlockEntityTodoTest, EnchantingTableAnimationClosesWithoutNearbyPlayers)
{
    blockentity::EnchantingTableEntity table(BlockPos(8, 64, 8));
    DummyWorld world;

    Player nearbyPlayer(42, "nearby", mc::test::testEcsRegistry());
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

TEST(BlockEntityTodoTest, ShulkerBoxCanOpenReturnsFalseWhenEntityBlocksTopSpace)
{
    blockentity::ShulkerBoxEntity shulker(BlockPos(8, 20, 8));
    DummyWorld world;

    world.setEntitiesInAabbResult({static_cast<Entity*>(nullptr)});

    EXPECT_FALSE(shulker.canOpen(world));
}

TEST(BlockEntityTodoTest, ShulkerBoxCanOpenReturnsTrueWhenTopSpaceIsClear)
{
    blockentity::ShulkerBoxEntity shulker(BlockPos(8, 20, 8));
    DummyWorld world;

    world.setEntitiesInAabbResult({});

    EXPECT_TRUE(shulker.canOpen(world));
}

TEST_F(BlockEntityTodoTestHelper, ChestEntityOpenCloseBroadcastsWhenWorldAttached)
{
    DummyWorld world;
    world.setBlockState(0, 0, 0, makeChestState());

    blockentity::ChestEntity chest(BlockPos(0, 0, 0));
    chest.setWorld(&world);

    chest.openContainer(nullptr);
    chest.closeContainer(nullptr);

    EXPECT_EQ(chest.getOpenCount(), 0);
    // c20a7597c 后 ChestEntity.broadcastChestState 改用 notifyBlockUpdate 通知客户端
    // （替代 setBlockState(pos,state,3)，方块状态未变时 setBlockState 会被 ServerWorld 跳过）。
    // open/close 各触发一次 broadcastChestState → notifyBlockUpdate。
    EXPECT_GE(world.notifyBlockUpdateCalls(), 2);
}

TEST_F(BlockEntityTodoTestHelper, ChestEntityTickPerformsPeriodicStateSync)
{
    DummyWorld world;
    world.setBlockState(0, 0, 0, makeChestState());

    blockentity::ChestEntity chest(BlockPos(0, 0, 0));
    chest.setWorld(&world);

    for (int i = 0; i < 205; ++i) {
        chest.tick(world);
    }

    // 每 SYNC_INTERVAL(200) ticks 调一次 notifyBlockUpdate 同步客户端
    EXPECT_GE(world.notifyBlockUpdateCalls(), 1);
}

TEST_F(BlockEntityTodoTestHelper, TrappedChestOpenCloseTriggersNeighborUpdatePath)
{
    DummyWorld world;
    world.setBlockState(4, 5, 6, makeTrappedChestState());

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
    // open/close 经 broadcastChestState → notifyBlockUpdate 通知客户端（c20a7597c 后）
    EXPECT_GE(world.notifyBlockUpdateCalls(), 1);
}

TEST_F(BlockEntityTodoTestHelper, HopperTickSkipsTransferWhenDisabledByState)
{
    blockentity::HopperEntity hopper(BlockPos(10, 20, 30));
    DummyWorld world;
    world.setBlockState(10, 20, 30, makeHopperDisabledState());

    hopper.tick(world);

    EXPECT_EQ(hopper.getTransferCooldown(), -1);
}

TEST_F(BlockEntityTodoTestHelper, HopperTickResetsCooldownWhenEnabledByState)
{
    blockentity::HopperEntity hopper(BlockPos(10, 20, 30));
    DummyWorld world;
    world.setBlockState(10, 20, 30, makeHopperEnabledState());

    hopper.tick(world);

    EXPECT_GE(hopper.getTransferCooldown(), 0);
}

TEST(BlockEntityTodoTest, HopperGetInventoryAtPositionEntityFallbackWithoutInventoryReturnsNull)
{
    DummyWorld world;
    world.setEntitiesInAabbResult({static_cast<Entity*>(nullptr)});

    InventoryRef found = blockentity::HopperEntity::getInventoryAtPosition(&world, BlockPos(1, 2, 3));

    EXPECT_EQ(found.get(), nullptr);
}

TEST(BlockEntityTodoTest, PistonMovesCollidedEntitiesAlongFacingDirection)
{
    VanillaBlocks::initialize();

    blockentity::PistonBlockEntity piston(
        BlockPos(0, 64, 0), VanillaBlocks::getState(VanillaBlocks::STONE), Direction::East, true, false);

    DummyWorld world;
    Entity pushedEntity(EntityInstanceId(101), &world, mc::test::testEcsRegistry());
    pushedEntity.setPosition(0.5f, 64.1f, 0.5f);
    const f32 beforeX = pushedEntity.x();

    world.setEntitiesInAabbResult({&pushedEntity});
    piston.tick(world);

    EXPECT_GT(pushedEntity.x(), beforeX);
}

TEST(BlockEntityTodoTest, BeaconPaymentRoundTripAndClonePreservePaymentItem)
{
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

TEST(BlockEntityTodoTest, BeaconDetectsThreeLevelPyramidWithoutSkyCheck)
{
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
                world.setBlockState(beaconPos.x + dx, y, beaconPos.z + dz, baseState);
            }
        }
    }

    world.setBlockState(beaconPos.x, beaconPos.y + 1, beaconPos.z, &VanillaBlocks::AIR->defaultState());

    for (int i = 0; i < 80; ++i) {
        beacon.tick(world);
    }
    EXPECT_EQ(beacon.getLevel(), 3);

    // MC 1.16.5: Beacon pyramid detection does NOT check sky visibility
    // Placing a block above the beacon should NOT affect pyramid level
    const BlockState* blocking = VanillaBlocks::getState(VanillaBlocks::STONE);
    ASSERT_NE(blocking, nullptr);
    world.setBlockState(beaconPos.x, beaconPos.y + 1, beaconPos.z, blocking);

    for (int i = 0; i < 80; ++i) {
        beacon.tick(world);
    }
    // Beacon level should still be 3 (sky visibility is not checked in MC 1.16.5)
    EXPECT_EQ(beacon.getLevel(), 3);
}

// ========== EnchantingTableEntity isValidBookshelf 测试 ==========

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_BookshelfAtOffset2IsValid)
{
    // 附魔台在(0,0,0)，书架在(2,0,0)，中间(1,0,0)是空气
    // 这是有效的书架位置
    DummyWorld world;
    VanillaBlocks::initialize();

    // 先设置中间位置为nullptr（空气），避免DummyWorld的m_state回退问题
    world.setBlockState(1, 0, 0, nullptr);

    // 设置书架在(2,0,0)
    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 0, 0, bookshelf);

    BlockPos offset(2, 0, 0); // BOOKSHELF_OFFSETS中的偏移量
    EXPECT_TRUE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_BookshelfBlockedBySolidBlock)
{
    // 附魔台在(0,0,0)，书架在(2,0,0)，中间(1,0,0)是石头（不可替换）
    // 这应该是无效的
    DummyWorld world;
    VanillaBlocks::initialize();

    // 设置书架在(2,0,0)
    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 0, 0, bookshelf);

    // 设置石头在(1,0,0) - 石头不可替换，应阻挡附魔力量
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    world.setBlockState(1, 0, 0, stone);

    BlockPos offset(2, 0, 0);
    EXPECT_FALSE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_NoBookshelfAtOffset)
{
    // 附魔台在(0,0,0)，(2,0,0)没有书架
    DummyWorld world;
    VanillaBlocks::initialize();

    BlockPos offset(2, 0, 0);
    EXPECT_FALSE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_CornerOffsetIsValid)
{
    // 附魔台在(0,0,0)，书架在角落(2,0,2)，中间(1,0,1)是空气
    DummyWorld world;
    VanillaBlocks::initialize();

    // 先设置中间位置为nullptr（空气），避免DummyWorld的m_state回退问题
    world.setBlockState(1, 0, 1, nullptr);

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 0, 2, bookshelf);

    BlockPos offset(2, 0, 2);
    EXPECT_TRUE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_UpperLevelBookshelf)
{
    // 附魔台在(0,0,0)，书架在(2,1,0)（Y=1层），中间(1,1,0)是空气
    DummyWorld world;
    VanillaBlocks::initialize();

    // 先设置中间位置为nullptr（空气），避免DummyWorld的m_state回退问题
    world.setBlockState(1, 1, 0, nullptr);

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 1, 0, bookshelf);

    BlockPos offset(2, 1, 0);
    EXPECT_TRUE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableRecalculateEnchantPower_CountsBookshelves)
{
    // 放置5个书架，计算附魔力量
    // DummyWorld的getBlockState()在位置未显式设置时返回m_state（最后setBlockState的值），
    // 所以需要将所有候选书架位置和中间位置显式设为nullptr，确保不会误判
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);

    // 先将所有30个候选书架位置和对应的中间位置设为nullptr（空气）
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    // 书架位置
                    world.setBlockState(x, y, z, nullptr);
                    // 中间位置 = (x/2, y, z/2)
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 在附魔台周围放置5个书架（水平2格，Y=0和Y=1）
    world.setBlockState(2, 0, 0, bookshelf);
    world.setBlockState(-2, 0, 0, bookshelf);
    world.setBlockState(0, 0, 2, bookshelf);
    world.setBlockState(0, 0, -2, bookshelf);
    world.setBlockState(2, 1, 0, bookshelf);

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 5);
}

TEST(BlockEntityTodoTest, EnchantingTableRecalculateEnchantPower_MaxIsFifteen)
{
    // 放置20个书架，附魔力量上限为15
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);

    // 先将所有中间位置设为nullptr（空气），避免DummyWorld的m_state回退问题
    // 中间位置 = 附魔台位置 + (offset.x/2, offset.y, offset.z/2)
    // 对于所有|x|==2或|z|==2的偏移量，可能的中间位置为：
    // x方向：-1, 0, 1；z方向：-1, 0, 1；y方向：0, 1
    for (i32 mx = -1; mx <= 1; ++mx) {
        for (i32 my = 0; my <= 1; ++my) {
            for (i32 mz = -1; mz <= 1; ++mz) {
                world.setBlockState(mx, my, mz, nullptr);
            }
        }
    }

    // 在所有30个候选位置放置书架
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, bookshelf);
                }
            }
        }
    }

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 15); // 最大15
}

TEST(BlockEntityTodoTest, EnchantingTableRecalculateEnchantPower_ZeroBookshelves)
{
    // 没有书架，附魔力量为0
    DummyWorld world;
    VanillaBlocks::initialize();

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 0);
}

// ========== canBeReplaced() 中间方块测试 ==========

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_ShortGrassAsMiddleBlock)
{
    // 附魔台在(0,0,0)，书架在(2,0,0)，中间(1,0,0)是短草（canBeReplaced=true）
    // 短草不是空气但可被替换，附魔力量应能穿过
    DummyWorld world;
    VanillaBlocks::initialize();

    // 设置中间位置为短草（REPLACEABLE_PLANT材质，canBeReplaced=true）
    const BlockState* shortGrass = VanillaBlocks::getState(VanillaBlocks::SHORT_GRASS);
    ASSERT_NE(shortGrass, nullptr);
    ASSERT_TRUE(shortGrass->canBeReplaced());

    // 先将所有候选书架位置设为nullptr（避免m_state回退问题）
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                }
            }
        }
    }

    // 设置书架在(2,0,0)，中间位置(1,0,0)设为短草
    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 0, 0, bookshelf);
    world.setBlockState(1, 0, 0, shortGrass);

    BlockPos offset(2, 0, 0);
    EXPECT_TRUE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableIsValidBookshelf_DandelionAsMiddleBlock)
{
    // 附魔台在(0,0,0)，书架在(2,0,0)，中间(1,0,0)是蒲公英（canBeReplaced=true）
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* dandelion = VanillaBlocks::getState(VanillaBlocks::DANDELION);
    ASSERT_NE(dandelion, nullptr);
    ASSERT_TRUE(dandelion->canBeReplaced());

    // 先将所有候选书架位置设为nullptr
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                }
            }
        }
    }

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    world.setBlockState(2, 0, 0, bookshelf);
    world.setBlockState(1, 0, 0, dandelion);

    BlockPos offset(2, 0, 0);
    EXPECT_TRUE(blockentity::EnchantingTableEntity::isValidBookshelf(world, BlockPos(0, 0, 0), offset));
}

TEST(BlockEntityTodoTest, EnchantingTableRecalculateEnchantPower_ShortGrassAllowsPower)
{
    // 中间方块为短草时，附魔力量应正常计算
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* shortGrass = VanillaBlocks::getState(VanillaBlocks::SHORT_GRASS);

    // 先将所有候选书架位置和中间位置设为nullptr
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 放置1个书架，中间位置为短草（应能通过）
    world.setBlockState(2, 0, 0, bookshelf);
    world.setBlockState(1, 0, 0, shortGrass); // 中间方块为短草

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 1);
}

TEST(BlockEntityTodoTest, EnchantingTableRecalculateEnchantPower_StoneBlocksPower)
{
    // 中间方块为石头时，附魔力量应被阻挡
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 先将所有候选书架位置和中间位置设为nullptr
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 放置1个书架，中间位置为石头（应阻挡附魔力量）
    world.setBlockState(2, 0, 0, bookshelf);
    world.setBlockState(1, 0, 0, stone); // 中间方块为石头

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 0);
}

// ========== BookshelfBlock 通知机制测试 ==========

TEST(BlockEntityTodoTest, BookshelfBlock_OnBlockAdded_NotifiesEnchantingTable)
{
    // 书架放置时，应通知2格范围内的附魔台重新计算附魔力量
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelfState = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);

    // 先将所有候选书架位置和中间位置设为nullptr
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 在(0,0,0)放置附魔台
    world.setBlockState(0, 0, 0, enchantingTableState);

    // 创建附魔台实体并放入世界
    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    world.setBlockEntity(BlockPos(0, 0, 0), &table);

    // 初始附魔力量应为0
    EXPECT_EQ(table.getEnchantPower(), 0);

    // 放置书架在(2,0,0)，中间(1,0,0)为空气(nullptr)
    world.setBlockState(2, 0, 0, bookshelfState);

    // 调用BookshelfBlock::onBlockAdded
    blocks::BookshelfBlock& bookshelfBlock = static_cast<blocks::BookshelfBlock&>(*VanillaBlocks::BOOKSHELF);
    bookshelfBlock.onBlockAdded(world, BlockPos(2, 0, 0), *bookshelfState);

    // 附魔台应被通知重新计算，附魔力量应为1
    EXPECT_EQ(table.getEnchantPower(), 1);
}

TEST(BlockEntityTodoTest, BookshelfBlock_OnBlockRemoved_NotifiesEnchantingTable)
{
    // 书架移除时，应通知2格范围内的附魔台重新计算附魔力量
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelfState = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);

    // 先将所有候选书架位置和中间位置设为nullptr
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 在(0,0,0)放置附魔台
    world.setBlockState(0, 0, 0, enchantingTableState);

    // 创建附魔台实体
    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    world.setBlockEntity(BlockPos(0, 0, 0), &table);

    // 先放置书架在(2,0,0)并手动计算附魔力量
    world.setBlockState(2, 0, 0, bookshelfState);
    table.recalculateEnchantPower(world);
    EXPECT_EQ(table.getEnchantPower(), 1);

    // 模拟书架被移除：先更新世界状态
    world.setBlockState(2, 0, 0, nullptr);

    // 调用BookshelfBlock::onBlockRemoved
    blocks::BookshelfBlock& bookshelfBlock = static_cast<blocks::BookshelfBlock&>(*VanillaBlocks::BOOKSHELF);
    bookshelfBlock.onBlockRemoved(world, BlockPos(2, 0, 0), *bookshelfState);

    // 附魔台应被通知重新计算，附魔力量应回到0
    EXPECT_EQ(table.getEnchantPower(), 0);
}

TEST(BlockEntityTodoTest, BookshelfBlock_DoesNotNotifyDistantEnchantingTable)
{
    // 书架不应通知3格外的附魔台（超出2格范围）
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelfState = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);

    // 先将候选位置清空
    for (i32 x = -3; x <= 3; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -3; z <= 3; ++z) {
                world.setBlockState(x, y, z, nullptr);
            }
        }
    }

    // 在(0,0,0)放置附魔台，书架在(3,0,0)（超出2格范围）
    world.setBlockState(0, 0, 0, enchantingTableState);

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    world.setBlockEntity(BlockPos(0, 0, 0), &table);

    // 放置书架在(3,0,0)（超出附魔台的2格检测范围）
    world.setBlockState(3, 0, 0, bookshelfState);

    blocks::BookshelfBlock& bookshelfBlock = static_cast<blocks::BookshelfBlock&>(*VanillaBlocks::BOOKSHELF);
    bookshelfBlock.onBlockAdded(world, BlockPos(3, 0, 0), *bookshelfState);

    // 附魔台不应受到影响（书架在3格外，不会被通知）
    EXPECT_EQ(table.getEnchantPower(), 0);
}

// ========== EnchantingTableBlock neighborChanged 和 tick 测试 ==========

TEST(BlockEntityTodoTest, EnchantingTableBlock_NeighborChanged_RecalculatesPower)
{
    // 当附魔台的邻居方块变化时，neighborChanged应重新计算附魔力量
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelfState = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);

    // 先将候选位置清空
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 在(0,0,0)放置附魔台
    world.setBlockState(0, 0, 0, enchantingTableState);

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    world.setBlockEntity(BlockPos(0, 0, 0), &table);

    // 初始附魔力量应为0
    EXPECT_EQ(table.getEnchantPower(), 0);

    // 放置书架在(2,0,0)
    world.setBlockState(2, 0, 0, bookshelfState);

    // 调用EnchantingTableBlock::neighborChanged
    blocks::EnchantingTableBlock& enchantingTableBlock =
        static_cast<blocks::EnchantingTableBlock&>(*VanillaBlocks::ENCHANTING_TABLE);
    enchantingTableBlock.neighborChanged(world, BlockPos(0, 0, 0), *VanillaBlocks::BOOKSHELF, BlockPos(1, 0, 0), false);

    // 附魔力量应被重新计算为1
    EXPECT_EQ(table.getEnchantPower(), 1);
}

TEST(BlockEntityTodoTest, EnchantingTableBlock_NeighborChanged_NoBlockEntity_DoesNothing)
{
    // 当附魔台位置没有方块实体时，neighborChanged不应崩溃
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);
    world.setBlockState(0, 0, 0, enchantingTableState);

    // 没有设置方块实体
    blocks::EnchantingTableBlock& enchantingTableBlock =
        static_cast<blocks::EnchantingTableBlock&>(*VanillaBlocks::ENCHANTING_TABLE);

    // 不应崩溃
    EXPECT_NO_THROW(enchantingTableBlock.neighborChanged(
        world, BlockPos(0, 0, 0), *VanillaBlocks::STONE, BlockPos(1, 0, 0), false));
}

TEST(BlockEntityTodoTest, EnchantingTableBlock_Tick_RecalculatesPower)
{
    // EnchantingTableBlock::tick应重新计算附魔力量
    // 这模拟了onBlockAdded中调度1tick延迟后的行为
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* bookshelfState = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);

    // 先将候选位置清空
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 y = 0; y <= 1; ++y) {
            for (i32 z = -2; z <= 2; ++z) {
                if (std::abs(x) == 2 || std::abs(z) == 2) {
                    world.setBlockState(x, y, z, nullptr);
                    world.setBlockState(x / 2, y, z / 2, nullptr);
                }
            }
        }
    }

    // 在(0,0,0)放置附魔台和书架
    world.setBlockState(0, 0, 0, enchantingTableState);
    world.setBlockState(2, 0, 0, bookshelfState);

    blockentity::EnchantingTableEntity table(BlockPos(0, 0, 0));
    world.setBlockEntity(BlockPos(0, 0, 0), &table);

    // 初始附魔力量应为0（尚未计算）
    EXPECT_EQ(table.getEnchantPower(), 0);

    // 调用EnchantingTableBlock::tick
    blocks::EnchantingTableBlock& enchantingTableBlock =
        static_cast<blocks::EnchantingTableBlock&>(*VanillaBlocks::ENCHANTING_TABLE);
    BlockState* mutableState = const_cast<BlockState*>(enchantingTableState);
    math::Random random(42);
    enchantingTableBlock.tick(world, BlockPos(0, 0, 0), *mutableState, random);

    // tick后附魔力量应被计算为1
    EXPECT_EQ(table.getEnchantPower(), 1);
}

TEST(BlockEntityTodoTest, EnchantingTableBlock_Tick_NoBlockEntity_DoesNothing)
{
    // 当附魔台位置没有方块实体时，tick不应崩溃
    DummyWorld world;
    VanillaBlocks::initialize();

    const BlockState* enchantingTableState = VanillaBlocks::getState(VanillaBlocks::ENCHANTING_TABLE);
    world.setBlockState(0, 0, 0, enchantingTableState);

    blocks::EnchantingTableBlock& enchantingTableBlock =
        static_cast<blocks::EnchantingTableBlock&>(*VanillaBlocks::ENCHANTING_TABLE);
    BlockState* mutableState = const_cast<BlockState*>(enchantingTableState);
    math::Random random(42);

    EXPECT_NO_THROW(enchantingTableBlock.tick(world, BlockPos(0, 0, 0), *mutableState, random));
}

// ============================================================================
// BarrelEntity 容器打开/关闭音效和状态更新测试
// ============================================================================

TEST_F(BlockEntityTodoTestHelper, BarrelEntityOpenCloseUpdatesBlockState)
{
    DummyWorld world;
    world.setBlockState(3, 5, 7, makeBarrelOpenState());

    blockentity::BarrelEntity barrel(BlockPos(3, 5, 7));
    barrel.setWorld(&world);

    // 打开时 m_openCount 增加并更新方块状态
    barrel.openContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 1);
    EXPECT_GE(world.setBlockStateCalls(), 1);

    // 关闭时 m_openCount 减少并更新方块状态
    barrel.closeContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 0);
}

TEST_F(BlockEntityTodoTestHelper, BarrelEntityMultipleOpenersIncrementCount)
{
    blockentity::BarrelEntity barrel(BlockPos(1, 2, 3));
    DummyWorld world;
    world.setBlockState(1, 2, 3, makeBarrelOpenState());
    barrel.setWorld(&world);

    barrel.openContainer(nullptr);
    barrel.openContainer(nullptr);
    barrel.openContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 3);

    barrel.closeContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 2);

    barrel.closeContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 1);
}

TEST_F(BlockEntityTodoTestHelper, BarrelEntitySoundAndStateOnlyOnFirstOpen)
{
    // MC原版：音效和方块状态更新仅在 openCount 从0变为1时触发
    // 后续玩家打开不应再触发音效和状态更新
    DummyWorld world;
    world.setBlockState(3, 5, 7, makeBarrelOpenState());

    blockentity::BarrelEntity barrel(BlockPos(3, 5, 7));
    barrel.setWorld(&world);

    // 第一次打开：应触发方块状态更新
    world.resetSetBlockStateCalls();
    barrel.openContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 1);
    EXPECT_GE(world.setBlockStateCalls(), 1); // 首次打开触发了_updateBlockState

    // 第二次打开：不应再触发方块状态更新
    world.resetSetBlockStateCalls();
    barrel.openContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 2);
    EXPECT_EQ(world.setBlockStateCalls(), 0); // 非首次打开不触发_updateBlockState

    // 第三次打开：同样不应触发
    world.resetSetBlockStateCalls();
    barrel.openContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 3);
    EXPECT_EQ(world.setBlockStateCalls(), 0);

    // 关闭一个：还有人在用，不应触发关闭状态更新和音效
    world.resetSetBlockStateCalls();
    barrel.closeContainer(nullptr);
    EXPECT_EQ(barrel.getOpenCount(), 2);
    // closeContainer在openCount > 0时仍更新状态为open=true，所以可能会有调用
    // 但不应播放关闭音效（openCount仍>0）
}

// ============================================================================
// LecternEntity 书本放置和翻页测试
// ============================================================================

TEST(BlockEntityTodoTest, LecternEntitySetBookAndPageNavigation)
{
    VanillaBlocks::initialize();
    Items::initialize();

    blockentity::LecternEntity lectern(BlockPos(4, 10, 6));

    // 讲台初始状态无书
    EXPECT_FALSE(lectern.hasBook());
    EXPECT_EQ(lectern.getPage(), 0);

    // 放入书本（使用 book 物品）
    const Item* bookItem = Items::BOOK;
    ASSERT_NE(bookItem, nullptr);
    ItemStack bookStack(*bookItem, 1);
    EXPECT_TRUE(lectern.setBook(bookStack));
    EXPECT_TRUE(lectern.hasBook());

    // 翻页测试（book 只有1页）
    EXPECT_EQ(lectern.getTotalPages(), 1);
    EXPECT_FALSE(lectern.nextPage());

    // 移除书本
    ItemStack removed = lectern.removeBook();
    EXPECT_FALSE(removed.isEmpty());
    EXPECT_FALSE(lectern.hasBook());
}

TEST(BlockEntityTodoTest, LecternEntityComparatorSignalBasedOnPage)
{
    VanillaBlocks::initialize();
    Items::initialize();

    blockentity::LecternEntity lectern(BlockPos(0, 0, 0));

    // 无书时信号为0
    EXPECT_EQ(lectern.getComparatorSignal(), 0);

    // 放入书本
    const Item* bookItem = Items::BOOK;
    ASSERT_NE(bookItem, nullptr);
    ItemStack bookStack(*bookItem, 1);
    lectern.setBook(bookStack);

    // 有书时至少有信号1
    EXPECT_GE(lectern.getComparatorSignal(), 1);
}

TEST(BlockEntityTodoTest, LecternEntityOpenCloseCountTracking)
{
    blockentity::LecternEntity lectern(BlockPos(2, 3, 4));

    EXPECT_EQ(lectern.getOpenCount(), 0);

    lectern.openContainer();
    EXPECT_EQ(lectern.getOpenCount(), 1);

    lectern.openContainer();
    EXPECT_EQ(lectern.getOpenCount(), 2);

    lectern.closeContainer();
    EXPECT_EQ(lectern.getOpenCount(), 1);

    lectern.closeContainer();
    EXPECT_EQ(lectern.getOpenCount(), 0);
}

TEST(BlockEntityTodoTest, LecternEntityCloseCountDoesNotGoBelowZero)
{
    blockentity::LecternEntity lectern(BlockPos(5, 6, 7));

    // 关闭次数超过打开次数时不应变为负数
    lectern.closeContainer();
    EXPECT_EQ(lectern.getOpenCount(), 0);
}

// ============================================================================
// LecternBlock::tryPlaceBook 集成测试
// ============================================================================

TEST_F(BlockEntityTodoTestHelper, LecternBlockTryPlaceBookSetsEntityBook)
{
    VanillaBlocks::initialize();
    Items::initialize();

    DummyWorld world;

    // 创建讲台方块状态
    static const blocks::LecternBlock s_lecternBlock(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState* lecternState = &s_lecternBlock.defaultState();
    world.setBlockState(3, 10, 5, lecternState);

    // 创建讲台方块实体
    auto lecternEntity = std::make_unique<blockentity::LecternEntity>(BlockPos(3, 10, 5));
    blockentity::LecternEntity* lecternPtr = lecternEntity.get();
    lecternEntity->setWorld(&world);
    world.setBlockEntity(BlockPos(3, 10, 5), lecternPtr);

    // 讲台初始无书
    EXPECT_FALSE(lecternPtr->hasBook());

    // 使用 book 物品放书
    const Item* bookItem = Items::BOOK;
    ASSERT_NE(bookItem, nullptr);
    ItemStack bookStack(*bookItem, 3);

    // 获取可修改的方块状态引用
    BlockState mutableState = *lecternState;
    bool result = blocks::LecternBlock::tryPlaceBook(world, BlockPos(3, 10, 5), bookStack);

    EXPECT_TRUE(result);
    EXPECT_TRUE(lecternPtr->hasBook());

    // tryPlaceBook 不负责消耗物品，调用方负责
    EXPECT_EQ(bookStack.getCount(), 3);
}

TEST_F(BlockEntityTodoTestHelper, LecternBlockTryPlaceBookFailsWhenBookAlreadyPresent)
{
    VanillaBlocks::initialize();
    Items::initialize();

    DummyWorld world;

    static const blocks::LecternBlock s_lecternBlock(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState* lecternState = &s_lecternBlock.defaultState();
    world.setBlockState(7, 20, 3, lecternState);

    auto lecternEntity = std::make_unique<blockentity::LecternEntity>(BlockPos(7, 20, 3));
    blockentity::LecternEntity* lecternPtr = lecternEntity.get();
    lecternEntity->setWorld(&world);
    world.setBlockEntity(BlockPos(7, 20, 3), lecternPtr);

    // 先放一本书
    const Item* bookItem = Items::BOOK;
    ItemStack firstBook(*bookItem, 1);
    lecternPtr->setBook(firstBook);
    ASSERT_TRUE(lecternPtr->hasBook());

    // 尝试再放一本应该失败
    ItemStack secondBook(*bookItem, 1);
    // 设置HAS_BOOK状态
    BlockState hasBookState = lecternState->with(BlockStateProperties::HAS_BOOK(), true);
    world.setBlockState(7, 20, 3, &hasBookState);

    bool result = blocks::LecternBlock::tryPlaceBook(world, BlockPos(7, 20, 3), secondBook);
    EXPECT_FALSE(result);
}

TEST_F(BlockEntityTodoTestHelper, LecternBlockTryPlaceBookFailsWithEmptyStack)
{
    VanillaBlocks::initialize();
    Items::initialize();

    DummyWorld world;

    static const blocks::LecternBlock s_lecternBlock(BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f));
    const BlockState* lecternState = &s_lecternBlock.defaultState();
    world.setBlockState(0, 0, 0, lecternState);

    auto lecternEntity = std::make_unique<blockentity::LecternEntity>(BlockPos(0, 0, 0));
    lecternEntity->setWorld(&world);
    world.setBlockEntity(BlockPos(0, 0, 0), lecternEntity.get());

    // 空物品堆应该失败
    ItemStack emptyStack;
    bool result = blocks::LecternBlock::tryPlaceBook(world, BlockPos(0, 0, 0), emptyStack);
    EXPECT_FALSE(result);
}
