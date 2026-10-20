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
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>
#include <vector>

using namespace mc;

// 装备拾取链路测试：对齐 MC Java 1.21.11 Mob.pickUpItem → equipItemIfPossible →
// canReplaceCurrentItem / canReplaceEqualItem / getEquipmentSlotForItem / isEquippableInSlot。
namespace {

class PickupTestMobEntity : public MobEntity {
public:
    PickupTestMobEntity()
        : MobEntity(EntityInstanceId(1), mc::test::testEcsRegistry())
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

class PickupTestWorld final : public mc::test::BaseTestWorld {
public:
    PickupTestWorld() { Items::initialize(); }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size() + 100);
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Entity>>& spawnedEntities() const { return m_spawnedEntities; }

private:
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

class EquipItemIfPossibleTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            VanillaBlocks::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<PickupTestWorld>();
        m_entity = std::make_unique<PickupTestMobEntity>();
        m_entity->setWorld(m_world.get());
        m_entity->setPosition(0.0, 64.0, 0.0);
    }

    /// 构造物品实体（ItemEntity 不可拷贝/移动，故以 unique_ptr 返回）。
    std::unique_ptr<ItemEntity> makeItemEntity(const ItemStack& stack) const
    {
        auto item =
            std::make_unique<ItemEntity>(EntityInstanceId(500), stack, 0.0f, 64.0f, 0.0f, mc::test::testEcsRegistry());
        item->setWorld(m_world.get());
        return item;
    }

    std::unique_ptr<PickupTestWorld> m_world;
    std::unique_ptr<PickupTestMobEntity> m_entity;
};

} // namespace

// 盔甲物品映射到对应的护甲槽位，非盔甲物品（工具、方块）映射到主手。
TEST_F(EquipItemIfPossibleTest, GetEquipmentSlotForItem_MapsArmorAndMainHand)
{
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::IRON_HELMET, 1)), EquipmentSlot::Head);
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::IRON_CHESTPLATE, 1)), EquipmentSlot::Chest);
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::IRON_LEGGINGS, 1)), EquipmentSlot::Legs);
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::IRON_BOOTS, 1)), EquipmentSlot::Feet);
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::IRON_SWORD, 1)), EquipmentSlot::MainHand);
    EXPECT_EQ(m_entity->getEquipmentSlotForItem(ItemStack(Items::DIRT, 1)), EquipmentSlot::MainHand);
}

// 非盔甲物品只能进主手，盔甲只能进其自身槽位。
TEST_F(EquipItemIfPossibleTest, IsEquippableInSlot_RejectsMismatchedSlots)
{
    const ItemStack helmet(Items::IRON_HELMET, 1);
    EXPECT_TRUE(m_entity->isEquippableInSlot(helmet, EquipmentSlot::Head));
    EXPECT_FALSE(m_entity->isEquippableInSlot(helmet, EquipmentSlot::MainHand));
    EXPECT_FALSE(m_entity->isEquippableInSlot(helmet, EquipmentSlot::Chest));

    const ItemStack sword(Items::IRON_SWORD, 1);
    EXPECT_TRUE(m_entity->isEquippableInSlot(sword, EquipmentSlot::MainHand));
    EXPECT_FALSE(m_entity->isEquippableInSlot(sword, EquipmentSlot::Head));
}

// 空槽位时物品直接装备，并标记为保整掉落与持久化。
TEST_F(EquipItemIfPossibleTest, EquipIntoEmptySlot_SetsGuaranteedDropAndPersistence)
{
    const ItemStack result = m_entity->equipItemIfPossible(ItemStack(Items::IRON_HELMET, 1));
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.getItem(), Items::IRON_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
    EXPECT_TRUE(m_entity->isEquipmentDropPreserved(EquipmentSlot::Head));
    EXPECT_TRUE(m_entity->isNoDespawnRequired());
}

// 护甲槽固定只能容纳 1 件，物品堆会被裁剪，剩余数量由调用方保留。
TEST_F(EquipItemIfPossibleTest, EquipArmor_LimitsToSingleItem)
{
    const ItemStack result = m_entity->equipItemIfPossible(ItemStack(Items::IRON_HELMET, 5));
    ASSERT_FALSE(result.isEmpty());
    EXPECT_EQ(result.getCount(), 1);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getCount(), 1);
}

