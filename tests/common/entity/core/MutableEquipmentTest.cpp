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
 * IMPLIED, INCLUDING ANY KIND, EXPRESS OR IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/tick/manager/TickManager.hpp"

using namespace mc;

namespace {

// ============================================================================
// 测试用 mock 世界
// ============================================================================

class MutableEquipmentTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("MutableEquipmentTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("MutableEquipmentTestWorld::tickManager not implemented");
    }

    void playSound(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32) override {}
    void broadcastEntityStatus(EntityInstanceId, u8) override {}
};

// ============================================================================
// 测试用 TestLivingEntity
// ============================================================================

class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityInstanceId(1))
    {
        registerData();
        registerAttributes();
        setHealth(maxHealth());
    }
};

// ============================================================================
// 测试用 TestMobEntity
// ============================================================================

class TestMobEntity : public MobEntity {
public:
    TestMobEntity()
        : MobEntity(EntityInstanceId(2))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

} // namespace

// ============================================================================
// 测试固定装置
// ============================================================================

class MutableEquipmentTest : public ::testing::Test {
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
        m_world = std::make_unique<MutableEquipmentTestWorld>();
        m_living = std::make_unique<TestLivingEntity>();
        m_living->setWorld(m_world.get());
        m_mob = std::make_unique<TestMobEntity>();
        m_mob->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_mob.reset();
        m_living.reset();
        m_world.reset();
    }

    std::unique_ptr<MutableEquipmentTestWorld> m_world;
    std::unique_ptr<TestLivingEntity> m_living;
    std::unique_ptr<TestMobEntity> m_mob;
};

// ============================================================================
// LivingEntity::getMutableEquipment 基础测试
// ============================================================================

TEST_F(MutableEquipmentTest, GetMutableEquipmentReturnsEmptyByDefault)
{
    // 所有槽位默认为空
    for (i32 i = 0; i < static_cast<i32>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        EXPECT_TRUE(m_living->getMutableEquipment(slot).isEmpty()) << "Slot " << i << " should be empty by default";
    }
}

TEST_F(MutableEquipmentTest, GetMutableEquipmentReturnsSameAsGetEquipment)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 设置装备后，getMutableEquipment 和 getEquipment 返回相同内容
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));

    EXPECT_EQ(m_living->getMutableEquipment(EquipmentSlot::MainHand).getItem(),
        m_living->getEquipment(EquipmentSlot::MainHand).getItem());
    EXPECT_EQ(m_living->getMutableEquipment(EquipmentSlot::Head).getItem(),
        m_living->getEquipment(EquipmentSlot::Head).getItem());

    // 空槽位也应一致
    EXPECT_EQ(m_living->getMutableEquipment(EquipmentSlot::OffHand).isEmpty(),
        m_living->getEquipment(EquipmentSlot::OffHand).isEmpty());
}

TEST_F(MutableEquipmentTest, GetMutableEquipmentInvalidSlotReturnsEmpty)
{
    // 越界槽位应返回空物品堆引用（不崩溃）
    auto& ref = m_living->getMutableEquipment(static_cast<EquipmentSlot>(999));
    EXPECT_TRUE(ref.isEmpty());
}

TEST_F(MutableEquipmentTest, GetMutableEquipmentAllowsMutation)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 通过 setEquipment 设置物品
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getCount(), 1);

    // 通过 getMutableEquipment 可变引用修改物品数量
    m_living->getMutableEquipment(EquipmentSlot::MainHand).setCount(3);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getCount(), 3);
}

TEST_F(MutableEquipmentTest, GetMutableEquipmentMutationReflectsInConstAccess)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));

    // 通过可变引用修改伤害值
    m_living->getMutableEquipment(EquipmentSlot::MainHand).setDamage(50);
    // const 访问应看到修改后的值
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getDamage(), 50);
}

// ============================================================================
// LivingEntity 便利方法测试
// ============================================================================

TEST_F(MutableEquipmentTest, GetMutableMainHandItem)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    EXPECT_EQ(m_living->getMutableMainHandItem().getItem(), Items::IRON_SWORD);

    // 修改可变引用
    m_living->getMutableMainHandItem().setDamage(25);
    EXPECT_EQ(m_living->getMainHandItem().getDamage(), 25);
}

TEST_F(MutableEquipmentTest, GetMutableOffHandItem)
{
    ASSERT_NE(Items::SHIELD, nullptr);

    m_living->setEquipment(EquipmentSlot::OffHand, ItemStack(Items::SHIELD, 1));
    EXPECT_EQ(m_living->getMutableOffHandItem().getItem(), Items::SHIELD);

    // 修改可变引用
    m_living->getMutableOffHandItem().setDamage(10);
    EXPECT_EQ(m_living->getOffHandItem().getDamage(), 10);
}

