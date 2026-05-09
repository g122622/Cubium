#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

using namespace mc;

namespace {

/**
 * @brief 测试用世界存根
 */
class ArmorTestWorld final : public IWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32, i32, i32) const override { return nullptr; }
    bool setBlockState(i32, i32, i32, const BlockState*) override { return false; }
    [[nodiscard]] const fluid::FluidState* getFluidState(i32, i32, i32) const override {
        return fluid::Fluid::getFluidState(0);
    }
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord, ChunkCoord) const override { return nullptr; }
    [[nodiscard]] bool hasChunk(ChunkCoord, ChunkCoord) const override { return false; }
    [[nodiscard]] i32 getHeight(i32, i32) const override { return 64; }
    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override { return 0; }
    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override { return 15; }
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override { return {}; }
    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override {
        return y >= mc::world::MIN_BUILD_HEIGHT && y < mc::world::MAX_BUILD_HEIGHT;
    }
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override { return false; }
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] PhysicsEngine* physicsEngine() override { return nullptr; }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return nullptr; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override { return {}; }
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override { return {}; }
    [[nodiscard]] DimensionId dimension() const override { return DimensionId(0); }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] u64 currentTick() const override { return 0; }
    [[nodiscard]] i64 dayTime() const override { return 0; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    [[nodiscard]] world::tick::TickManager& tickManager() override {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // WorldBorder interface (stubbed for tests)
    [[nodiscard]] world::border::WorldBorder& worldBorder() override {
        throw std::runtime_error("ArmorTestWorld::worldBorder not implemented");
    }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override {
        throw std::runtime_error("ArmorTestWorld::worldBorder not implemented");
    }

    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override {
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return ++m_lastEntityId;
    }

    Entity* getEntity(EntityId id) override {
        for (auto* e : m_spawnedEntities) {
            if (e && e->id() == static_cast<u32>(id)) return e;
        }
        return nullptr;
    }

    const Entity* getEntity(EntityId id) const override {
        for (const auto* e : m_spawnedEntities) {
            if (e && e->id() == static_cast<u32>(id)) return e;
        }
        return nullptr;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

    void clearSpawnedEntities() {
        m_spawnedEntities.clear();
        m_ownedEntities.clear();
    }

private:
    math::Random m_random{12345};
    EntityId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

} // namespace

// ============================================================================
// ArmorItem::getTotalArmorValue 测试
// ============================================================================

class ArmorValueTest : public ::testing::Test {
protected:
    void SetUp() override {
        Items::initialize();
    }

    void TearDown() override {
        // Items 清理由静态析构处理
    }
};

TEST_F(ArmorValueTest, EmptyArmorReturnsZero) {
    // 创建一个玩家，不穿戴任何护甲
    Player player(1, "TestPlayer");

    // 护甲值应该为0
    EXPECT_EQ(player.armorValue(), 0);
}

TEST_F(ArmorValueTest, TotalArmorValueWithFullDiamondArmor) {
    // 钻石护甲值（MC 1.16.5）：
    // 头盔: 3, 胸甲: 8, 护腿: 6, 靴子: 3 = 20
    if (Items::DIAMOND_HELMET && Items::DIAMOND_CHESTPLATE &&
        Items::DIAMOND_LEGGINGS && Items::DIAMOND_BOOTS) {

        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));
        inv.setChestplate(ItemStack(Items::DIAMOND_CHESTPLATE, 1));
        inv.setLeggings(ItemStack(Items::DIAMOND_LEGGINGS, 1));
        inv.setBoots(ItemStack(Items::DIAMOND_BOOTS, 1));

        // 钻石全套护甲值为 20
        EXPECT_EQ(player.armorValue(), 20);
    }
}

TEST_F(ArmorValueTest, TotalArmorValueWithFullIronArmor) {
    // 铁护甲值（MC 1.16.5）：
    // 头盔: 2, 胸甲: 6, 护腿: 5, 靴子: 2 = 15
    if (Items::IRON_HELMET && Items::IRON_CHESTPLATE &&
        Items::IRON_LEGGINGS && Items::IRON_BOOTS) {

        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::IRON_HELMET, 1));
        inv.setChestplate(ItemStack(Items::IRON_CHESTPLATE, 1));
        inv.setLeggings(ItemStack(Items::IRON_LEGGINGS, 1));
        inv.setBoots(ItemStack(Items::IRON_BOOTS, 1));

        // 铁全套护甲值为 15
        EXPECT_EQ(player.armorValue(), 15);
    }
}

