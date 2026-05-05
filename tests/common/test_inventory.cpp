#include <gtest/gtest.h>
#include "../src/common/entity/inventory/IInventory.hpp"
#include "../src/common/entity/inventory/Slot.hpp"
#include "../src/common/entity/inventory/PlayerInventory.hpp"
#include "../src/common/entity/core/LivingEntity.hpp"
#include "../src/common/item/armor/ArmorMaterial.hpp"
#include "../src/common/item/items/armor/ArmorItem.hpp"
#include "../src/common/item/items/armor/DyeableArmorItem.hpp"
#include "../src/common/item/items/armor/ElytraItem.hpp"
#include "../src/common/entity/entities/player/Player.hpp"
#include "../src/common/item/core/ItemRegistry.hpp"
#include "../src/common/item/Items.hpp"
#include "../src/common/world/IWorld.hpp"
#include "../src/common/world/chunk/ChunkData.hpp"
#include "../src/common/world/fluid/Fluid.hpp"
#include "../src/common/world/tick/manager/TickManager.hpp"
#include "../src/common/core/Constants.hpp"
#include "../src/common/world/block/Block.hpp"
#include "../src/common/util/math/random/Random.hpp"
#include "../src/common/world/blockentity/core/SimpleInventory.hpp"

#include <array>

using namespace mc;

namespace {

class TestLivingEntity final : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(LegacyEntityType::Player, 1) {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class ArmorTestWorld final : public IWorld {
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
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Normal; }
    [[nodiscard]] bool isClientSide() override { return false; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }

    void setCurrentTick(u64 tick) { m_currentTick = tick; }

    // TickManager interface (stubbed for tests)
    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }

    // Random interface (stubbed for tests)
    [[nodiscard]] math::Random& getRandom() override {
        throw std::runtime_error("ArmorTestWorld::getRandom not implemented");
    }
    [[nodiscard]] const math::Random& getRandom() const override {
        throw std::runtime_error("ArmorTestWorld::getRandom not implemented");
    }

private:
    u64 m_currentTick = 0;
};

} // namespace

// ============================================================================
// Slot 索引常量测试
// ============================================================================

class InventorySlotsTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }
};

TEST_F(InventorySlotsTest, ConstantsAreCorrect) {
    // 快捷栏
    EXPECT_EQ(InventorySlots::HOTBAR_START, 0);
    EXPECT_EQ(InventorySlots::HOTBAR_END, 8);
    EXPECT_EQ(InventorySlots::HOTBAR_SIZE, 9);

    // 主背包
    EXPECT_EQ(InventorySlots::MAIN_START, 9);
    EXPECT_EQ(InventorySlots::MAIN_END, 35);
    EXPECT_EQ(InventorySlots::MAIN_SIZE, 27);

    // 护甲
    EXPECT_EQ(InventorySlots::ARMOR_START, 36);
    EXPECT_EQ(InventorySlots::ARMOR_END, 39);
    EXPECT_EQ(InventorySlots::ARMOR_SIZE, 4);
    EXPECT_EQ(InventorySlots::ARMOR_HEAD, 36);
    EXPECT_EQ(InventorySlots::ARMOR_CHEST, 37);
    EXPECT_EQ(InventorySlots::ARMOR_LEGS, 38);
    EXPECT_EQ(InventorySlots::ARMOR_FEET, 39);

    // 副手
    EXPECT_EQ(InventorySlots::OFFHAND, 40);

    // 总大小
    EXPECT_EQ(InventorySlots::TOTAL_SIZE, 41);
}

// ============================================================================
// PlayerInventory 测试
// ============================================================================

class PlayerInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);

        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
        m_diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    }

    std::unique_ptr<PlayerInventory> m_inventory;
    Item* m_diamond = nullptr;
    Item* m_stick = nullptr;
    Item* m_diamondSword = nullptr;
};

