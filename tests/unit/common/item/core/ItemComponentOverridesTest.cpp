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

/**
 * @file ItemComponentOverridesTest.cpp
 * @brief 数据组件覆盖（max_stack_size / max_damage / unbreakable / item_name /
 *        item_model / rarity / enchantable / attack_range）的运行时消费与编解码测试。
 *
 * 覆盖三个层面：
 * - ItemRarity 辅助函数（名/ID 双向映射、越界回落 Common）
 * - AttackRange::defaultFor（纯值语义）
 * - ItemStack 对组件覆盖的运行时消费（getMaxStackSize / getMaxDamage /
 *   isDamageable / getRarity / getEnchantmentValue / isEnchantable）与
 *   NBT / wire 往返保真
 */

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/component/AttackRange.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemRarity.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/backend/java/mappings/JavaItemIdMap.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/StringTextComponent.hpp"

using namespace mc;
using namespace mc::item::component;

// ============================================================================
// ItemRarity 辅助函数
// ============================================================================

TEST(ItemRarityTest, NameRoundTrip)
{
    EXPECT_EQ(rarityName(ItemRarity::Common), std::string_view("common"));
    EXPECT_EQ(rarityName(ItemRarity::Uncommon), std::string_view("uncommon"));
    EXPECT_EQ(rarityName(ItemRarity::Rare), std::string_view("rare"));
    EXPECT_EQ(rarityName(ItemRarity::Epic), std::string_view("epic"));

    EXPECT_EQ(rarityFromName("common"), ItemRarity::Common);
    EXPECT_EQ(rarityFromName("epic"), ItemRarity::Epic);
    EXPECT_FALSE(rarityFromName("legendary").has_value());
}

TEST(ItemRarityTest, IdMappingMatchesVanillaOrder)
{
    // 严格对齐 vanilla Rarity 声明顺序 common(0) uncommon(1) rare(2) epic(3)
    EXPECT_EQ(rarityId(ItemRarity::Common), 0);
    EXPECT_EQ(rarityId(ItemRarity::Uncommon), 1);
    EXPECT_EQ(rarityId(ItemRarity::Rare), 2);
    EXPECT_EQ(rarityId(ItemRarity::Epic), 3);

    EXPECT_EQ(rarityFromId(0), ItemRarity::Common);
    EXPECT_EQ(rarityFromId(3), ItemRarity::Epic);
    // 越界回落 Common（对齐 vanilla ByIdMap.OutOfBoundsStrategy.ZERO）
    EXPECT_EQ(rarityFromId(99), ItemRarity::Common);
    EXPECT_EQ(rarityFromId(-1), ItemRarity::Common);
}

// ============================================================================
// AttackRange 纯值语义
// ============================================================================

TEST(AttackRangeTest, DefaultForUsesInteractionRange)
{
    const auto range = AttackRange::defaultFor(3.0f);
    EXPECT_FLOAT_EQ(range.minRange, 0.0f);
    EXPECT_FLOAT_EQ(range.maxRange, 3.0f);
    EXPECT_FLOAT_EQ(range.minCreativeRange, 0.0f);
    EXPECT_FLOAT_EQ(range.maxCreativeRange, 3.0f);
    EXPECT_FLOAT_EQ(range.hitboxMargin, 0.0f);
    EXPECT_FLOAT_EQ(range.mobFactor, 1.0f);
}

TEST(AttackRangeTest, EqualityComparesAllFields)
{
    const AttackRange a{2.0f, 4.5f, 2.0f, 6.5f, 0.125f, 0.5f};
    AttackRange b = a;
    EXPECT_TRUE(a == b);
    b.hitboxMargin = 0.3f;
    EXPECT_FALSE(a == b);
}

// ============================================================================
// ItemStack 组件覆盖：运行时消费 + 编解码
// ============================================================================

class ItemComponentOverridesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_sword = ItemRegistry::instance().getItem(ResourceLocation("minecraft:diamond_sword"));
        m_stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
        ASSERT_NE(m_sword, nullptr);
        ASSERT_NE(m_stick, nullptr);
        ASSERT_TRUE(network::backend::java::JavaItemIdMap::instance().initialize().success());
    }

    const Item* m_sword = nullptr; // 有耐久（可损坏）
    const Item* m_stick = nullptr; // 无耐久（可堆叠 64）
};

TEST_F(ItemComponentOverridesTest, MaxStackSizeOverrideOnNonDamageableItem)
{
    ItemStack stack(*m_stick, 1);
    EXPECT_EQ(stack.getMaxStackSize(), 64);

    DataComponentPatch patch;
    patch.add(DataComponentType::MaxStackSize, DataComponentPayload{std::in_place_index<1>, 16});
    stack.applyComponentPatch(patch);
    EXPECT_EQ(stack.getMaxStackSize(), 16);

    // 移除后回退基础值
    DataComponentPatch removePatch;
    removePatch.remove(DataComponentType::MaxStackSize);
    stack.applyComponentPatch(removePatch);
    EXPECT_EQ(stack.getMaxStackSize(), 64);
}