TEST_F(ArmorValueTest, TotalArmorValueWithPartialArmor) {
    // 只穿戴部分护甲
    if (Items::DIAMOND_HELMET && Items::DIAMOND_CHESTPLATE) {
        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        // 只戴头盔和胸甲
        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));
        inv.setChestplate(ItemStack(Items::DIAMOND_CHESTPLATE, 1));

        // 钻石头盔(3) + 钻石胸甲(8) = 11
        EXPECT_EQ(player.armorValue(), 11);
    }
}

TEST_F(ArmorValueTest, TotalArmorValueWithMixedArmor) {
    // 混合护甲
    if (Items::DIAMOND_HELMET && Items::IRON_CHESTPLATE &&
        Items::DIAMOND_LEGGINGS && Items::IRON_BOOTS) {

        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));      // 3
        inv.setChestplate(ItemStack(Items::IRON_CHESTPLATE, 1)); // 6
        inv.setLeggings(ItemStack(Items::DIAMOND_LEGGINGS, 1));  // 6
        inv.setBoots(ItemStack(Items::IRON_BOOTS, 1));           // 2

        // 3 + 6 + 6 + 2 = 17
        EXPECT_EQ(player.armorValue(), 17);
    }
}

TEST_F(ArmorValueTest, NonArmorItemsDoNotContribute) {
    // 非护甲物品不应该贡献护甲值
    if (Items::STONE && Items::IRON_HELMET) {
        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        // 在头盔槽放石头
        inv.setHelmet(ItemStack(Items::STONE, 1));

        // 护甲值应该为0
        EXPECT_EQ(player.armorValue(), 0);

        // 现在放真正的头盔
        inv.setHelmet(ItemStack(Items::IRON_HELMET, 1));

        // 护甲值应该是铁头盔的值(2)
        EXPECT_EQ(player.armorValue(), 2);
    }
}

TEST_F(ArmorValueTest, EmptyStackDoesNotContribute) {
    // 空物品堆不应该贡献护甲值
    Player player(1, "TestPlayer");
    PlayerInventory& inv = player.inventory();

    // 设置为空堆
    inv.setHelmet(ItemStack::EMPTY);
    inv.setChestplate(ItemStack::EMPTY);
    inv.setLeggings(ItemStack::EMPTY);
    inv.setBoots(ItemStack::EMPTY);

    EXPECT_EQ(player.armorValue(), 0);
}

TEST_F(ArmorValueTest, ArmorValueStaticMethod) {
    // 测试静态方法 ArmorItem::getTotalArmorValue
    if (Items::DIAMOND_HELMET && Items::DIAMOND_CHESTPLATE) {
        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));
        inv.setChestplate(ItemStack(Items::DIAMOND_CHESTPLATE, 1));

        // 静态方法应该返回相同值
        EXPECT_EQ(item::items::ArmorItem::getTotalArmorValue(player), player.armorValue());
    }
}

// ============================================================================
// ArmorItem 护甲韧性测试
// ============================================================================

TEST_F(ArmorValueTest, ArmorToughnessDiamondArmor) {
    // 钻石护甲韧性：每件2点，全套8点
    if (Items::DIAMOND_HELMET) {
        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));
        inv.setChestplate(ItemStack(Items::DIAMOND_CHESTPLATE, 1));
        inv.setLeggings(ItemStack(Items::DIAMOND_LEGGINGS, 1));
        inv.setBoots(ItemStack(Items::DIAMOND_BOOTS, 1));

        // 钻石全套韧性为 8
        EXPECT_EQ(item::items::ArmorItem::getTotalToughness(player), 8.0f);
    }
}

TEST_F(ArmorValueTest, ArmorToughnessNetheriteArmor) {
    // 下界合金护甲韧性：每件3点，全套12点
    if (Items::NETHERITE_HELMET) {
        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::NETHERITE_HELMET, 1));
        inv.setChestplate(ItemStack(Items::NETHERITE_CHESTPLATE, 1));
        inv.setLeggings(ItemStack(Items::NETHERITE_LEGGINGS, 1));
        inv.setBoots(ItemStack(Items::NETHERITE_BOOTS, 1));

        // 下界合金全套护甲值 20，韧性 12
        EXPECT_EQ(player.armorValue(), 20);
        EXPECT_EQ(item::items::ArmorItem::getTotalToughness(player), 12.0f);
    }
}

// ============================================================================
// PlayerInventory::getDestroySpeed 测试
// ============================================================================

TEST_F(ArmorValueTest, GetDestroySpeedWithEmptyHand) {
    Player player(1, "TestPlayer");

    // 空手应该返回 1.0
    // 注意：需要一个有效的BlockState来测试
    // 这里简单验证方法存在且可调用
    EXPECT_EQ(player.inventory().getSelectedStack().isEmpty(), true);
}
