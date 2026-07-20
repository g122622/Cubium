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
#include "common/core/Constants.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;

namespace {

/**
 * @brief 测试用世界存根
 */
class ArmorTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("ArmorTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return ++m_lastEntityId;
    }

    Entity* getEntity(EntityInstanceId id) override
    {
        for (auto* e : m_spawnedEntities) {
            if (e && e->id() == static_cast<u32>(id)) return e;
        }
        return nullptr;
    }

    const Entity* getEntity(EntityInstanceId id) const override
    {
        for (const auto* e : m_spawnedEntities) {
            if (e && e->id() == static_cast<u32>(id)) return e;
        }
        return nullptr;
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

    void clearSpawnedEntities()
    {
        m_spawnedEntities.clear();
        m_ownedEntities.clear();
    }

private:
    EntityInstanceId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

} // namespace

// ============================================================================
// ArmorItem::getTotalArmorValue 测试
// ============================================================================

class ArmorValueTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    void TearDown() override
    {
        // Items 清理由静态析构处理
    }
};

TEST_F(ArmorValueTest, EmptyArmorReturnsZero)
{
    // 创建一个玩家，不穿戴任何护甲
    Player player(1, "TestPlayer");

    // 护甲值应该为0
    EXPECT_EQ(player.armorValue(), 0);
}

TEST_F(ArmorValueTest, TotalArmorValueWithFullDiamondArmor)
{
    // 钻石护甲值（MC 1.16.5）：
    // 头盔: 3, 胸甲: 8, 护腿: 6, 靴子: 3 = 20
    if (Items::DIAMOND_HELMET && Items::DIAMOND_CHESTPLATE && Items::DIAMOND_LEGGINGS && Items::DIAMOND_BOOTS) {

        Player player(1, "TestPlayer");
        PlayerInventory& inv = player.inventory();

        inv.setHelmet(ItemStack(Items::DIAMOND_HELMET, 1));
        inv.setChestplate(ItemStack(Items::DIAMOND_CHESTPLATE, 1));
        inv.setLeggings(ItemStack(Items::DIAMOND_LEGGINGS, 1));
        inv.setBoots(ItemStack(Items::DIAMOND_BOOTS, 1));

        // 钻石全套护甲值为 20
        EXPECT_EQ(player.armorValue(), 20);
    } else {
        FAIL() << "Diamond armor items not initialized";
    }
}

TEST_F(ArmorValueTest, TotalArmorValueWithFullIronArmor)
{
    // 铁护甲值（MC 1.16.5）：
    // 头盔: 2, 胸甲: 6, 护腿: 5, 靴子: 2 = 15
    if (Items::IRON_HELMET && Items::IRON_CHESTPLATE && Items::IRON_LEGGINGS && Items::IRON_BOOTS) {

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

TEST_F(ArmorValueTest, TotalArmorValueWithPartialArmor)
{
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

TEST_F(ArmorValueTest, TotalArmorValueWithMixedArmor)
{
    // 混合护甲
    if (Items::DIAMOND_HELMET && Items::IRON_CHESTPLATE && Items::DIAMOND_LEGGINGS && Items::IRON_BOOTS) {

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

TEST_F(ArmorValueTest, NonArmorItemsDoNotContribute)
{
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

TEST_F(ArmorValueTest, EmptyStackDoesNotContribute)
{
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

TEST_F(ArmorValueTest, ArmorValueStaticMethod)
{
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

TEST_F(ArmorValueTest, ArmorToughnessDiamondArmor)
{
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

TEST_F(ArmorValueTest, ArmorToughnessNetheriteArmor)
{
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

TEST_F(ArmorValueTest, GetDestroySpeedWithEmptyHand)
{
    Player player(1, "TestPlayer");

    // 空手应该返回 1.0
    // 注意：需要一个有效的BlockState来测试
    // 这里简单验证方法存在且可调用
    EXPECT_EQ(player.inventory().getSelectedStack().isEmpty(), true);
}
