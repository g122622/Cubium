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

#include "item/Items.hpp"
#include "item/armor/ArmorMaterial.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include <array>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// ItemProperties 测试
// ============================================================================

class ItemPropertiesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个测试前重置 Items
        // 注意：Items::initialize() 只能调用一次
    }
};

TEST_F(ItemPropertiesTest, DefaultValues)
{
    ItemProperties props;

    EXPECT_EQ(props.maxStackSize(), 64);
    EXPECT_EQ(props.maxDamage(), 0);
    EXPECT_EQ(props.containerItem(), nullptr);
    EXPECT_EQ(props.rarity(), ItemRarity::Common);
    EXPECT_FALSE(props.isBurnable());
    EXPECT_TRUE(props.isRepairable());
}

TEST_F(ItemPropertiesTest, MaxStackSize)
{
    ItemProperties props;

    props.maxStackSize(32);
    EXPECT_EQ(props.maxStackSize(), 32);

    props.maxStackSize(1);
    EXPECT_EQ(props.maxStackSize(), 1);

    // 边界测试：最小值
    props.maxStackSize(1);
    EXPECT_EQ(props.maxStackSize(), 1);
}

TEST_F(ItemPropertiesTest, MaxDamageSetsStackSizeToOne)
{
    ItemProperties props;

    // 设置耐久度后，堆叠数应自动变为1
    props.maxDamage(100);
    EXPECT_EQ(props.maxDamage(), 100);
    EXPECT_EQ(props.maxStackSize(), 1);

    // 先设置堆叠数再设置耐久度
    ItemProperties props2;
    props2.maxStackSize(64);
    props2.maxDamage(50);
    EXPECT_EQ(props2.maxStackSize(), 1);
}

TEST_F(ItemPropertiesTest, Rarity)
{
    ItemProperties props;

    props.rarity(ItemRarity::Uncommon);
    EXPECT_EQ(props.rarity(), ItemRarity::Uncommon);

    props.rarity(ItemRarity::Rare);
    EXPECT_EQ(props.rarity(), ItemRarity::Rare);

    props.rarity(ItemRarity::Epic);
    EXPECT_EQ(props.rarity(), ItemRarity::Epic);
}

TEST_F(ItemPropertiesTest, ChainedCalls)
{
    ItemProperties props;

    props.maxStackSize(16).maxDamage(0).rarity(ItemRarity::Rare).burnable(true).repairable(false);

    EXPECT_EQ(props.maxStackSize(), 16);
    EXPECT_EQ(props.maxDamage(), 0);
    EXPECT_EQ(props.rarity(), ItemRarity::Rare);
    EXPECT_TRUE(props.isBurnable());
    EXPECT_FALSE(props.isRepairable());
}

// ============================================================================
// Item 测试
// ============================================================================

class ItemTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemTest, ItemRegistration)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_EQ(diamond->itemLocation(), ResourceLocation("minecraft:diamond"));
    EXPECT_GT(diamond->itemId(), 0);
}

TEST_F(ItemTest, ItemById)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    ItemId id = diamond->itemId();
    Item* retrieved = ItemRegistry::instance().getItem(id);
    EXPECT_EQ(retrieved, diamond);
}

TEST_F(ItemTest, ItemProperties)
{
    Item* diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
    ASSERT_NE(diamondSword, nullptr);

    // 钻石剑有耐久度，堆叠数应为1
    EXPECT_EQ(diamondSword->maxStackSize(), 1);
    EXPECT_TRUE(diamondSword->isDamageable());
    EXPECT_EQ(diamondSword->maxDamage(), 1561);
}

TEST_F(ItemTest, StackableItem)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    // 钻石是可堆叠的
    EXPECT_EQ(diamond->maxStackSize(), 64);
    EXPECT_FALSE(diamond->isDamageable());
}

TEST_F(ItemTest, RarityItems)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_EQ(diamond->rarity(), ItemRarity::Rare);

    Item* netherStar = ItemRegistry::instance().getItem(ResourceLocation("minecraft:nether_star"));
    ASSERT_NE(netherStar, nullptr);
    EXPECT_EQ(netherStar->rarity(), ItemRarity::Uncommon);
}