TEST_F(PlayerInventoryTest, InitialState) {
    EXPECT_EQ(m_inventory->getContainerSize(), 41);
    EXPECT_TRUE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryTest, SetAndGetItem) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 32);
    m_inventory->setItem(0, stack);

    EXPECT_FALSE(m_inventory->isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_diamond);
}

TEST_F(PlayerInventoryTest, HotbarOperations) {
    ASSERT_NE(m_diamond, nullptr);

    // 设置选中槽位
    m_inventory->setSelectedSlot(5);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 5);

    // 边界检查
    m_inventory->setSelectedSlot(-1);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 0);

    m_inventory->setSelectedSlot(10);
    EXPECT_EQ(m_inventory->getSelectedSlot(), 8);

    // 设置选中物品
    ItemStack stack(*m_diamond, 10);
    m_inventory->setItem(3, stack);
    m_inventory->setSelectedSlot(3);
    EXPECT_EQ(m_inventory->getSelectedStack().getCount(), 10);
}

TEST_F(PlayerInventoryTest, RemoveItem) {
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 32));

    // 移除部分
    ItemStack removed = m_inventory->removeItem(0, 10);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 22);

    // 移除剩余部分
    removed = m_inventory->removeItem(0, 100);
    EXPECT_EQ(removed.getCount(), 22);
    EXPECT_TRUE(m_inventory->getItem(0).isEmpty());
}

TEST_F(PlayerInventoryTest, ClearInventory) {
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(40, ItemStack(*m_diamond, 5));

    EXPECT_FALSE(m_inventory->isEmpty());

    m_inventory->clear();

    EXPECT_TRUE(m_inventory->isEmpty());
}

TEST_F(PlayerInventoryTest, AddItem) {
    ASSERT_NE(m_diamond, nullptr);

    // 添加到空背包
    ItemStack stack(*m_diamond, 32);
    i32 remaining = m_inventory->add(stack);

    EXPECT_EQ(remaining, 32);  // 全部添加成功
    EXPECT_TRUE(stack.isEmpty());

    // 检查物品在快捷栏
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);
}

TEST_F(PlayerInventoryTest, AddItemMerging) {
    ASSERT_NE(m_diamond, nullptr);

    // 先放入一些钻石
    m_inventory->setItem(0, ItemStack(*m_diamond, 50));

    // 添加更多钻石（应该合并）
    ItemStack stack(*m_diamond, 20);
    i32 remaining = m_inventory->add(stack);

    // 槽位 0 从 50 变成 64（堆叠上限），剩余 6 个会放到下一个空槽位
    // MC 1.16.5 行为: 空槽位优先级是 选中槽 → 副手 → 快捷栏 → 主背包
    // 所以剩余的 6 个会放到副手槽 (slot 40)，而不是 slot 1
    EXPECT_EQ(remaining, 20);  // 全部添加成功
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 64);  // 达到堆叠上限
    EXPECT_TRUE(stack.isEmpty());  // 全部添加成功，stack 变空

    // 剩余的 6 个应该放在副手槽 (slot 40)
    EXPECT_EQ(m_inventory->getItem(40).getCount(), 6);
}

TEST_F(PlayerInventoryTest, AddMultipleItems) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    // 添加钻石
    ItemStack diamonds(*m_diamond, 32);
    m_inventory->add(diamonds);

    // 添加木棍
    ItemStack sticks(*m_stick, 16);
    m_inventory->add(sticks);

    EXPECT_EQ(m_inventory->countItem(*m_diamond), 32);
    EXPECT_EQ(m_inventory->countItem(*m_stick), 16);
}

TEST_F(PlayerInventoryTest, FindSlot) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(5, ItemStack(*m_diamond, 10));
    m_inventory->setItem(20, ItemStack(*m_stick, 5));

    EXPECT_EQ(m_inventory->findSlot(*m_diamond), 5);
    EXPECT_EQ(m_inventory->findSlot(*m_stick), 20);
    EXPECT_EQ(m_inventory->findSlot(*ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"))), -1);
}

TEST_F(PlayerInventoryTest, CountItem) {
    ASSERT_NE(m_diamond, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_diamond, 20));
    m_inventory->setItem(30, ItemStack(*m_diamond, 15));

    EXPECT_EQ(m_inventory->countItem(*m_diamond), 45);
}