// 护甲值更高的装备可以替换当前装备；护甲值更低时不可替换。
TEST_F(EquipItemIfPossibleTest, EquipArmor_ReplacesWeakerArmorOnly)
{
    ASSERT_FALSE(m_entity->equipItemIfPossible(ItemStack(Items::LEATHER_HELMET, 1)).isEmpty());

    const ItemStack better = m_entity->equipItemIfPossible(ItemStack(Items::IRON_HELMET, 1));
    EXPECT_EQ(better.getItem(), Items::IRON_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);

    // 较弱的护甲无法替换头盔，但主手为空时仍会退入主手（vanilla fallback 分支），
    // 因此此处显式占住主手以验证"不可替换"语义。
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::STONE_SWORD, 1));
    const ItemStack worse = m_entity->equipItemIfPossible(ItemStack(Items::LEATHER_HELMET, 1));
    EXPECT_TRUE(worse.isEmpty());
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::MainHand).getItem(), Items::STONE_SWORD);
}

// 护甲不可替换且主手为空时退回主手（vanilla equipItemIfPossible 的 fallback 分支）。
TEST_F(EquipItemIfPossibleTest, EquipArmor_FallsBackToEmptyMainHand)
{
    ASSERT_FALSE(m_entity->equipItemIfPossible(ItemStack(Items::IRON_HELMET, 1)).isEmpty());

    const ItemStack fallback = m_entity->equipItemIfPossible(ItemStack(Items::LEATHER_HELMET, 1));
    ASSERT_FALSE(fallback.isEmpty());
    EXPECT_EQ(fallback.getItem(), Items::LEATHER_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::MainHand).getItem(), Items::LEATHER_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
}

// 主手比较攻击伤害：更高伤害的武器可以替换，更低伤害的不行。
TEST_F(EquipItemIfPossibleTest, EquipWeapon_ComparesAttackDamage)
{
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::DIAMOND_SWORD, 1));

    // 铁剑伤害低于钻石剑：不可替换
    EXPECT_TRUE(m_entity->equipItemIfPossible(ItemStack(Items::IRON_SWORD, 1)).isEmpty());

    // 铁斧伤害高于钻石剑：可替换
    const ItemStack axe = m_entity->equipItemIfPossible(ItemStack(Items::IRON_AXE, 1));
    ASSERT_FALSE(axe.isEmpty());
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::MainHand).getItem(), Items::IRON_AXE);

    EXPECT_TRUE(m_entity->equipItemIfPossible(ItemStack(Items::WOODEN_SWORD, 1)).isEmpty());
}

// 攻击伤害相同时按耐久度比较：剩余耐久更多（伤害值更小）者胜出。
TEST_F(EquipItemIfPossibleTest, CanReplaceEqualItem_PrefersLessDamagedStack)
{
    const ItemStack candidate(Items::IRON_SWORD, 1);
    ItemStack current(Items::IRON_SWORD, 1);
    current.setDamage(10);

    EXPECT_TRUE(m_entity->canReplaceEqualItem(candidate, current));
    EXPECT_FALSE(m_entity->canReplaceEqualItem(current, candidate));

    // 附魔数量更多者优先
    ItemStack enchanted(Items::IRON_SWORD, 1);
    enchanted.addEnchantment("minecraft:sharpness", 1);
    EXPECT_TRUE(m_entity->canReplaceEqualItem(enchanted, current));
    EXPECT_FALSE(m_entity->canReplaceEqualItem(current, enchanted));
}

// pickUpItem 走完整链路：主手可堆叠物品被拾取后物品实体数量减少，耗尽时被移除。
TEST_F(EquipItemIfPossibleTest, PickUpItem_ShrinksAndRemovesEmptyItemEntity)
{
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::STONE_SWORD, 1));

    auto item = makeItemEntity(ItemStack(Items::STONE_SWORD, 3));
    m_entity->pickUpItem(*item);

    // 同种武器攻击伤害与耐久相同，canReplaceEqualItem 判定为不替换，主手保持原物品。
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::MainHand).getCount(), 1);
    EXPECT_FALSE(item->isRemoved());

    // 换成更高伤害的武器后，物品实体被完整吸收并移除。
    auto betterItem = makeItemEntity(ItemStack(Items::IRON_AXE, 1));
    m_entity->pickUpItem(*betterItem);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::MainHand).getItem(), Items::IRON_AXE);
    EXPECT_TRUE(betterItem->getItemStack().isEmpty());
    EXPECT_TRUE(betterItem->isRemoved());
}

// 护甲槽容量为 1：拾取多件头盔时只吸收 1 件，其余数量保留在物品实体上。
TEST_F(EquipItemIfPossibleTest, PickUpItem_LimitsArmorToSingleItem)
{
    auto item = makeItemEntity(ItemStack(Items::IRON_HELMET, 3));
    m_entity->pickUpItem(*item);

    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getItem(), Items::IRON_HELMET);
    EXPECT_EQ(m_entity->getEquipment(EquipmentSlot::Head).getCount(), 1);
    ASSERT_FALSE(item->isRemoved());
    EXPECT_EQ(item->getItemStack().getCount(), 2);
}