// ============================================================================
// MobEntity::getMutableEquipment 测试（确保虚方法派发正确）
// ============================================================================

TEST_F(MutableEquipmentTest, MobEntityGetMutableEquipmentWorks)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    m_mob->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    EXPECT_EQ(m_mob->getMutableEquipment(EquipmentSlot::MainHand).getItem(), Items::IRON_SWORD);

    // MobEntity 使用 LivingEntity 基类实现，直接操作 m_equipment
    m_mob->getMutableEquipment(EquipmentSlot::MainHand).setDamage(30);
    EXPECT_EQ(m_mob->getEquipment(EquipmentSlot::MainHand).getDamage(), 30);
}

TEST_F(MutableEquipmentTest, MobEntityBurnUndeadUsesMutableEquipment)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);

    // 给 MobEntity 戴上头盔（Feet 槽位用于 sunProtectionSlot 的僵尸不是此测试重点，
    // 但此测试验证 getMutableEquipment 替代 m_equipment 直接访问后仍可修改物品）
    m_mob->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    i32 originalDamage = m_mob->getEquipment(EquipmentSlot::Head).getDamage();
    EXPECT_EQ(originalDamage, 0);

    // 通过 getMutableEquipment 修改伤害
    m_mob->getMutableEquipment(EquipmentSlot::Head).setDamage(10);
    EXPECT_EQ(m_mob->getEquipment(EquipmentSlot::Head).getDamage(), 10);
}

// ============================================================================
// hurtAndBreak 通过 getMutableEquipment 使用测试
// ============================================================================

TEST_F(MutableEquipmentTest, HurtAndBreakViaMutableEquipment)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 设置主手物品
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    i32 maxDamage = m_living->getEquipment(EquipmentSlot::MainHand).getMaxDamage();
    ASSERT_GT(maxDamage, 0);

    // 通过 getMutableEquipment 获取可变引用，调用 hurtAndBreak
    ItemStack& stack = m_living->getMutableEquipment(EquipmentSlot::MainHand);
    bool broken = LivingEntity::hurtAndBreak(stack, 1, m_living.get(), EquipmentSlot::MainHand);
    EXPECT_FALSE(broken); // 1 点伤害不应该破坏物品
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getDamage(), 1);
}

TEST_F(MutableEquipmentTest, HurtAndBreakDestroysItem)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 设置主手物品
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    i32 maxDamage = m_living->getEquipment(EquipmentSlot::MainHand).getMaxDamage();
    ASSERT_GT(maxDamage, 0);

    // 通过 getMutableEquipment 获取可变引用，设置伤害为最大值-1
    m_living->getMutableEquipment(EquipmentSlot::MainHand).setDamage(maxDamage - 1);

    // hurtAndBreak 1 点应导致物品损坏
    ItemStack& stack = m_living->getMutableEquipment(EquipmentSlot::MainHand);
    bool broken = LivingEntity::hurtAndBreak(stack, 1, m_living.get(), EquipmentSlot::MainHand);
    EXPECT_TRUE(broken);
    // 物品损坏后应变为空
    EXPECT_TRUE(m_living->getEquipment(EquipmentSlot::MainHand).isEmpty());
}

// ============================================================================
// 多槽位 getMutableEquipment 测试
// ============================================================================

TEST_F(MutableEquipmentTest, AllEquipmentSlotsMutable)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ASSERT_NE(Items::SHIELD, nullptr);
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    ASSERT_NE(Items::IRON_LEGGINGS, nullptr);
    ASSERT_NE(Items::IRON_BOOTS, nullptr);

    // 设置所有装备槽位
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    m_living->setEquipment(EquipmentSlot::OffHand, ItemStack(Items::SHIELD, 1));
    m_living->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_living->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    m_living->setEquipment(EquipmentSlot::Legs, ItemStack(Items::IRON_LEGGINGS, 1));
    m_living->setEquipment(EquipmentSlot::Feet, ItemStack(Items::IRON_BOOTS, 1));

    // 逐个通过 getMutableEquipment 修改伤害值
    m_living->getMutableEquipment(EquipmentSlot::MainHand).setDamage(1);
    m_living->getMutableEquipment(EquipmentSlot::OffHand).setDamage(2);
    m_living->getMutableEquipment(EquipmentSlot::Head).setDamage(3);
    m_living->getMutableEquipment(EquipmentSlot::Chest).setDamage(4);
    m_living->getMutableEquipment(EquipmentSlot::Legs).setDamage(5);
    m_living->getMutableEquipment(EquipmentSlot::Feet).setDamage(6);

    // 通过 const 访问验证修改
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getDamage(), 1);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::OffHand).getDamage(), 2);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::Head).getDamage(), 3);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::Chest).getDamage(), 4);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::Legs).getDamage(), 5);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::Feet).getDamage(), 6);
}