TEST_F(PlayerInventoryTest, HasItem) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));

    EXPECT_TRUE(m_inventory->hasItem(*m_diamond));
    EXPECT_FALSE(m_inventory->hasItem(*m_stick));
}

TEST_F(PlayerInventoryTest, GetFirstEmptySlot) {
    ASSERT_NE(m_diamond, nullptr);

    // 空背包
    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 0);

    // 填充前几个槽位
    m_inventory->setItem(0, ItemStack(*m_diamond, 1));
    m_inventory->setItem(1, ItemStack(*m_diamond, 1));
    m_inventory->setItem(2, ItemStack(*m_diamond, 1));

    EXPECT_EQ(m_inventory->getFirstEmptySlot(), 3);
}

TEST_F(PlayerInventoryTest, SwapSlots) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    m_inventory->setItem(0, ItemStack(*m_diamond, 10));
    m_inventory->setItem(5, ItemStack(*m_stick, 5));

    m_inventory->swapSlots(0, 5);

    EXPECT_EQ(m_inventory->getItem(0).getItem(), m_stick);
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 5);
    EXPECT_EQ(m_inventory->getItem(5).getItem(), m_diamond);
    EXPECT_EQ(m_inventory->getItem(5).getCount(), 10);
}

TEST_F(PlayerInventoryTest, PlaceItem) {
    ASSERT_NE(m_diamond, nullptr);

    // 放入空槽位
    ItemStack stack(*m_diamond, 32);
    ItemStack remaining = m_inventory->placeItem(0, stack);
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 32);

    // 合并到现有堆
    ItemStack more(*m_diamond, 20);
    remaining = m_inventory->placeItem(0, more);
    EXPECT_TRUE(remaining.isEmpty());
    EXPECT_EQ(m_inventory->getItem(0).getCount(), 52);
}

TEST_F(PlayerInventoryTest, ArmorSlots) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack helmet(*m_diamond, 1);
    ItemStack chestplate(*m_diamond, 1);
    ItemStack leggings(*m_diamond, 1);
    ItemStack boots(*m_diamond, 1);

    m_inventory->setHelmet(helmet);
    m_inventory->setChestplate(chestplate);
    m_inventory->setLeggings(leggings);
    m_inventory->setBoots(boots);

    EXPECT_EQ(m_inventory->getHelmet().getCount(), 1);
    EXPECT_EQ(m_inventory->getChestplate().getCount(), 1);
    EXPECT_EQ(m_inventory->getLeggings().getCount(), 1);
    EXPECT_EQ(m_inventory->getBoots().getCount(), 1);

    // 通过索引访问
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_HEAD).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_CHEST).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_LEGS).getCount(), 1);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::ARMOR_FEET).getCount(), 1);
}

TEST(ArmorItemTest, RightClickEquipsMatchingArmorSlot) {
    ArmorTestWorld world;
    Player player(1, "armor-test");

    const std::array<std::pair<item::armor::ArmorSlot, i32>, 4> cases = {{
        {item::armor::ArmorSlot::Head, InventorySlots::ARMOR_HEAD},
        {item::armor::ArmorSlot::Chest, InventorySlots::ARMOR_CHEST},
        {item::armor::ArmorSlot::Legs, InventorySlots::ARMOR_LEGS},
        {item::armor::ArmorSlot::Feet, InventorySlots::ARMOR_FEET},
    }};

    for (const auto& [slot, inventorySlot] : cases) {
        player.inventory().clear();
        player.inventory().setSelectedSlot(0);

        item::items::ArmorItem armorItem(
            item::armor::ArmorMaterials::IRON,
            slot,
            ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(slot)));
        player.inventory().setItem(0, ItemStack(armorItem));

        ItemActionResult result = armorItem.onItemRightClick(world, player, Hand::MainHand);

        EXPECT_TRUE(result.isConsume());
        EXPECT_TRUE(result.getResult().isEmpty());
        EXPECT_TRUE(player.getHeldItem(Hand::MainHand).isEmpty());
        EXPECT_EQ(player.inventory().getItem(inventorySlot).getItem(), &armorItem);
        EXPECT_EQ(player.inventory().getItem(inventorySlot).getCount(), 1);
    }
}