TEST_F(ItemComponentOverridesTest, MaxDamageOverrideChangesDamageability)
{
    ItemStack stack(*m_sword, 1);
    EXPECT_TRUE(stack.isDamageable());
    EXPECT_GT(stack.getMaxDamage(), 0);

    DataComponentPatch patch;
    patch.add(DataComponentType::MaxDamage, DataComponentPayload{std::in_place_index<1>, 100});
    stack.applyComponentPatch(patch);
    EXPECT_EQ(stack.getMaxDamage(), 100);
    EXPECT_TRUE(stack.isDamageable());

    // max_damage=0 → 不可损坏
    DataComponentPatch zeroPatch;
    zeroPatch.add(DataComponentType::MaxDamage, DataComponentPayload{std::in_place_index<1>, 0});
    stack.applyComponentPatch(zeroPatch);
    EXPECT_FALSE(stack.isDamageable());
}

TEST_F(ItemComponentOverridesTest, UnbreakableMakesItemNonDamageable)
{
    ItemStack stack(*m_sword, 1);
    EXPECT_TRUE(stack.isDamageable());
    EXPECT_FALSE(stack.isUnbreakable());

    stack.setUnbreakable(true);
    EXPECT_TRUE(stack.isUnbreakable());
    EXPECT_FALSE(stack.isDamageable());
    // 不可损坏后堆叠上限不再被强制为 1
    EXPECT_GE(stack.getMaxStackSize(), 1);

    stack.setUnbreakable(false);
    EXPECT_FALSE(stack.isUnbreakable());
    EXPECT_TRUE(stack.isDamageable());
}

TEST_F(ItemComponentOverridesTest, RarityOverrideAndEnchantUpgrade)
{
    ItemStack stack(*m_stick, 1);
    // 默认稀有度
    EXPECT_EQ(stack.getRarity(), ItemRarity::Common);

    DataComponentPatch patch;
    patch.add(DataComponentType::Rarity, DataComponentPayload{std::in_place_index<9>, ItemRarity::Epic});
    stack.applyComponentPatch(patch);
    EXPECT_EQ(stack.getRarity(), ItemRarity::Epic);
    // 已 EPIC，附魔不改变
    stack.addEnchantment("minecraft:sharpness", 1);
    EXPECT_EQ(stack.getRarity(), ItemRarity::Epic);
}

TEST_F(ItemComponentOverridesTest, CommonRarityUpgradesWhenEnchanted)
{
    ItemStack stack(*m_stick, 1);
    stack.addEnchantment("minecraft:unbreaking", 1);
    EXPECT_EQ(stack.getRarity(), ItemRarity::Rare);
}

TEST_F(ItemComponentOverridesTest, EnchantableOverrideGatesEnchantability)
{
    ItemStack stack(*m_stick, 1);
    // 木棍无附魔能力
    EXPECT_EQ(stack.getEnchantmentValue(), 0);
    EXPECT_FALSE(stack.isEnchantable());

    stack.setEnchantableOverride(15);
    EXPECT_EQ(stack.getEnchantmentValue(), 15);
    EXPECT_TRUE(stack.isEnchantable());

    // 已附魔后不可再附魔
    stack.addEnchantment("minecraft:sharpness", 1);
    EXPECT_FALSE(stack.isEnchantable());

    // 清除覆盖后回退基础值
    stack.setEnchantableOverride(0);
    EXPECT_EQ(stack.getEnchantmentValue(), 0);
}

TEST_F(ItemComponentOverridesTest, AttackRangeOverrideIsReadable)
{
    ItemStack stack(*m_stick, 1);
    EXPECT_EQ(stack.getAttackRange(), nullptr);

    const AttackRange spear{2.0f, 4.5f, 2.0f, 6.5f, 0.125f, 0.5f};
    stack.setAttackRange(spear);
    ASSERT_NE(stack.getAttackRange(), nullptr);
    EXPECT_TRUE(*stack.getAttackRange() == spear);
}

TEST_F(ItemComponentOverridesTest, ItemNameAndModelOverride)
{
    ItemStack stack(*m_stick, 1);
    EXPECT_EQ(stack.getItemNameComponent(), nullptr);
    EXPECT_EQ(stack.getItemModel(), nullptr);

    stack.setItemNameComponent(std::make_unique<text::StringTextComponent>("Custom Stick"));
    stack.setItemModel("minecraft:item/custom_stick");
    ASSERT_NE(stack.getItemNameComponent(), nullptr);
    EXPECT_EQ(stack.getItemNameComponent()->getUnformattedText(), "Custom Stick");
    ASSERT_NE(stack.getItemModel(), nullptr);
    EXPECT_EQ(*stack.getItemModel(), "minecraft:item/custom_stick");
    // 无 custom_name 时，item_name 组件作为显示名
    EXPECT_EQ(stack.getDisplayName()->getUnformattedText(), "Custom Stick");
}

// ============================================================================
// NBT / wire 往返：新增组件保真
// ============================================================================

