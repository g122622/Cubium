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

#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "item/items/block/BlockItemRegistry.hpp"

using namespace mc;
using namespace mc::entity;
using namespace mc::item::tag;

// ============================================================================
// 测试固定装置
// ============================================================================

class ItemEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化顺序：方块 -> 物品 -> 方块物品 -> 物品标签
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        ItemTags::initialize();
    }

    void SetUp() override {}
};

// ============================================================================
// FIRE_RESISTANT 标签测试
// ============================================================================

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteIngot)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    EXPECT_TRUE(netheriteIngot->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteScrap)
{
    Item* scrap = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_scrap"));
    ASSERT_NE(scrap, nullptr);
    EXPECT_TRUE(scrap->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsAncientDebris)
{
    Item* debris = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "ancient_debris"));
    ASSERT_NE(debris, nullptr);
    EXPECT_TRUE(debris->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetherStar)
{
    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(netherStar, nullptr);
    EXPECT_TRUE(netherStar->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteSword)
{
    Item* sword = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_sword"));
    ASSERT_NE(sword, nullptr);
    EXPECT_TRUE(sword->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheritePickaxe)
{
    Item* pickaxe = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_pickaxe"));
    ASSERT_NE(pickaxe, nullptr);
    EXPECT_TRUE(pickaxe->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteHelmet)
{
    Item* helmet = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_helmet"));
    ASSERT_NE(helmet, nullptr);
    EXPECT_TRUE(helmet->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteChestplate)
{
    Item* chestplate = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_chestplate"));
    ASSERT_NE(chestplate, nullptr);
    EXPECT_TRUE(chestplate->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagContainsNetheriteHorseArmor)
{
    Item* armor = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_horse_armor"));
    ASSERT_NE(armor, nullptr);
    EXPECT_TRUE(armor->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagDoesNotContainIronIngot)
{
    Item* ironIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    ASSERT_NE(ironIngot, nullptr);
    EXPECT_FALSE(ironIngot->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagDoesNotContainDiamond)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_FALSE(diamond->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagDoesNotContainStone)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(stone->isIn(ItemTags::FIRE_RESISTANT()));
}

TEST_F(ItemEntityTest, FireResistantTagIdIsCorrect)
{
    EXPECT_EQ(ItemTags::FIRE_RESISTANT().getId(), ResourceLocation("minecraft", "fire_resistant"));
}

// ============================================================================
// ItemStack::canBeHurtBy 测试
// ============================================================================

TEST_F(ItemEntityTest, CanBeHurtBy_NormalItemCanBeHurtByFire)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_TRUE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NormalItemCanBeHurtByLava)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto lavaDamage = DamageSources::lava();
    EXPECT_TRUE(stack.canBeHurtBy(lavaDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NormalItemCanBeHurtByGenericDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemStack stack(*stone, 1);
    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(stack.canBeHurtBy(genericDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NetheriteItemCannotBeHurtByFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NetheriteItemCannotBeHurtByLava)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    auto lavaDamage = DamageSources::lava();
    EXPECT_FALSE(stack.canBeHurtBy(lavaDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NetheriteItemCanBeHurtByGenericDamage)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemStack stack(*netheriteIngot, 1);
    // 防火物品只免疫火焰伤害源，不免疫其他伤害源
    auto genericDamage = DamageSources::generic();
    EXPECT_TRUE(stack.canBeHurtBy(genericDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_NetherStarCannotBeHurtByFire)
{
    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(netherStar, nullptr);
    ItemStack stack(*netherStar, 1);
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(stack.canBeHurtBy(fireDamage));
}

TEST_F(ItemEntityTest, CanBeHurtBy_EmptyStackReturnsFalse)
{
    ItemStack emptyStack;
    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(emptyStack.canBeHurtBy(fireDamage));
}

// ============================================================================
// ItemEntity::isImmuneToFire 测试
// ============================================================================

TEST_F(ItemEntityTest, IsImmuneToFire_NormalItemNotImmune)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(entity.isImmuneToFire());
}

TEST_F(ItemEntityTest, IsImmuneToFire_NetheriteItemImmune)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

TEST_F(ItemEntityTest, IsImmuneToFire_NetherStarImmune)
{
    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "nether_star"));
    ASSERT_NE(netherStar, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netherStar, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

TEST_F(ItemEntityTest, IsImmuneToFire_AncientDebrisImmune)
{
    Item* debris = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "ancient_debris"));
    ASSERT_NE(debris, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*debris, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_TRUE(entity.isImmuneToFire());
}

// ============================================================================
// ItemEntity::hurt 测试
// ============================================================================

TEST_F(ItemEntityTest, Hurt_DefaultHealthIsFive)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.DEFAULT_HEALTH, 5);
}

TEST_F(ItemEntityTest, Hurt_InvulnerableEntityReturnsFalse)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    auto genericDamage = DamageSources::generic();
    EXPECT_FALSE(entity.hurt(genericDamage, 1.0f));
}

TEST_F(ItemEntityTest, Hurt_FireResistantItemNotHurtByFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    auto fireDamage = DamageSources::inFire();
    EXPECT_FALSE(entity.hurt(fireDamage, 3.0f));
}

TEST_F(ItemEntityTest, Hurt_NormalItemHurtByFireReducesHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto fireDamage = DamageSources::inFire();
    bool result = entity.hurt(fireDamage, 3.0f);
    EXPECT_TRUE(result);
    // 伤害后生命值应该减少：5 - 3 = 2
    EXPECT_EQ(entity.getHealth(), 2);
}

TEST_F(ItemEntityTest, Hurt_NormalItemHurtByGenericDamage)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 2.0f);
    EXPECT_TRUE(result);
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityTest, Hurt_HealthZeroDiscardsEntity)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 造成足够伤害使生命值归零（默认5点生命值，造成6点伤害）
    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 6.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityTest, Hurt_ExactHealthZeroDiscardsEntity)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    // 造成恰好5点伤害
    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 5.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityTest, Hurt_NetheriteItemHurtByGenericNotFire)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    // 防火物品不免疫普通伤害
    auto genericDamage = DamageSources::generic();
    bool result = entity.hurt(genericDamage, 2.0f);
    EXPECT_TRUE(result);
    EXPECT_EQ(entity.getHealth(), 3);
}

