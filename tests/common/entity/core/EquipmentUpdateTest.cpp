/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include "common/item/items/tool/SwordItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;
using namespace mc::entity::attribute;

namespace {

// ============================================================================
// 测试用 mock 世界
// ============================================================================

class EquipmentUpdateTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("EquipmentUpdateTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("EquipmentUpdateTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void broadcastEntityStatus(EntityInstanceId, u8) override {}
};

// ============================================================================
// 测试用 TestLivingEntity（不注册 ATTACK_DAMAGE 属性）
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1), nullptr, mc::test::testEcsRegistry())
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// 测试用 TestLivingEntityWithAttack（注册 ATTACK_DAMAGE 和 ATTACK_SPEED 属性）
// 用于验证剑的修饰符是否正确应用
// ============================================================================

class TestLivingEntityWithAttack : public LivingEntity {
public:
    TestLivingEntityWithAttack()
        : LivingEntity(EntityInstanceId(2), nullptr, mc::test::testEcsRegistry())
    {
        registerData();
        registerAttributes();
        // MonsterEntity 会注册这两个属性，此处手动注册以模拟怪物实体
        attributes().registerAttribute(*entity::attribute::Attributes::attackDamage());
        attributes().registerAttribute(*entity::attribute::Attributes::attackSpeed());
        setHealth(maxHealth());
    }
};

} // namespace

// ============================================================================
// 测试固定装置
// ============================================================================

class EquipmentUpdateTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<EquipmentUpdateTestWorld>();
        m_living = std::make_unique<TestLivingEntity>();
        m_living->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_living.reset();
        m_world.reset();
    }

    std::unique_ptr<EquipmentUpdateTestWorld> m_world;
    std::unique_ptr<TestLivingEntity> m_living;
};

// ============================================================================
// equipmentHasChanged 测试
// ============================================================================

TEST_F(EquipmentUpdateTest, EquipmentHasChangedBothEmpty)
{
    ItemStack empty1;
    ItemStack empty2;
    EXPECT_FALSE(LivingEntity::equipmentHasChanged(empty1, empty2));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedSameItem)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ItemStack sword1(Items::IRON_SWORD, 1);
    ItemStack sword2(Items::IRON_SWORD, 1);
    EXPECT_FALSE(LivingEntity::equipmentHasChanged(sword1, sword2));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedDifferentItems)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ASSERT_NE(Items::DIAMOND_SWORD, nullptr);
    ItemStack ironSword(Items::IRON_SWORD, 1);
    ItemStack diamondSword(Items::DIAMOND_SWORD, 1);
    EXPECT_TRUE(LivingEntity::equipmentHasChanged(ironSword, diamondSword));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedEmptyToItem)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ItemStack empty;
    ItemStack sword(Items::IRON_SWORD, 1);
    EXPECT_TRUE(LivingEntity::equipmentHasChanged(empty, sword));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedItemToEmpty)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ItemStack sword(Items::IRON_SWORD, 1);
    ItemStack empty;
    EXPECT_TRUE(LivingEntity::equipmentHasChanged(sword, empty));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedDifferentCount)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ItemStack one(Items::IRON_SWORD, 1);
    ItemStack two(Items::IRON_SWORD, 2);
    EXPECT_TRUE(LivingEntity::equipmentHasChanged(one, two));
}

TEST_F(EquipmentUpdateTest, EquipmentHasChangedDifferentDamage)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ItemStack noDamage(Items::IRON_SWORD, 1);
    noDamage.setDamage(0);
    ItemStack damaged(Items::IRON_SWORD, 1);
    damaged.setDamage(10);
    EXPECT_TRUE(LivingEntity::equipmentHasChanged(noDamage, damaged));
}