TEST(ArmorItemTest, RightClickPassesWhenArmorSlotOccupied) {
    ArmorTestWorld world;
    Player player(2, "armor-pass-test");

    item::items::ArmorItem armorItem(
        item::armor::ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));
    player.inventory().setItem(0, ItemStack(armorItem));
    item::items::ArmorItem equippedHelmet(
        item::armor::ArmorMaterials::IRON,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::IRON.getDurability(item::armor::ArmorSlot::Head)));
    player.inventory().setHelmet(ItemStack(equippedHelmet));

    ItemActionResult result = armorItem.onItemRightClick(world, player, Hand::MainHand);

    EXPECT_TRUE(result.isPass());
    EXPECT_FALSE(player.getHeldItem(Hand::MainHand).isEmpty());
    EXPECT_EQ(player.inventory().getHelmet().getItem(), &equippedHelmet);
    EXPECT_EQ(result.getResult().getItem(), &armorItem);
}

TEST_F(PlayerInventoryTest, OffhandSlot) {
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 5);
    m_inventory->setOffhandItem(stack);

    EXPECT_EQ(m_inventory->getOffhandItem().getCount(), 5);
    EXPECT_EQ(m_inventory->getItem(InventorySlots::OFFHAND).getCount(), 5);
}

TEST_F(PlayerInventoryTest, SerializationEmpty) {
    network::PacketSerializer ser;
    m_inventory->serialize(ser);

    const std::vector<u8>& data = ser.buffer();
    EXPECT_GT(data.size(), 0);

    network::PacketDeserializer deser(data);
    auto result = PlayerInventory::deserialize(deser);
    EXPECT_TRUE(result.success());

    PlayerInventory& loaded = result.value();
    EXPECT_TRUE(loaded.isEmpty());
    EXPECT_EQ(loaded.getSelectedSlot(), 0);
}

TEST_F(PlayerInventoryTest, SerializationWithItems) {
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    // 设置一些物品
    m_inventory->setItem(0, ItemStack(*m_diamond, 32));
    m_inventory->setItem(5, ItemStack(*m_stick, 16));
    m_inventory->setItem(40, ItemStack(*m_diamond, 8));
    m_inventory->setSelectedSlot(3);

    // 序列化
    network::PacketSerializer ser;
    m_inventory->serialize(ser);

    // 反序列化
    network::PacketDeserializer deser(ser.buffer());
    auto result = PlayerInventory::deserialize(deser);
    EXPECT_TRUE(result.success());

    PlayerInventory& loaded = result.value();
    EXPECT_EQ(loaded.getSelectedSlot(), 3);
    EXPECT_EQ(loaded.getItem(0).getCount(), 32);
    EXPECT_EQ(loaded.getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loaded.getItem(5).getCount(), 16);
    EXPECT_EQ(loaded.getItem(5).getItem(), m_stick);
    EXPECT_EQ(loaded.getItem(40).getCount(), 8);
}

TEST_F(PlayerInventoryTest, DamageableItemStacking) {
    ASSERT_NE(m_diamondSword, nullptr);

    // 有耐久度的物品堆叠数为1
    ItemStack sword(*m_diamondSword, 1);
    EXPECT_EQ(sword.getMaxStackSize(), 1);

    // 两把剑不能合并
    m_inventory->setItem(0, sword);
    ItemStack anotherSword(*m_diamondSword, 1);
    EXPECT_FALSE(m_inventory->getItem(0).canMergeWith(anotherSword));
}