TEST_F(ItemTest, ArmorMaterialsUseVanillaSoundsAndRepairItems)
{
    struct ArmorMaterialCase {
        const item::armor::ArmorMaterial* material;
        const char* equipSoundId;
        const Item* repairItem;
    };

    const std::array<ArmorMaterialCase, 8> cases = {{
        {&item::armor::ArmorMaterials::LEATHER, "minecraft:item.armor.equip_leather", Items::LEATHER},
        {&item::armor::ArmorMaterials::COPPER, "minecraft:item.armor.equip_copper", Items::COPPER_INGOT},
        {&item::armor::ArmorMaterials::CHAIN, "minecraft:item.armor.equip_chain", Items::IRON_INGOT},
        {&item::armor::ArmorMaterials::IRON, "minecraft:item.armor.equip_iron", Items::IRON_INGOT},
        {&item::armor::ArmorMaterials::GOLD, "minecraft:item.armor.equip_gold", Items::GOLD_INGOT},
        {&item::armor::ArmorMaterials::DIAMOND, "minecraft:item.armor.equip_diamond", Items::DIAMOND},
        {&item::armor::ArmorMaterials::TURTLE, "minecraft:item.armor.equip_turtle", Items::TURTLE_SCUTE},
        {&item::armor::ArmorMaterials::NETHERITE, "minecraft:item.armor.equip_netherite", Items::NETHERITE_INGOT},
    }};

    for (const auto& testCase : cases) {
        ASSERT_NE(testCase.material, nullptr);
        ASSERT_NE(testCase.repairItem, nullptr);

        EXPECT_EQ(testCase.material->getEquipSound().getId(), ResourceLocation(testCase.equipSoundId));
        EXPECT_TRUE(testCase.material->getRepairMaterial().test(*testCase.repairItem));
    }
}

TEST_F(ItemTest, ArmorMaterialAssetIdsMatchVanillaEquipmentPaths)
{
    // getAssetId() 返回的资产ID应与 MC 1.21+ equipment 纹理目录名称一致
    struct AssetIdCase {
        const item::armor::ArmorMaterial* material;
        const char* expectedAssetId;
    };

    const std::array<AssetIdCase, 8> cases = {{
        {&item::armor::ArmorMaterials::LEATHER, "leather"},
        {&item::armor::ArmorMaterials::COPPER, "copper"},
        {&item::armor::ArmorMaterials::CHAIN, "chainmail"},
        {&item::armor::ArmorMaterials::IRON, "iron"},
        {&item::armor::ArmorMaterials::GOLD, "gold"},
        {&item::armor::ArmorMaterials::DIAMOND, "diamond"},
        {&item::armor::ArmorMaterials::TURTLE, "turtle_scute"},
        {&item::armor::ArmorMaterials::NETHERITE, "netherite"},
    }};

    for (const auto& testCase : cases) {
        ASSERT_NE(testCase.material, nullptr);
        EXPECT_EQ(testCase.material->getAssetId(), testCase.expectedAssetId)
            << "Material " << testCase.material->getName() << " asset ID mismatch";
    }
}

TEST_F(ItemTest, ArmorTexturePathReturnsCorrectHumanoidPaths)
{
    using ArmorSlot = item::armor::ArmorSlot;

    // 头盔/胸甲/靴子使用 humanoid 子目录
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("iron", ArmorSlot::Head),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/iron.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("diamond", ArmorSlot::Chest),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/diamond.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("netherite", ArmorSlot::Feet),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/netherite.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("turtle_scute", ArmorSlot::Head),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/turtle_scute.png"));
}

TEST_F(ItemTest, ArmorTexturePathReturnsCorrectLeggingsPaths)
{
    using ArmorSlot = item::armor::ArmorSlot;

    // 护腿使用 humanoid_leggings 子目录
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("iron", ArmorSlot::Legs),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid_leggings/iron.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("chainmail", ArmorSlot::Legs),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid_leggings/chainmail.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getArmorTexturePath("leather", ArmorSlot::Legs),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid_leggings/leather.png"));
}

