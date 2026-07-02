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
 * @file BrushItemTest.cpp
 * @brief BrushItem 刷子物品单元测试
 *
 * 测试刷子的核心逻辑：
 * - 物品注册和基本属性
 * - 使用动作和持续时长
 * - onUseTick 刷扫触发时机
 * - onUseTick 非玩家实体停止使用
 * - updateActiveItem 中 stopActiveHand 后不触发 onItemUseFinish
 * - itemInteractionForEntity 空实现
 * - 常量验证
 *
 * MC 1.21 参考：net.minecraft.world.item.BrushItem
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/special/BrushItem.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试用实体
// ============================================================================

/**
 * @brief 测试用 LivingEntity（非玩家）
 */
class TestLivingEntity : public LivingEntity {
public:
    TestLivingEntity()
        : LivingEntity(EntityId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }
};

/**
 * @brief 测试用 Player
 */
class TestPlayer : public Player {
public:
    explicit TestPlayer(IWorld* world = nullptr)
        : Player(EntityId(1), "TestPlayer")
    {
        registerAttributes();
        setHealth(maxHealth());
        if (world != nullptr) {
            setWorld(world);
        }
    }
};

// ============================================================================
// 测试用世界桩
// ============================================================================

/**
 * @brief 刷子测试用世界
 *
 * 记录 addParticle 调用以验证粒子生成。
 */
class BrushTestWorld final : public test::BaseTestWorld {
public:
    struct ParticleCall {
        particle::ParticleTypeId typeId;
        Vector3 position;
        Vector3 delta;
    };

    void addParticle(particle::ParticleTypeId typeId, const Vector3& pos, const Vector3& delta) override
    {
        m_particles.push_back(ParticleCall{typeId, pos, delta});
    }

    [[nodiscard]] const std::vector<ParticleCall>& particles() const { return m_particles; }
    [[nodiscard]] i32 particleCount() const { return static_cast<i32>(m_particles.size()); }
    void clearParticles() { m_particles.clear(); }

private:
    std::vector<ParticleCall> m_particles;
};

// ============================================================================
// 测试基类
// ============================================================================

class BrushItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override { m_world = std::make_unique<BrushTestWorld>(); }

    std::unique_ptr<BrushTestWorld> m_world;
};

// ============================================================================
// 物品注册测试
// ============================================================================

TEST_F(BrushItemTest, BrushIsRegistered)
{
    ASSERT_NE(Items::BRUSH, nullptr) << "BRUSH should be registered";
}

TEST_F(BrushItemTest, ArmadilloScuteIsRegistered)
{
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr) << "ARMADILLO_SCUTE should be registered";
}

TEST_F(BrushItemTest, BrushItemLocation)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->itemLocation(), ResourceLocation("minecraft:brush"));
}

TEST_F(BrushItemTest, ArmadilloScuteItemLocation)
{
    ASSERT_NE(Items::ARMADILLO_SCUTE, nullptr);
    EXPECT_EQ(Items::ARMADILLO_SCUTE->itemLocation(), ResourceLocation("minecraft:armadillo_scute"));
}

// ============================================================================
// 物品属性测试
// ============================================================================

TEST_F(BrushItemTest, MaxDamage)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->maxDamage(), item::BrushItem::MAX_DURABILITY);
    EXPECT_EQ(Items::BRUSH->maxDamage(), 64);
}

TEST_F(BrushItemTest, MaxStackSize)
{
    // 有耐久度的物品堆叠数为1
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->maxStackSize(), 1);
}

TEST_F(BrushItemTest, IsDamageable)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_TRUE(Items::BRUSH->isDamageable());
}

TEST_F(BrushItemTest, GetUseDuration)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(Items::BRUSH->getUseDuration(stack), item::BrushItem::USE_DURATION);
    EXPECT_EQ(Items::BRUSH->getUseDuration(stack), 200);
}

TEST_F(BrushItemTest, GetUseAction)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(Items::BRUSH->getUseAction(stack), UseAction::Brush);
}