// ============================================================================
// detectEquipmentUpdates 测试
// ============================================================================

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesFirstTickInitializes)
{
    // 首次调用 detectEquipmentUpdates 应初始化快照但不应用修饰符
    m_living->detectEquipmentUpdates();

    // 验证属性值仍然是默认值（未装备物品时属性不变）
    EXPECT_DOUBLE_EQ(m_living->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(m_living->getAttributeValue(Attributes::ARMOR, 0.0), 0.0);
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesNoChangeNoEffect)
{
    // 初始化快照
    m_living->detectEquipmentUpdates();

    // 无装备变化时再次调用，属性不应改变
    m_living->detectEquipmentUpdates();
    EXPECT_DOUBLE_EQ(m_living->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0), 0.0);
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesSwordNoEffectWithoutAttribute)
{
    // LivingEntity 基类不注册 ATTACK_DAMAGE 和 ATTACK_SPEED 属性，
    // 所以剑的修饰符无法被应用（AttributeMap::addModifier 返回 false）。
    // 这是正确的行为——子类需要在 registerAttributes() 中注册这些属性。
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_living->detectEquipmentUpdates();
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_living->detectEquipmentUpdates();

    // 攻击伤害属性未注册，因此修饰符添加被忽略，值仍为默认
    EXPECT_DOUBLE_EQ(m_living->getAttributeValue(Attributes::ATTACK_DAMAGE, -1.0), -1.0)
        << "ATTACK_DAMAGE should not change when attribute is not registered";
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesAppliesArmorModifiers)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 初始化快照
    m_living->detectEquipmentUpdates();

    // 装备铁头盔（防御 2，韧性 0 —— 原版行为：铁盔甲韧性为0）
    ItemStack ironHelmet(Items::IRON_HELMET, 1);
    m_living->setEquipment(EquipmentSlot::Head, ironHelmet);

    // 检测装备变化并应用修饰符
    m_living->detectEquipmentUpdates();

    // LivingEntity 基类注册了 ARMOR 属性，所以护甲修饰符应被应用
    // 铁头盔防御 2 = 铁材质对 Head 槽位的防御值
    f64 armor = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armor, 0.0) << "Armor attribute should increase after equipping iron helmet";

    // 铁盔甲的韧性为 0.0（原版行为：只有钻石和下界合金有韧性）
    // 所以护甲韧性不应改变
    f64 toughness = m_living->getAttributeValue(Attributes::ARMOR_TOUGHNESS, 0.0);
    EXPECT_DOUBLE_EQ(toughness, 0.0) << "Iron armor should have 0 toughness per vanilla behavior";
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesRemovesArmorModifiers)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 初始化快照
    m_living->detectEquipmentUpdates();

    // 装备铁头盔
    ItemStack ironHelmet(Items::IRON_HELMET, 1);
    m_living->setEquipment(EquipmentSlot::Head, ironHelmet);
    m_living->detectEquipmentUpdates();

    f64 armorWithHelmet = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithHelmet, 0.0);

    // 卸下铁头盔
    m_living->setEquipment(EquipmentSlot::Head, ItemStack());
    m_living->detectEquipmentUpdates();

    // 护甲值应回到 0
    f64 armorAfterRemoval = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_DOUBLE_EQ(armorAfterRemoval, 0.0) << "Armor should return to 0 after removing helmet";
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesReplacesArmor)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    ASSERT_NE(Items::DIAMOND_HELMET, nullptr);

    // 初始化快照
    m_living->detectEquipmentUpdates();

    // 装备铁头盔
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithIron = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithIron, 0.0);

    // 替换为钻石头盔（防御更高）
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::DIAMOND_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithDiamond = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithDiamond, armorWithIron) << "Diamond helmet should provide more armor than iron helmet";
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesMultipleSlots)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);

    // 初始化快照
    m_living->detectEquipmentUpdates();

    // 同时装备多个护甲
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithBoth = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithBoth, 0.0);

    // 卸下胸甲
    m_living->setEquipment(EquipmentSlot::Chest, ItemStack());
    m_living->detectEquipmentUpdates();

    f64 armorWithOnlyHelmet = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_LT(armorWithOnlyHelmet, armorWithBoth) << "Armor should decrease after removing chestplate";
    EXPECT_GT(armorWithOnlyHelmet, 0.0) << "Helmet armor should still be present";
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesClientSideNoOp)
{
    // 客户端世界不应执行装备更新检测
    // BaseTestWorld 的 isClientSide() 默认返回 false
    // 这里无法直接测试客户端行为，但验证服务端正常工作即可
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    m_living->detectEquipmentUpdates();
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    EXPECT_GT(m_living->getAttributeValue(Attributes::ARMOR, 0.0), 0.0);
}