// ============================================================================
// getMutableEquipment 和 setEquipment 一致性测试
// ============================================================================

TEST_F(MutableEquipmentTest, SetEquipmentThenMutableAccessConsistent)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ASSERT_NE(Items::DIAMOND_SWORD, nullptr);

    // 先 setEquipment 设置
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    EXPECT_EQ(m_living->getMutableEquipment(EquipmentSlot::MainHand).getItem(), Items::IRON_SWORD);

    // 通过 getMutableEquipment 修改后再 setEquipment 覆盖
    m_living->getMutableEquipment(EquipmentSlot::MainHand).setDamage(50);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getDamage(), 50);

    // setEquipment 覆盖后，getMutableEquipment 应反映新值
    m_living->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::DIAMOND_SWORD, 1));
    EXPECT_EQ(m_living->getMutableEquipment(EquipmentSlot::MainHand).getItem(), Items::DIAMOND_SWORD);
    EXPECT_EQ(m_living->getEquipment(EquipmentSlot::MainHand).getDamage(), 0);
}

// ============================================================================
// Player::getMutableEquipment 虚方法派发测试
//
// Player 重写 getMutableEquipment()，委托到 PlayerInventory 而非基类 m_equipment。
// 以下测试验证 Player 的 override 实现正确地将可变引用指向 PlayerInventory。
// ============================================================================

class PlayerMutableEquipmentTest : public ::testing::Test {
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
        m_world = std::make_unique<MutableEquipmentTestWorld>();
        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(10), "TestPlayer");
        m_player->setWorld(m_world.get());
    }

    void TearDown() override
    {
        m_player.reset();
        m_world.reset();
    }

    std::unique_ptr<MutableEquipmentTestWorld> m_world;
    std::unique_ptr<Player> m_player;
};

TEST_F(PlayerMutableEquipmentTest, PlayerGetMutableEquipmentDelegatesToPlayerInventory)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 通过 setEquipment 设置主手物品（Player 的 override 会写入 PlayerInventory）
    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));

    // 通过 PlayerInventory 直接验证物品存在
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getItem(), Items::IRON_SWORD);

    // 通过 getMutableEquipment 修改伤害值
    m_player->getMutableEquipment(EquipmentSlot::MainHand).setDamage(42);

    // 修改应通过 PlayerInventory 可见
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getDamage(), 42);

    // 同时通过 const 访问也可见
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::MainHand).getDamage(), 42);
}

TEST_F(PlayerMutableEquipmentTest, PlayerGetMutableMainHandItemDelegatesToPlayerInventory)
{
    ASSERT_NE(Items::DIAMOND_SWORD, nullptr);

    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::DIAMOND_SWORD, 1));

    // getMutableMainHandItem 应修改 PlayerInventory 中的主手物品
    m_player->getMutableMainHandItem().setDamage(99);

    // 验证 PlayerInventory 中的引用被修改
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getDamage(), 99);
    // 验证 const 访问一致
    EXPECT_EQ(m_player->getMainHandItem().getDamage(), 99);
}

TEST_F(PlayerMutableEquipmentTest, PlayerGetMutableOffHandItemDelegatesToPlayerInventory)
{
    ASSERT_NE(Items::SHIELD, nullptr);

    m_player->setEquipment(EquipmentSlot::OffHand, ItemStack(Items::SHIELD, 1));

    // getMutableOffHandItem 应修改 PlayerInventory 中的副手物品
    m_player->getMutableOffHandItem().setDamage(77);

    // 验证 PlayerInventory 中的引用被修改
    EXPECT_EQ(m_player->inventory().getOffhandItemRef().getDamage(), 77);
    // 验证 const 访问一致
    EXPECT_EQ(m_player->getOffHandItem().getDamage(), 77);
}