TEST_F(BrushItemTest, GetItemEnchantability)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_EQ(Items::BRUSH->getItemEnchantability(), 1);
}

TEST_F(BrushItemTest, IsFood)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_FALSE(Items::BRUSH->isFood());
}

TEST_F(BrushItemTest, IsMusicDisc)
{
    ASSERT_NE(Items::BRUSH, nullptr);
    EXPECT_FALSE(Items::BRUSH->isMusicDisc());
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(BrushItemTest, ConstantValuesMatchMC)
{
    // MC 1.21: BrushItem 常量
    EXPECT_EQ(item::BrushItem::MAX_DURABILITY, 64);
    EXPECT_EQ(item::BrushItem::USE_DURATION, 200);
    EXPECT_EQ(item::BrushItem::ANIMATION_DURATION, 10);
    EXPECT_EQ(item::BrushItem::BRUSH_TICK_IN_CYCLE, 4);
    EXPECT_EQ(item::BrushItem::ARMADILLO_DURABILITY_COST, 16);
}

// ============================================================================
// 类型转换测试
// ============================================================================

TEST_F(BrushItemTest, DynamicCastToBrushItem)
{
    Item* baseItem = Items::BRUSH;
    ASSERT_NE(baseItem, nullptr);
    auto* brushItem = dynamic_cast<item::BrushItem*>(baseItem);
    ASSERT_NE(brushItem, nullptr) << "BRUSH should be castable to BrushItem";
}

// ============================================================================
// ItemStack 测试
// ============================================================================

TEST_F(BrushItemTest, ItemStackCreation)
{
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_FALSE(stack.isEmpty());
    EXPECT_EQ(stack.getItem(), Items::BRUSH);
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getDamage(), 0) << "New brush should have 0 damage";
}

TEST_F(BrushItemTest, ItemStackDurabilityTracking)
{
    ItemStack stack(Items::BRUSH, 1);
    EXPECT_EQ(stack.getMaxDamage(), 64);

    // 模拟耐久度消耗
    stack.attemptDamageItem(1, nullptr);
    EXPECT_EQ(stack.getDamage(), 1);

    // 累积消耗
    stack.attemptDamageItem(9, nullptr);
    EXPECT_EQ(stack.getDamage(), 10);
}

// ============================================================================
// onUseTick 刷扫触发时机测试
// ============================================================================

/**
 * @brief 验证刷扫触发时机计算
 *
 * MC原版逻辑：elapsedTicks 从1开始，
 * 当 (elapsedTicks % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1) 时触发刷扫。
 * 即在 elapsedTicks = 5, 15, 25, 35, ... 时触发。
 */
TEST_F(BrushItemTest, OnUseTick_BrushTriggerTiming)
{
    // 验证刷扫触发时机公式
    constexpr i32 ANIMATION_DURATION = item::BrushItem::ANIMATION_DURATION;   // 10
    constexpr i32 BRUSH_TICK_IN_CYCLE = item::BrushItem::BRUSH_TICK_IN_CYCLE; // 4

    // 应该触发刷扫的 elapsedTicks 值
    // (elapsedTicks % 10 == 4 + 1) => (elapsedTicks % 10 == 5)
    // 即: 5, 15, 25, 35, 45, 55, 65, 75, 85, 95, 105, ...
    for (i32 tick = 1; tick <= 200; ++tick) {
        bool shouldBrush = (tick % ANIMATION_DURATION == BRUSH_TICK_IN_CYCLE + 1);
        if (tick % 10 == 5) {
            EXPECT_TRUE(shouldBrush) << "Tick " << tick << " should trigger brush";
        } else {
            EXPECT_FALSE(shouldBrush) << "Tick " << tick << " should NOT trigger brush";
        }
    }
}

/**
 * @brief 验证粒子生成在正确的刷扫tick
 *
 * 当玩家使用刷子时，onUseTick 应该在刷扫触发tick生成 DustPlume 粒子。
 */