// ============================================================================
// stopLocationBasedEffects 测试
// ============================================================================

TEST_F(EquipmentUpdateTest, StopLocationBasedEffectsEmptyStack)
{
    // 空物品堆不应导致崩溃
    ItemStack empty;
    m_living->stopLocationBasedEffects(empty, EquipmentSlot::MainHand);
    // 仅验证不崩溃
}

TEST_F(EquipmentUpdateTest, StopLocationBasedEffectsRemovesArmorModifiers)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 先通过 detectEquipmentUpdates 添加修饰符
    m_living->detectEquipmentUpdates();
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithHelmet = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithHelmet, 0.0);

    // 直接调用 stopLocationBasedEffects
    m_living->stopLocationBasedEffects(m_living->getEquipment(EquipmentSlot::Head), EquipmentSlot::Head);

    // 护甲值应回到 0
    f64 armorAfter = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_DOUBLE_EQ(armorAfter, 0.0);
}

TEST_F(EquipmentUpdateTest, StopLocationBasedEffectsNoopForWrongSlot)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 先通过 detectEquipmentUpdates 添加修饰符（Head 槽位）
    m_living->detectEquipmentUpdates();
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithHelmet = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithHelmet, 0.0);

    // 对错误的槽位调用 stopLocationBasedEffects（MainHand 槽位）
    // 铁头盔的修饰符只对 Head 槽位有效，对 MainHand 调用不应移除 Head 的修饰符
    m_living->stopLocationBasedEffects(m_living->getEquipment(EquipmentSlot::Head), EquipmentSlot::MainHand);

    // Head 槽位的护甲值应不变
    f64 armorAfter = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_DOUBLE_EQ(armorAfter, armorWithHelmet) << "Armor should not change when stopping effects for wrong slot";
}

// ============================================================================
// onEquippedItemBroken 测试（stopLocationBasedEffects 集成）
// ============================================================================

TEST_F(EquipmentUpdateTest, OnEquippedItemBrokenRemovesModifiers)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 先通过 detectEquipmentUpdates 添加修饰符
    m_living->detectEquipmentUpdates();
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armorWithHelmet = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armorWithHelmet, 0.0);

    // 物品损坏时调用 onEquippedItemBroken
    m_living->onEquippedItemBroken(*Items::IRON_HELMET, EquipmentSlot::Head);

    // 护甲修饰符应被移除
    f64 armorAfter = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_DOUBLE_EQ(armorAfter, 0.0) << "Armor modifiers should be removed after item breaks";
}

// ============================================================================
// ArmorItem::getAttributeModifiers(i32) 测试
// ============================================================================

class ArmorItemAttributeModifiersTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            s_initialized = true;
        }
    }
};

TEST_F(ArmorItemAttributeModifiersTest, IronHelmetReturnsModifiersForHeadSlot)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(Items::IRON_HELMET);
    ASSERT_NE(armorItem, nullptr);

    auto modifiers = armorItem->getAttributeModifiers(static_cast<i32>(EquipmentSlot::Head));
    EXPECT_FALSE(modifiers.isEmpty()) << "Iron helmet should have modifiers for Head slot";
}

TEST_F(ArmorItemAttributeModifiersTest, IronHelmetReturnsEmptyForWrongSlot)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(Items::IRON_HELMET);
    ASSERT_NE(armorItem, nullptr);

    auto modifiers = armorItem->getAttributeModifiers(static_cast<i32>(EquipmentSlot::MainHand));
    EXPECT_TRUE(modifiers.isEmpty()) << "Iron helmet should have no modifiers for MainHand slot";

    auto feetModifiers = armorItem->getAttributeModifiers(static_cast<i32>(EquipmentSlot::Feet));
    EXPECT_TRUE(feetModifiers.isEmpty()) << "Iron helmet should have no modifiers for Feet slot";
}