TEST_F(PlayerInventoryTest, IsHotbar) {
    EXPECT_TRUE(PlayerInventory::isHotbar(0));
    EXPECT_TRUE(PlayerInventory::isHotbar(4));
    EXPECT_TRUE(PlayerInventory::isHotbar(8));
    EXPECT_FALSE(PlayerInventory::isHotbar(9));
    EXPECT_FALSE(PlayerInventory::isHotbar(-1));
    EXPECT_FALSE(PlayerInventory::isHotbar(40));
}

TEST_F(PlayerInventoryTest, GetBestHotbarSlot) {
    ASSERT_NE(m_diamond, nullptr);

    // 空背包，返回第一个槽位
    EXPECT_EQ(m_inventory->getBestHotbarSlot(), 0);

    // 填充一些槽位
    m_inventory->setItem(0, ItemStack(*m_diamond, 1));
    m_inventory->setItem(1, ItemStack(*m_diamond, 1));
    m_inventory->setItem(3, ItemStack(*m_diamond, 1));

    // 应该返回第一个空槽位
    EXPECT_EQ(m_inventory->getBestHotbarSlot(), 2);
}

// ============================================================================
// Slot 测试
// ============================================================================

class SlotTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    }

    std::unique_ptr<PlayerInventory> m_inventory;
    Item* m_diamond = nullptr;
};

TEST_F(SlotTest, BasicOperations) {
    ASSERT_NE(m_diamond, nullptr);

    Slot slot(m_inventory.get(), 0, 10, 20);

    EXPECT_EQ(slot.getIndex(), 0);
    EXPECT_EQ(slot.getX(), 10);
    EXPECT_EQ(slot.getY(), 20);
    EXPECT_TRUE(slot.isEmpty());

    // 设置物品
    ItemStack stack(*m_diamond, 32);
    m_inventory->setItem(0, stack);

    EXPECT_FALSE(slot.isEmpty());
    EXPECT_EQ(slot.getItem().getCount(), 32);

    // 移除物品
    ItemStack removed = slot.remove(10);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_EQ(slot.getItem().getCount(), 22);
}

TEST_F(SlotTest, MaxStackSize) {
    ASSERT_NE(m_diamond, nullptr);

    Slot slot(m_inventory.get(), 0, 0, 0);
    ItemStack stack(*m_diamond, 1);

    EXPECT_EQ(slot.getMaxStackSize(), 64);
    EXPECT_EQ(slot.getMaxStackSize(stack), 64);
}

TEST_F(SlotTest, MayPlace) {
    Slot slot(m_inventory.get(), 0, 0, 0);

    EXPECT_TRUE(slot.mayPlace(ItemStack::EMPTY));
}