TEST_F(BrushItemTest, OnUseTick_PlayerGeneratesParticlesOnBrushTick)
{
    TestPlayer player(m_world.get());

    // 设置玩家手持刷子
    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);

    // 激活使用
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();

    // 模拟5个tick: elapsedTicks 1,2,3,4,5
    // 第5个tick（elapsedTicks=5）应该触发刷扫生成粒子
    for (i32 i = 0; i < 5; ++i) {
        player.updateActiveItem();
    }

    // 第5个tick（elapsedTicks=5）应该生成了粒子
    EXPECT_EQ(m_world->particleCount(), 1) << "Should generate one DustPlume particle at tick 5";
    if (!m_world->particles().empty()) {
        EXPECT_EQ(m_world->particles()[0].typeId, particle::ParticleTypeId::DustPlume);
    }
}

/**
 * @brief 验证非刷扫tick不生成粒子
 *
 * elapsedTicks = 1,2,3,4 不应生成粒子。
 */
TEST_F(BrushItemTest, OnUseTick_NoParticlesOnNonBrushTick)
{
    TestPlayer player(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();

    // 前4个tick不应生成粒子（elapsedTicks = 1,2,3,4）
    for (i32 i = 0; i < 4; ++i) {
        player.updateActiveItem();
    }

    EXPECT_EQ(m_world->particleCount(), 0) << "Should not generate particles on non-brush ticks";
}

/**
 * @brief 验证多个刷扫周期
 *
 * 在前25个tick中，应该在 elapsedTicks=5, 15, 25 生成粒子。
 * (elapsedTicks % 10 == 5) => 5, 15, 25
 */
TEST_F(BrushItemTest, OnUseTick_MultipleBrushCycles)
{
    TestPlayer player(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    player.setEquipment(EquipmentSlot::MainHand, brushStack);
    player.setActiveHand(Hand::MainHand);
    ASSERT_TRUE(player.isUsingItem());

    m_world->clearParticles();

    // tick 25次，elapsedTicks 1-25
    // 刷扫触发在 5, 15, 25
    for (i32 i = 0; i < 25; ++i) {
        player.updateActiveItem();
    }

    EXPECT_EQ(m_world->particleCount(), 3) << "Should generate 3 DustPlume particles in 25 ticks";
}

// ============================================================================
// onUseTick 非玩家实体停止使用测试
// ============================================================================

/**
 * @brief 验证非玩家实体使用刷子时被停止
 *
 * BrushItem::onUseTick 检查实体是否为玩家，
 * 非玩家实体调用 stopActiveHand()。
 */
TEST_F(BrushItemTest, OnUseTick_NonPlayerEntityStopsUsing)
{
    // 创建一个非玩家 LivingEntity
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    entity.setEquipment(EquipmentSlot::MainHand, brushStack);
    entity.setActiveHand(Hand::MainHand);

    // 确认已开始使用
    ASSERT_TRUE(entity.isUsingItem());

    // 模拟一个tick - onUseTick 会检测到非玩家并调用 stopActiveHand
    entity.updateActiveItem();

    // 非玩家应该已被停止使用
    EXPECT_FALSE(entity.isUsingItem()) << "Non-player entity should stop using brush after onUseTick";
}

// ============================================================================
// onUseTick 中 stopActiveHand 后不触发 onItemUseFinish 测试
// ============================================================================

/**
 * @brief 验证 onUseTick 内部调用 stopActiveHand 后不会触发 onItemUseFinish
 *
 * 当 onUseTick 调用 stopActiveHand()（如非玩家实体使用刷子），
 * updateActiveItem 不应在后续逻辑中调用 onItemUseFinish。
 * 这验证了 LivingEntity::updateActiveItem 中的 isUsingItem() 检查。
 */
TEST_F(BrushItemTest, OnUseTick_StopActiveHandPreventsFinish)
{
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    entity.setEquipment(EquipmentSlot::MainHand, brushStack);
    entity.setActiveHand(Hand::MainHand);

    ASSERT_TRUE(entity.isUsingItem());
    EXPECT_EQ(entity.getItemInUseCount(), item::BrushItem::USE_DURATION);

    // 单次tick后，非玩家实体应被停止使用
    entity.updateActiveItem();

    // 验证使用状态已完全重置（stopActiveHand 被调用）
    EXPECT_FALSE(entity.isUsingItem());
    EXPECT_EQ(entity.getItemInUseCount(), 0);
    EXPECT_TRUE(entity.getActiveItem().isEmpty());
}

// ============================================================================
// itemInteractionForEntity 测试
// ============================================================================

TEST_F(BrushItemTest, ItemInteractionForEntity_ReturnsFalse)
{
    // 当前 itemInteractionForEntity 返回 false（犰狳交互尚未实现）
    auto* brushItem = dynamic_cast<item::BrushItem*>(Items::BRUSH);
    ASSERT_NE(brushItem, nullptr);

    TestPlayer player(m_world.get());

    TestLivingEntity target;
    target.setWorld(m_world.get());

    ItemStack brushStack(Items::BRUSH, 1);
    EXPECT_FALSE(brushItem->itemInteractionForEntity(brushStack, player, target, Hand::MainHand));
}

// ============================================================================
// UseAction::Brush 枚举值测试
// ============================================================================

TEST_F(BrushItemTest, UseActionBrushValue)
{
    EXPECT_EQ(static_cast<u8>(UseAction::Brush), 9);
    EXPECT_STREQ(toString(UseAction::Brush), "brush");
}

// ============================================================================
// 默认 Item::onUseTick 测试
// ============================================================================

/**
 * @brief 验证默认 Item::onUseTick 不做任何事情
 *
 * 基类 Item 的 onUseTick 默认实现是空操作，
 * 不应抛出异常或改变任何状态。
 */
TEST_F(BrushItemTest, DefaultOnUseTickIsNoOp)
{
    // 使用一个没有重写 onUseTick 的普通物品
    ItemStack stickStack(Items::STICK, 1);
    Item* stickItem = Items::STICK;
    ASSERT_NE(stickItem, nullptr);

    // 创建一个测试实体
    TestLivingEntity entity;
    entity.setWorld(m_world.get());

    // 默认 onUseTick 不应抛出异常
    EXPECT_NO_THROW(stickItem->onUseTick(stickStack, *m_world, entity, 1));
    EXPECT_NO_THROW(stickItem->onUseTick(stickStack, *m_world, entity, 100));
}

// ============================================================================
// 耐久度消耗估算测试
// ============================================================================

/**
 * @brief 验证刷子耐久度消耗与使用次数的关系
 *
 * 刷子耐久度64，每次刷扫消耗1耐久（BrushableBlock场景），
 * 使用时长200 ticks，每10 ticks触发一次刷扫（第5 tick），
 * 因此一次完整使用最多触发 floor(200/10) = 20 次刷扫。
 *
 * 刷子可完整使用 floor(64/1) = 64 次刷扫（在BrushableBlock场景下），
 * 但每次完整使用最多触发20次刷扫，所以完全消耗耐久需要
 * ceil(64/20) = 4 次完整使用（64耐久 = 3*20 + 4）。
 */
TEST_F(BrushItemTest, DurabilityAndBrushCountEstimate)
{
    constexpr i32 maxDamage = item::BrushItem::MAX_DURABILITY;             // 64
    constexpr i32 useDuration = item::BrushItem::USE_DURATION;             // 200
    constexpr i32 animationDuration = item::BrushItem::ANIMATION_DURATION; // 10

    // 每次完整使用最多触发多少次刷扫
    i32 brushCountPerUse = useDuration / animationDuration; // 200 / 10 = 20
    EXPECT_EQ(brushCountPerUse, 20);

    // 总耐久度支持多少次刷扫（每次消耗1耐久）
    i32 totalBrushCount = maxDamage; // 64
    EXPECT_EQ(totalBrushCount, 64);
}

} // namespace
} // namespace mc