TEST_F(ArmorItemAttributeModifiersTest, DiamondChestplateReturnsModifiersForChestSlot)
{
    ASSERT_NE(Items::DIAMOND_CHESTPLATE, nullptr);
    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(Items::DIAMOND_CHESTPLATE);
    ASSERT_NE(armorItem, nullptr);

    auto modifiers = armorItem->getAttributeModifiers(static_cast<i32>(EquipmentSlot::Chest));
    EXPECT_FALSE(modifiers.isEmpty()) << "Diamond chestplate should have modifiers for Chest slot";

    auto wrongSlotModifiers = armorItem->getAttributeModifiers(static_cast<i32>(EquipmentSlot::Legs));
    EXPECT_TRUE(wrongSlotModifiers.isEmpty()) << "Diamond chestplate should have no modifiers for Legs slot";
}

TEST_F(ArmorItemAttributeModifiersTest, BaseItemReturnsEmptyModifiers)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    // 铁剑不是盔甲，getDefense() 不可用，但基类 Item::getAttributeModifiers 应返回空
    auto modifiers = Items::IRON_SWORD->getAttributeModifiers(static_cast<i32>(EquipmentSlot::MainHand));
    // 注意：SwordItem 重写了 getAttributeModifiers，会返回攻击修饰符
    // 这里测试 Item 基类的默认实现——使用一个没有重写的物品
    // 实际上 IRON_SWORD 是 SwordItem，所以我们改用 APPLE 来测试基类
    ASSERT_NE(Items::APPLE, nullptr);
    auto appleModifiers = Items::APPLE->getAttributeModifiers(static_cast<i32>(EquipmentSlot::MainHand));
    EXPECT_TRUE(appleModifiers.isEmpty()) << "Apple should have no attribute modifiers";
}

TEST_F(ArmorItemAttributeModifiersTest, SwordReturnsModifiersForMainHandOnly)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 铁剑应有 MainHand 修饰符
    auto mainHandModifiers = Items::IRON_SWORD->getAttributeModifiers(static_cast<i32>(EquipmentSlot::MainHand));
    EXPECT_FALSE(mainHandModifiers.isEmpty()) << "Iron sword should have modifiers for MainHand";

    // 铁剑不应有 OffHand 修饰符
    auto offHandModifiers = Items::IRON_SWORD->getAttributeModifiers(static_cast<i32>(EquipmentSlot::OffHand));
    EXPECT_TRUE(offHandModifiers.isEmpty()) << "Iron sword should have no modifiers for OffHand";
}

TEST_F(ArmorItemAttributeModifiersTest, ModifierEntrySlotMatchesRequestedSlot)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(Items::IRON_HELMET);
    ASSERT_NE(armorItem, nullptr);

    i32 headSlot = static_cast<i32>(EquipmentSlot::Head);
    auto modifiers = armorItem->getAttributeModifiers(headSlot);

    for (const auto& entry : modifiers.getEntries()) {
        EXPECT_EQ(entry.equipmentSlot, headSlot) << "Modifier entry should match the requested equipment slot";
    }
}

// ============================================================================
// 边界场景测试
// ============================================================================

TEST_F(EquipmentUpdateTest, StopLocationBasedEffectsNullItem)
{
    // 使用 nullptr Item 的 ItemStack（理论上不应出现，但测试健壮性）
    ItemStack empty;
    EXPECT_NO_THROW(m_living->stopLocationBasedEffects(empty, EquipmentSlot::MainHand));
}

TEST_F(EquipmentUpdateTest, DetectEquipmentUpdatesMultipleTicksNoChange)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 初始化
    m_living->detectEquipmentUpdates();

    // 装备
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->detectEquipmentUpdates();

    f64 armor = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_GT(armor, 0.0);

    // 多次 tick 无变化，属性应保持不变
    for (int i = 0; i < 10; ++i) {
        m_living->detectEquipmentUpdates();
    }

    f64 armorAfterMultipleTicks = m_living->getAttributeValue(Attributes::ARMOR, 0.0);
    EXPECT_DOUBLE_EQ(armorAfterMultipleTicks, armor)
        << "Armor should remain stable across multiple ticks with no equipment change";
}