TEST_F(ItemEntityTest, Hurt_MultipleHitsReduceHealth)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);

    auto genericDamage = DamageSources::generic();

    // 第一击：5 - 2 = 3
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 3);
    EXPECT_FALSE(entity.isRemoved());

    // 第二击：3 - 2 = 1
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_EQ(entity.getHealth(), 1);
    EXPECT_FALSE(entity.isRemoved());

    // 第三击：1 - 2 = -1 -> 销毁
    EXPECT_TRUE(entity.hurt(genericDamage, 2.0f));
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityTest, Hurt_VoidDamageBypassesInvulnerability)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    entity.setInvulnerable(true);

    // 虚空伤害绕过无敌
    auto voidDamage = DamageSources::outOfWorld();
    bool result = entity.hurt(voidDamage, 100.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

TEST_F(ItemEntityTest, Hurt_NetheriteItemVoidDamageKills)
{
    Item* netheriteIngot = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "netherite_ingot"));
    ASSERT_NE(netheriteIngot, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*netheriteIngot, 1), 0.0f, 0.0f, 0.0f);

    // 防火物品不免疫虚空伤害
    auto voidDamage = DamageSources::outOfWorld();
    bool result = entity.hurt(voidDamage, 100.0f);
    EXPECT_TRUE(result);
    EXPECT_TRUE(entity.isRemoved());
}

// ============================================================================
// ItemEntity 默认值测试
// ============================================================================

TEST_F(ItemEntityTest, DefaultHealthIsFive)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.getHealth(), 5);
}

TEST_F(ItemEntityTest, DefaultPickupDelayIsTen)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.getPickupDelay(), 10);
}

TEST_F(ItemEntityTest, DefaultLifetimeIs6000)
{
    Item* stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    ASSERT_NE(stone, nullptr);
    ItemEntity entity(EntityId(1), ItemStack(*stone, 1), 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(entity.DEFAULT_LIFETIME, 6000);
}