TEST_F(ItemTest, ArmorTexturePathMatchesAllMaterialAssetIds)
{
    using ArmorSlot = item::armor::ArmorMaterial;

    // 验证每种材质的 assetId 都能正确构建纹理路径
    struct PathCase {
        const item::armor::ArmorMaterial* material;
        const char* expectedHeadPath;
        const char* expectedLegsPath;
    };

    const std::array<PathCase, 8> cases = {{
        {&item::armor::ArmorMaterials::LEATHER,
            "minecraft:textures/entity/equipment/humanoid/leather.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/leather.png"},
        {&item::armor::ArmorMaterials::COPPER,
            "minecraft:textures/entity/equipment/humanoid/copper.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/copper.png"},
        {&item::armor::ArmorMaterials::CHAIN,
            "minecraft:textures/entity/equipment/humanoid/chainmail.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/chainmail.png"},
        {&item::armor::ArmorMaterials::IRON,
            "minecraft:textures/entity/equipment/humanoid/iron.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/iron.png"},
        {&item::armor::ArmorMaterials::GOLD,
            "minecraft:textures/entity/equipment/humanoid/gold.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/gold.png"},
        {&item::armor::ArmorMaterials::DIAMOND,
            "minecraft:textures/entity/equipment/humanoid/diamond.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/diamond.png"},
        {&item::armor::ArmorMaterials::TURTLE,
            "minecraft:textures/entity/equipment/humanoid/turtle_scute.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/turtle_scute.png"},
        {&item::armor::ArmorMaterials::NETHERITE,
            "minecraft:textures/entity/equipment/humanoid/netherite.png",
            "minecraft:textures/entity/equipment/humanoid_leggings/netherite.png"},
    }};

    for (const auto& testCase : cases) {
        ASSERT_NE(testCase.material, nullptr);
        auto headPath = item::armor::ArmorMaterial::getArmorTexturePath(
            testCase.material->getAssetId(), item::armor::ArmorSlot::Head);
        auto legsPath = item::armor::ArmorMaterial::getArmorTexturePath(
            testCase.material->getAssetId(), item::armor::ArmorSlot::Legs);
        EXPECT_EQ(headPath, ResourceLocation(testCase.expectedHeadPath))
            << "Head texture path mismatch for " << testCase.material->getName();
        EXPECT_EQ(legsPath, ResourceLocation(testCase.expectedLegsPath))
            << "Legs texture path mismatch for " << testCase.material->getName();
    }
}

TEST_F(ItemTest, LeatherOverlayTexturePathReturnsCorrectPaths)
{
    using ArmorSlot = item::armor::ArmorSlot;

    // 头盔/胸甲/靴子使用 humanoid 子目录的覆盖层
    EXPECT_EQ(item::armor::ArmorMaterial::getLeatherOverlayTexturePath(ArmorSlot::Head),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/leather_overlay.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getLeatherOverlayTexturePath(ArmorSlot::Chest),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/leather_overlay.png"));
    EXPECT_EQ(item::armor::ArmorMaterial::getLeatherOverlayTexturePath(ArmorSlot::Feet),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid/leather_overlay.png"));

    // 护腿使用 humanoid_leggings 子目录的覆盖层
    EXPECT_EQ(item::armor::ArmorMaterial::getLeatherOverlayTexturePath(ArmorSlot::Legs),
        ResourceLocation("minecraft:textures/entity/equipment/humanoid_leggings/leather_overlay.png"));
}

TEST_F(ItemTest, NonExistentItem)
{
    Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft:nonexistent"));
    EXPECT_EQ(item, nullptr);
}

TEST_F(ItemTest, NonExistentItemById)
{
    Item* item = ItemRegistry::instance().getItem(9999);
    EXPECT_EQ(item, nullptr);
}

TEST_F(ItemTest, TranslationKey)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_EQ(diamond->getTranslationKey(), "item.minecraft:diamond");
}

TEST_F(ItemTest, GetDefaultInstance)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    ItemStack stack = diamond->getDefaultInstance();
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), diamond);
    EXPECT_EQ(stack.getCount(), 1);
}

TEST_F(ItemTest, ForEachItem)
{
    size_t count = 0;
    ItemRegistry::instance().forEachItem([&count](Item& item) { count++; });
    EXPECT_GT(count, 0);
}

TEST_F(ItemTest, DuplicateRegistrationReturnsExistingItem)
{
    const ResourceLocation id("test:duplicate_item_reg");
    const size_t countBefore = ItemRegistry::instance().itemCount();

    auto& first = ItemRegistry::instance().registerItem<Item>(id, ItemProperties().maxStackSize(16));
    auto& second = ItemRegistry::instance().registerItem<Item>(id, ItemProperties().maxStackSize(1));

    EXPECT_EQ(&first, &second);
    EXPECT_EQ(ItemRegistry::instance().itemCount(), countBefore + 1);
    EXPECT_EQ(first.maxStackSize(), 16);
}