TEST_F(SlotTest, ArmorSlotOnlyAcceptsMatchingArmorType) {
    auto makeArmorItem = [](const item::armor::ArmorMaterial& material,
                            item::armor::ArmorSlot slot) {
        return item::items::ArmorItem(
            material,
            slot,
            ItemProperties().maxDamage(material.getDurability(slot)));
    };

    const auto helmet = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Head);
    const auto chestplate = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Chest);
    const auto leggings = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Legs);
    const auto boots = makeArmorItem(item::armor::ArmorMaterials::IRON, item::armor::ArmorSlot::Feet);

    ArmorSlot headSlot(m_inventory.get(), InventorySlots::ARMOR_HEAD, 0, 0, ArmorSlot::ArmorType::Head);
    ArmorSlot chestSlot(m_inventory.get(), InventorySlots::ARMOR_CHEST, 0, 0, ArmorSlot::ArmorType::Chest);
    ArmorSlot legsSlot(m_inventory.get(), InventorySlots::ARMOR_LEGS, 0, 0, ArmorSlot::ArmorType::Legs);
    ArmorSlot feetSlot(m_inventory.get(), InventorySlots::ARMOR_FEET, 0, 0, ArmorSlot::ArmorType::Feet);

    EXPECT_TRUE(headSlot.mayPlace(ItemStack(helmet)));
    EXPECT_FALSE(headSlot.mayPlace(ItemStack(chestplate)));
    EXPECT_FALSE(headSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(chestSlot.mayPlace(ItemStack(chestplate)));
    EXPECT_FALSE(chestSlot.mayPlace(ItemStack(leggings)));
    EXPECT_FALSE(chestSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(legsSlot.mayPlace(ItemStack(leggings)));
    EXPECT_FALSE(legsSlot.mayPlace(ItemStack(boots)));
    EXPECT_FALSE(legsSlot.mayPlace(ItemStack(*m_diamond)));

    EXPECT_TRUE(feetSlot.mayPlace(ItemStack(boots)));
    EXPECT_FALSE(feetSlot.mayPlace(ItemStack(helmet)));
    EXPECT_FALSE(feetSlot.mayPlace(ItemStack(*m_diamond)));
}

TEST(ArmorItemTest, TotalArmorStatsSumAllEquippedPieces) {
    TestLivingEntity entity;

    const item::items::ArmorItem helmet(
        item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Head,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Head)));
    const item::items::ArmorItem chestplate(
        item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Chest,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Chest)));
    const item::items::ArmorItem leggings(
        item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Legs,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Legs)));
    const item::items::ArmorItem boots(
        item::armor::ArmorMaterials::NETHERITE,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::NETHERITE.getDurability(item::armor::ArmorSlot::Feet)));

    entity.setEquipment(EquipmentSlot::Head, ItemStack(helmet));
    entity.setEquipment(EquipmentSlot::Chest, ItemStack(chestplate));
    entity.setEquipment(EquipmentSlot::Legs, ItemStack(leggings));
    entity.setEquipment(EquipmentSlot::Feet, ItemStack(boots));

    EXPECT_EQ(item::items::ArmorItem::getTotalArmorValue(entity), 20);
    EXPECT_FLOAT_EQ(item::items::ArmorItem::getTotalToughness(entity), 12.0f);
    EXPECT_FLOAT_EQ(item::items::ArmorItem::getTotalKnockbackResistance(entity), 0.4f);
}

TEST(DyeableArmorItemTest, ColorRoundTripUsesDisplayTag) {
    const item::items::DyeableArmorItem leatherBoots(
        item::armor::ArmorMaterials::LEATHER,
        item::armor::ArmorSlot::Feet,
        ItemProperties().maxDamage(item::armor::ArmorMaterials::LEATHER.getDurability(item::armor::ArmorSlot::Feet)));

    ItemStack stack(leatherBoots);
    EXPECT_FALSE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0xA06540u);

    item::items::DyeableArmorItem::setColor(stack, 0x123456u);
    EXPECT_TRUE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0x123456u);
    ASSERT_NE(stack.getChildTag("display"), nullptr);
    const int storedColor = (*stack.getChildTag("display"))["color"].get<int>();
    EXPECT_EQ(storedColor, 0x123456);

    item::items::DyeableArmorItem::clearColor(stack);
    EXPECT_FALSE(item::items::DyeableArmorItem::hasColor(stack));
    EXPECT_EQ(leatherBoots.getColor(stack), 0xA06540u);
    EXPECT_FALSE(stack.hasTag());
}

TEST(ElytraItemTest, RightClickEquipsChestSlot) {
    ArmorTestWorld world;
    Player player(3, "elytra-test");

    item::items::ElytraItem elytra{ItemProperties()};
    player.inventory().setItem(0, ItemStack(elytra));

    ItemActionResult result = elytra.onItemRightClick(world, player, Hand::MainHand);

    EXPECT_TRUE(result.isConsume());
    EXPECT_TRUE(result.getResult().isEmpty());
    EXPECT_TRUE(player.getHeldItem(Hand::MainHand).isEmpty());
    EXPECT_EQ(player.inventory().getChestplate().getItem(), &elytra);
    EXPECT_EQ(player.inventory().getChestplate().getCount(), 1);
}