TEST_F(ItemComponentOverridesTest, NbtRoundTripPreservesNewComponents)
{
    ItemStack stack(*m_sword, 1);
    stack.setUnbreakable(true);
    stack.setItemModel("minecraft:item/custom");
    stack.setEnchantableOverride(12);
    stack.setAttackRange(AttackRange{2.0f, 4.5f, 2.0f, 6.5f, 0.125f, 0.5f});
    DataComponentPatch patch;
    patch.add(DataComponentType::MaxDamage, DataComponentPayload{std::in_place_index<1>, 500});
    patch.add(DataComponentType::Rarity, DataComponentPayload{std::in_place_index<9>, ItemRarity::Uncommon});
    stack.applyComponentPatch(patch);

    nbt::tags::compound_tag tag;
    stack.toNbt(tag);

    auto result = ItemStack::fromNbt(tag);
    ASSERT_TRUE(result.success());
    const auto& restored = result.value();

    EXPECT_TRUE(restored.isUnbreakable());
    EXPECT_EQ(restored.getMaxDamage(), 500);
    EXPECT_EQ(restored.getRarity(), ItemRarity::Uncommon);
    EXPECT_EQ(restored.getEnchantmentValue(), 12);
    ASSERT_NE(restored.getItemModel(), nullptr);
    EXPECT_EQ(*restored.getItemModel(), "minecraft:item/custom");
    ASSERT_NE(restored.getAttackRange(), nullptr);
    EXPECT_FLOAT_EQ(restored.getAttackRange()->maxRange, 4.5f);
    EXPECT_FLOAT_EQ(restored.getAttackRange()->hitboxMargin, 0.125f);
}

TEST_F(ItemComponentOverridesTest, WireRoundTripPreservesNewComponents)
{
    ItemStack stack(*m_sword, 1);
    stack.setUnbreakable(true);
    stack.setItemModel("minecraft:item/custom");
    stack.setEnchantableOverride(12);
    stack.setAttackRange(AttackRange{2.0f, 4.5f, 2.0f, 6.5f, 0.125f, 0.5f});
    DataComponentPatch patch;
    patch.add(DataComponentType::MaxDamage, DataComponentPayload{std::in_place_index<1>, 500});
    patch.add(DataComponentType::Rarity, DataComponentPayload{std::in_place_index<9>, ItemRarity::Uncommon});
    stack.applyComponentPatch(patch);

    const auto view = network::ir::toItemStackView(stack);
    EXPECT_FALSE(view.componentsPatch.empty());

    auto result = network::ir::fromItemStackView(view);
    ASSERT_TRUE(result.success());
    const auto& restored = result.value();

    EXPECT_TRUE(restored.isUnbreakable());
    EXPECT_EQ(restored.getMaxDamage(), 500);
    EXPECT_EQ(restored.getRarity(), ItemRarity::Uncommon);
    EXPECT_EQ(restored.getEnchantmentValue(), 12);
    ASSERT_NE(restored.getItemModel(), nullptr);
    EXPECT_EQ(*restored.getItemModel(), "minecraft:item/custom");
    ASSERT_NE(restored.getAttackRange(), nullptr);
    EXPECT_FLOAT_EQ(restored.getAttackRange()->maxRange, 4.5f);
    EXPECT_FLOAT_EQ(restored.getAttackRange()->hitboxMargin, 0.125f);
}

TEST_F(ItemComponentOverridesTest, ComponentIdsMatchVanillaRegistryOrder)
{
    // typeId 严格对齐 vanilla DataComponents.register 声明顺序（关键抽样）
    EXPECT_EQ(componentTypeId(DataComponentType::CustomData), 0);
    EXPECT_EQ(componentTypeId(DataComponentType::MaxStackSize), 1);
    EXPECT_EQ(componentTypeId(DataComponentType::MaxDamage), 2);
    EXPECT_EQ(componentTypeId(DataComponentType::Damage), 3);
    EXPECT_EQ(componentTypeId(DataComponentType::Unbreakable), 4);
    EXPECT_EQ(componentTypeId(DataComponentType::CustomName), 6);
    EXPECT_EQ(componentTypeId(DataComponentType::ItemName), 8);
    EXPECT_EQ(componentTypeId(DataComponentType::ItemModel), 9);
    EXPECT_EQ(componentTypeId(DataComponentType::Lore), 10);
    EXPECT_EQ(componentTypeId(DataComponentType::Rarity), 11);
    EXPECT_EQ(componentTypeId(DataComponentType::Enchantments), 12);
    EXPECT_EQ(componentTypeId(DataComponentType::CanPlaceOn), 13);
    EXPECT_EQ(componentTypeId(DataComponentType::CanBreak), 14);
    EXPECT_EQ(componentTypeId(DataComponentType::RepairCost), 18);
    EXPECT_EQ(componentTypeId(DataComponentType::AttackRange), 29);
    EXPECT_EQ(componentTypeId(DataComponentType::Enchantable), 30);
    EXPECT_EQ(componentTypeId(DataComponentType::PotionContents), 48);
}