// ============================================================================
// ItemStack 测试
// ============================================================================

class ItemStackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
        m_diamondSword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
        m_stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    }

    Item* m_diamond = nullptr;
    Item* m_diamondSword = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(ItemStackTest, EmptyStack)
{
    ItemStack empty;
    EXPECT_TRUE(empty.isEmpty());
    EXPECT_EQ(empty.getCount(), 0);
    EXPECT_EQ(empty.getItem(), nullptr);
}

TEST_F(ItemStackTest, EmptyConstant)
{
    EXPECT_TRUE(ItemStack::EMPTY.isEmpty());
}

TEST_F(ItemStackTest, CreateStack)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 32);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), m_diamond);
    EXPECT_EQ(stack.getCount(), 32);
    EXPECT_EQ(stack.getMaxStackSize(), 64);
}

TEST_F(ItemStackTest, CreateStackFromPointer)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(m_diamond, 16);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), m_diamond);
    EXPECT_EQ(stack.getCount(), 16);
}

TEST_F(ItemStackTest, ZeroCountBecomesEmpty)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 0);
    EXPECT_TRUE(stack.isEmpty());
    EXPECT_EQ(stack.getCount(), 0);
}

TEST_F(ItemStackTest, NullItemBecomesEmpty)
{
    ItemStack stack(static_cast<Item*>(nullptr), 10);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(ItemStackTest, SetCount)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 10);
    stack.setCount(50);
    EXPECT_EQ(stack.getCount(), 50);

    stack.setCount(0);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(ItemStackTest, GrowAndShrink)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 10);
    stack.grow(5);
    EXPECT_EQ(stack.getCount(), 15);

    stack.shrink(3);
    EXPECT_EQ(stack.getCount(), 12);
}

TEST_F(ItemStackTest, DamageableStack)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack stack(*m_diamondSword, 1);
    EXPECT_TRUE(stack.isDamageable());
    EXPECT_EQ(stack.getMaxStackSize(), 1);
    EXPECT_EQ(stack.getMaxDamage(), 1561);
}

TEST_F(ItemStackTest, DamageAndBreak)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack stack(*m_diamondSword, 1);
    EXPECT_FALSE(stack.isDamaged());

    stack.setDamage(100);
    EXPECT_TRUE(stack.isDamaged());
    EXPECT_EQ(stack.getDamage(), 100);
}

TEST_F(ItemStackTest, BreakItem)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack stack(*m_diamondSword, 1);

    // 造成足够的伤害使物品损坏
    bool broken = stack.attemptDamageItem(2000);
    EXPECT_TRUE(broken);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(ItemStackTest, AttemptDamagePartial)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack stack(*m_diamondSword, 1);

    // 部分伤害
    bool broken = stack.attemptDamageItem(100);
    EXPECT_FALSE(broken);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getDamage(), 100);
}

TEST_F(ItemStackTest, CanMergeWith)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_diamondSword, nullptr);
    ASSERT_NE(m_stick, nullptr);

    // 相同物品可以合并
    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*m_diamond, 20);
    EXPECT_TRUE(stack1.canMergeWith(stack2));

    // 不同物品不能合并
    ItemStack stack3(*m_stick, 10);
    EXPECT_FALSE(stack1.canMergeWith(stack3));

    // 有耐久度的物品不能合并（因为堆叠数已经是1，无法再添加）
    ItemStack sword1(*m_diamondSword, 1);
    ItemStack sword2(*m_diamondSword, 1);
    // 两者耐久度相同（都是0），但由于堆叠数限制（maxStackSize=1），不能合并
    EXPECT_FALSE(sword1.canMergeWith(sword2));
}

TEST_F(ItemStackTest, CanMergeWithDamaged)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack sword1(*m_diamondSword, 1);
    ItemStack sword2(*m_diamondSword, 1);

    sword1.setDamage(50);
    sword2.setDamage(100);

    // 不同耐久度不能合并
    EXPECT_FALSE(sword1.canMergeWith(sword2));
}

TEST_F(ItemStackTest, IsSameItem)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*m_diamond, 20);

    EXPECT_TRUE(stack1.isSameItem(stack2));

    ItemStack stack3(*m_stick, 10);
    EXPECT_FALSE(stack1.isSameItem(stack3));

    // 空堆与空堆
    ItemStack empty1;
    ItemStack empty2;
    EXPECT_TRUE(empty1.isSameItem(empty2));

    // 空堆与非空堆
    EXPECT_FALSE(empty1.isSameItem(stack1));
}