TEST(ElytraItemTest, InventoryTickDamagesOnlyWhenGlidingInChestSlot) {
    ArmorTestWorld world;
    world.setCurrentTick(20);

    TestLivingEntity entity;
    entity.setPose(EntityPose::FallFlying);

    item::items::ElytraItem elytra{ItemProperties()};
    ItemStack chestElytra(elytra);
    elytra.inventoryTick(chestElytra, world, entity, InventorySlots::ARMOR_CHEST, false);

    EXPECT_EQ(chestElytra.getDamage(), 1);

    ItemStack carriedElytra(elytra);
    elytra.inventoryTick(carriedElytra, world, entity, 0, false);

    EXPECT_EQ(carriedElytra.getDamage(), 0);
}

// ============================================================================
// IInventory 接口测试
// ============================================================================

class IInventoryInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
        m_inventory = std::make_unique<PlayerInventory>(nullptr);
    }

    std::unique_ptr<PlayerInventory> m_inventory;
};

TEST_F(IInventoryInterfaceTest, HasAny_WorksWithIInventory) {
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond};

    EXPECT_FALSE(inv->hasAny(items));

    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_EmptySet) {
    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> empty;
    EXPECT_FALSE(inv->hasAny(empty));
}

TEST_F(IInventoryInterfaceTest, HasAny_MultipleItems) {
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    ASSERT_NE(diamond, nullptr);
    ASSERT_NE(coal, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond, coal};

    // 空背包不包含任何物品
    EXPECT_FALSE(inv->hasAny(items));

    // 添加钻石
    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));

    // 清空后添加煤炭
    m_inventory->clear();
    m_inventory->setItem(5, ItemStack(*coal, 5));
    EXPECT_TRUE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_AfterPartialRemove) {
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    std::unordered_set<const Item*> items = {diamond};

    // 添加物品然后部分移除
    m_inventory->setItem(0, ItemStack(*diamond, 10));
    EXPECT_TRUE(inv->hasAny(items));

    // 部分移除
    m_inventory->removeItem(0, 5);
    EXPECT_TRUE(inv->hasAny(items));

    // 完全移除
    m_inventory->removeItem(0, 5);
    EXPECT_FALSE(inv->hasAny(items));
}

TEST_F(IInventoryInterfaceTest, HasAny_WithNullItemInSet) {
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    IInventory* inv = m_inventory.get();
    // 集合中包含空指针（边界情况）
    std::unordered_set<const Item*> items = {diamond, nullptr};

    m_inventory->setItem(0, ItemStack(*diamond, 10));
    // 应该仍然能找到钻石
    EXPECT_TRUE(inv->hasAny(items));
}

// ============================================================================
// FurnaceFuelSlot 边界测试
// ============================================================================

class FurnaceFuelSlotEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }

    void TearDown() override {
        // 确保清理
    }
};

TEST_F(FurnaceFuelSlotEdgeCaseTest, MayPlace_EmptyStack) {
    blockentity::SimpleInventory inventory(3);
    FurnaceFuelSlot slot(&inventory, 0, 10, 10);

    // 空物品堆
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, GetMaxStackSize_EmptyStack) {
    blockentity::SimpleInventory inventory(3);
    FurnaceFuelSlot slot(&inventory, 0, 10, 10);

    // 空物品堆的堆叠上限
    ItemStack emptyStack;
    EXPECT_EQ(slot.getMaxStackSize(emptyStack), 64);
}

TEST_F(FurnaceFuelSlotEdgeCaseTest, SetAndGetItem) {
    blockentity::SimpleInventory inventory(3);
    FurnaceFuelSlot slot(&inventory, 0, 10, 10);

    Item* coal = ItemRegistry::instance().getItem(ResourceLocation("minecraft:coal"));
    if (coal == nullptr) {
        GTEST_SKIP() << "Coal not registered";
    }

    // 设置物品到槽位
    ItemStack stack(*coal, 32);
    inventory.setItem(0, stack);

    EXPECT_FALSE(slot.isEmpty());
    EXPECT_EQ(slot.getItem().getCount(), 32);
}