// ============================================================================
// MonsterEntity 装备修饰符测试（ATTACK_DAMAGE 属性已注册）
// ============================================================================

class MonsterEquipmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        static bool s_initialized = false;
        if (!s_initialized) {
            Items::initialize();
            s_initialized = true;
        }
    }

    void SetUp() override
    {
        m_world = std::make_unique<EquipmentUpdateTestWorld>();
        m_entity = std::make_unique<TestLivingEntityWithAttack>();
        m_entity->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_entity.reset();
        m_world.reset();
    }

    std::unique_ptr<EquipmentUpdateTestWorld> m_world;
    std::unique_ptr<TestLivingEntityWithAttack> m_entity;
};

TEST_F(MonsterEquipmentTest, SwordAppliesAttackDamageModifier)
{
    // 注册了 ATTACK_DAMAGE 属性后，铁剑的攻击伤害修饰符应正确应用
    // 铁剑攻击伤害 = 铁材质攻击伤害 2.0 + 基础 4.0 = 6.0
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_entity->detectEquipmentUpdates();

    // 装备前的攻击伤害应为默认值 2.0（ATTACK_DAMAGE 属性默认值）
    f64 attackDamageBefore = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_DOUBLE_EQ(attackDamageBefore, 2.0);

    // 装备铁剑
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_entity->detectEquipmentUpdates();

    // 装备后攻击伤害应增加
    // 铁剑攻击伤害修饰符为 +5.0（参数 3 + 材质 2.0），总计 2.0 + 5.0 = 7.0
    f64 attackDamageAfter = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_GT(attackDamageAfter, attackDamageBefore) << "Attack damage should increase after equipping iron sword";
    EXPECT_DOUBLE_EQ(attackDamageAfter, 7.0);
}

TEST_F(MonsterEquipmentTest, SwordRemovesAttackDamageOnUnequip)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_entity->detectEquipmentUpdates();
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_entity->detectEquipmentUpdates();

    f64 attackDamageWithSword = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_GT(attackDamageWithSword, 2.0); // 大于默认值 2.0

    // 卸下铁剑
    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack());
    m_entity->detectEquipmentUpdates();

    f64 attackDamageAfter = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_DOUBLE_EQ(attackDamageAfter, 2.0) << "Attack damage should return to default after unequipping sword";
}

TEST_F(MonsterEquipmentTest, SwordAttackSpeedModifier)
{
    // 铁剑的攻击速度修饰符应为 -2.4
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_entity->detectEquipmentUpdates();

    // 装备前的攻击速度应为默认值 4.0（Attributes::attackSpeed 默认值）
    f64 attackSpeedBefore = m_entity->getAttributeValue(Attributes::ATTACK_SPEED, 0.0);

    m_entity->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_entity->detectEquipmentUpdates();

    // 装备后攻击速度应降低（-2.4 修饰符）
    f64 attackSpeedAfter = m_entity->getAttributeValue(Attributes::ATTACK_SPEED, 0.0);
    EXPECT_LT(attackSpeedAfter, attackSpeedBefore) << "Attack speed should decrease after equipping iron sword";
}

TEST_F(MonsterEquipmentTest, SwordDoesNotAffectOtherSlots)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_entity->detectEquipmentUpdates();

    f64 attackDamageBefore = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);

    // 将铁剑放在 Head 槽位（不应该生效）
    m_entity->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_SWORD, 1));
    m_entity->detectEquipmentUpdates();

    // 攻击伤害不应改变（剑的修饰符只对 MainHand 生效）
    f64 attackDamageAfter = m_entity->getAttributeValue(Attributes::ATTACK_DAMAGE, 0.0);
    EXPECT_DOUBLE_EQ(attackDamageAfter, attackDamageBefore)
        << "Sword should not affect attack damage when in Head slot";
}