TEST_F(ItemStackTest, Split)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 32);

    ItemStack split = stack.split(10);
    EXPECT_EQ(split.getCount(), 10);
    EXPECT_EQ(stack.getCount(), 22);
    EXPECT_EQ(split.getItem(), m_diamond);
}

TEST_F(ItemStackTest, SplitMoreThanAvailable)
{
    ASSERT_NE(m_diamond, nullptr);

    ItemStack stack(*m_diamond, 10);

    ItemStack split = stack.split(20);
    EXPECT_EQ(split.getCount(), 10);
    EXPECT_TRUE(stack.isEmpty());
}

TEST_F(ItemStackTest, SplitEmpty)
{
    ItemStack empty;
    ItemStack split = empty.split(5);
    EXPECT_TRUE(split.isEmpty());
}

TEST_F(ItemStackTest, Copy)
{
    ASSERT_NE(m_diamondSword, nullptr);

    ItemStack original(*m_diamondSword, 1);
    original.setDamage(50);

    ItemStack copy = original.copy();
    EXPECT_EQ(copy.getCount(), 1);
    EXPECT_EQ(copy.getDamage(), 50);
    EXPECT_EQ(copy.getItem(), m_diamondSword);

    // 修改副本不影响原堆
    copy.setDamage(100);
    EXPECT_EQ(original.getDamage(), 50);
}

TEST_F(ItemStackTest, Equality)
{
    ASSERT_NE(m_diamond, nullptr);
    ASSERT_NE(m_stick, nullptr);

    ItemStack stack1(*m_diamond, 10);
    ItemStack stack2(*m_diamond, 10);
    ItemStack stack3(*m_diamond, 20);
    ItemStack stack4(*m_stick, 10);

    EXPECT_EQ(stack1, stack2);
    EXPECT_NE(stack1, stack3); // 数量不同
    EXPECT_NE(stack1, stack4); // 物品不同

    // 空堆相等
    ItemStack empty1;
    ItemStack empty2;
    EXPECT_EQ(empty1, empty2);
}

// ============================================================================
// ItemRegistry 测试
// ============================================================================

class ItemRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemRegistryTest, GetItem)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);
    EXPECT_EQ(diamond->itemLocation(), ResourceLocation("minecraft:diamond"));
}

TEST_F(ItemRegistryTest, GetItemById)
{
    Item* diamond = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond"));
    ASSERT_NE(diamond, nullptr);

    ItemId id = diamond->itemId();
    Item* retrieved = ItemRegistry::instance().getItem(id);
    EXPECT_EQ(retrieved, diamond);
}

TEST_F(ItemRegistryTest, HasItem)
{
    EXPECT_TRUE(ItemRegistry::instance().hasItem(ResourceLocation("minecraft:diamond")));
    EXPECT_FALSE(ItemRegistry::instance().hasItem(ResourceLocation("minecraft:nonexistent")));
}

TEST_F(ItemRegistryTest, RegisterSimpleItem)
{
    Item& customItem =
        ItemRegistry::instance().registerItem(ResourceLocation("test:custom_item"), ItemProperties().maxStackSize(16));

    EXPECT_EQ(customItem.maxStackSize(), 16);
    EXPECT_EQ(customItem.itemLocation(), ResourceLocation("test:custom_item"));
    EXPECT_GT(customItem.itemId(), 0);

    // 验证可以获取
    Item* retrieved = ItemRegistry::instance().getItem(ResourceLocation("test:custom_item"));
    EXPECT_EQ(retrieved, &customItem);
}

TEST_F(ItemRegistryTest, RegisterDamageableItem)
{
    Item& customSword =
        ItemRegistry::instance().registerItem(ResourceLocation("test:custom_sword"), ItemProperties().maxDamage(1000));

    EXPECT_EQ(customSword.maxDamage(), 1000);
    EXPECT_EQ(customSword.maxStackSize(), 1);
    EXPECT_TRUE(customSword.isDamageable());
}

TEST_F(ItemRegistryTest, ItemCount)
{
    size_t count = ItemRegistry::instance().itemCount();
    EXPECT_GT(count, 0);
}