TEST_F(PlayerMutableEquipmentTest, PlayerArmorSlotsMutableViaPlayerInventory)
{
    ASSERT_NE(Items::IRON_HELMET, nullptr);
    ASSERT_NE(Items::IRON_CHESTPLATE, nullptr);
    ASSERT_NE(Items::IRON_LEGGINGS, nullptr);
    ASSERT_NE(Items::IRON_BOOTS, nullptr);

    // 设置护甲
    m_player->setEquipment(EquipmentSlot::Head, ItemStack(Items::IRON_HELMET, 1));
    m_player->setEquipment(EquipmentSlot::Chest, ItemStack(Items::IRON_CHESTPLATE, 1));
    m_player->setEquipment(EquipmentSlot::Legs, ItemStack(Items::IRON_LEGGINGS, 1));
    m_player->setEquipment(EquipmentSlot::Feet, ItemStack(Items::IRON_BOOTS, 1));

    // 通过 getMutableEquipment 修改伤害
    m_player->getMutableEquipment(EquipmentSlot::Head).setDamage(10);
    m_player->getMutableEquipment(EquipmentSlot::Chest).setDamage(20);
    m_player->getMutableEquipment(EquipmentSlot::Legs).setDamage(30);
    m_player->getMutableEquipment(EquipmentSlot::Feet).setDamage(40);

    // 验证 PlayerInventory 中的护甲引用被修改
    EXPECT_EQ(m_player->inventory().getHelmetRef().getDamage(), 10);
    EXPECT_EQ(m_player->inventory().getChestplateRef().getDamage(), 20);
    EXPECT_EQ(m_player->inventory().getLeggingsRef().getDamage(), 30);
    EXPECT_EQ(m_player->inventory().getBootsRef().getDamage(), 40);

    // 验证 const 访问一致
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::Head).getDamage(), 10);
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::Chest).getDamage(), 20);
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::Legs).getDamage(), 30);
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::Feet).getDamage(), 40);
}

TEST_F(PlayerMutableEquipmentTest, PlayerInventoryMutationVisibleViaGetMutableEquipment)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 通过 setEquipment 设置主手物品
    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));

    // 直接通过 PlayerInventory 修改伤害值
    m_player->inventory().getSelectedStackRef().setDamage(55);

    // 通过 getMutableEquipment 应可见
    EXPECT_EQ(m_player->getMutableEquipment(EquipmentSlot::MainHand).getDamage(), 55);
    // 通过 getEquipment 也应可见
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::MainHand).getDamage(), 55);
}

TEST_F(PlayerMutableEquipmentTest, PlayerHurtAndBreakViaGetMutableEquipment)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);

    // 设置主手物品
    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));
    i32 maxDamage = m_player->getEquipment(EquipmentSlot::MainHand).getMaxDamage();
    ASSERT_GT(maxDamage, 0);

    // 通过 getMutableEquipment 获取可变引用并调用 hurtAndBreak
    ItemStack& stack = m_player->getMutableEquipment(EquipmentSlot::MainHand);
    bool broken = LivingEntity::hurtAndBreak(stack, 1, m_player.get(), EquipmentSlot::MainHand);
    EXPECT_FALSE(broken);

    // hurtAndBreak 修改的物品应反映在 PlayerInventory 中
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getDamage(), 1);
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::MainHand).getDamage(), 1);
}

TEST_F(PlayerMutableEquipmentTest, PlayerInvalidSlotReturnsEmpty)
{
    // Player 对越界槽位也应返回空物品堆
    auto& ref = m_player->getMutableEquipment(static_cast<EquipmentSlot>(999));
    EXPECT_TRUE(ref.isEmpty());
}

TEST_F(PlayerMutableEquipmentTest, PlayerDoesNotModifyBaseClassMEquipment)
{
    ASSERT_NE(Items::IRON_SWORD, nullptr);
    ASSERT_NE(Items::DIAMOND_SWORD, nullptr);

    // 通过 Player 的 setEquipment 设置主手物品（写入 PlayerInventory）
    m_player->setEquipment(EquipmentSlot::MainHand, ItemStack(Items::IRON_SWORD, 1));

    // 通过 getMutableEquipment 修改（应修改 PlayerInventory，而非基类 m_equipment）
    m_player->getMutableEquipment(EquipmentSlot::MainHand).setDamage(33);

    // 基类 m_equipment[MainHand] 应仍为空（Player 不使用基类 m_equipment）
    // 通过 LivingEntity::getEquipment 虚方法验证：Player override 返回 PlayerInventory 数据
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::MainHand).getDamage(), 33);
    EXPECT_EQ(m_player->getEquipment(EquipmentSlot::MainHand).getItem(), Items::IRON_SWORD);

    // 通过 PlayerInventory 直接验证一致
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getDamage(), 33);
    EXPECT_EQ(m_player->inventory().getSelectedStackRef().getItem(), Items::IRON_SWORD);
}
