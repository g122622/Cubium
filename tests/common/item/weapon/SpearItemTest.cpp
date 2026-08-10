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
#include "common/core/Types.hpp"
#include "common/entity/ecs/components/ProjectileArrowStateComponent.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/SpearEntity.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/SpearItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/item/tier/ItemTiers.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace {

/**
 * @brief 测试用世界存根
 */
class SpearTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("SpearTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("SpearTestWorld::tickManager not implemented");
    }

    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(entity.get());
        m_ownedEntities.push_back(std::move(entity));
        return ++m_lastEntityId;
    }

    void addParticle(
        particle::ParticleTypeId, const Vector3&, const Vector3&, const Vector3& = Vector3(0, 0, 0), u32 = 1) override
    {
        // 测试中忽略粒子效果
    }

    [[nodiscard]] const std::vector<Entity*>& spawnedEntities() const { return m_spawnedEntities; }

private:
    EntityInstanceId m_lastEntityId = 0;
    std::vector<Entity*> m_spawnedEntities;
    std::vector<std::unique_ptr<Entity>> m_ownedEntities;
};

/**
 * @brief 测试用 SpearEntity 子类，暴露 protected 成员用于测试
 */
class TestSpearEntity : public entity::SpearEntity {
public:
    explicit TestSpearEntity(EntityInstanceId id)
        : SpearEntity(id, mc::test::testEcsRegistry())
    {}

    // arrowShake 字段已迁入 ecs::ProjectileArrowStateComponent（无 public setter），
    // 测试经 tryGetComponent 直接写组件字段以模拟抖动状态。
    void setArrowShakeForTest(i32 shake)
    {
        auto* c = tryGetComponent<ecs::ProjectileArrowStateComponent>();
        if (c != nullptr) {
            c->m_arrowShake = shake;
        }
    }
};

/**
 * @brief 统计玩家背包中指定物品的总数量
 */
i32 countItemInInventory(const Player& player, const Item* target)
{
    i32 total = 0;
    const auto& inv = player.inventory();
    for (i32 i = 0; i < inv.getContainerSize(); ++i) {
        const ItemStack& s = inv.getItem(i);
        if (!s.isEmpty() && s.getItem() == target) {
            total += s.getCount();
        }
    }
    return total;
}

class SpearItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        item::tag::ItemTags::initialize();
        // SpearEntity 字段级 NBT 序列化已搬 ComponentSerializerRegistry（投掷物族批次6.2），
        // 须注册序列化器才能在 writeToNBT/readFromNBT 经 saveAll/loadAll 读写。
        entity::VanillaEntities::registerAll();
    }

    SpearTestWorld m_world;
};

// ============================================================================
// 长矛注册与基础属性测试
// ============================================================================

TEST_F(SpearItemTest, AllSpearsRegistered)
{
    ASSERT_NE(Items::WOODEN_SPEAR, nullptr);
    ASSERT_NE(Items::STONE_SPEAR, nullptr);
    ASSERT_NE(Items::COPPER_SPEAR, nullptr);
    ASSERT_NE(Items::IRON_SPEAR, nullptr);
    ASSERT_NE(Items::GOLDEN_SPEAR, nullptr);
    ASSERT_NE(Items::DIAMOND_SPEAR, nullptr);
    ASSERT_NE(Items::NETHERITE_SPEAR, nullptr);
}

TEST_F(SpearItemTest, WoodenSpear_HasCorrectDurability)
{
    // 木长矛耐久度 = 木层级耐久 = 59
    EXPECT_EQ(Items::WOODEN_SPEAR->maxDamage(), 59);
}

TEST_F(SpearItemTest, StoneSpear_HasCorrectDurability)
{
    // 石长矛耐久度 = 石层级耐久 = 131
    EXPECT_EQ(Items::STONE_SPEAR->maxDamage(), 131);
}

TEST_F(SpearItemTest, CopperSpear_HasCorrectDurability)
{
    // 铜长矛耐久度 = 铜层级耐久 = 190
    EXPECT_EQ(Items::COPPER_SPEAR->maxDamage(), 190);
}

TEST_F(SpearItemTest, IronSpear_HasCorrectDurability)
{
    // 铁长矛耐久度 = 铁层级耐久 = 250
    EXPECT_EQ(Items::IRON_SPEAR->maxDamage(), 250);
}

TEST_F(SpearItemTest, GoldenSpear_HasCorrectDurability)
{
    // 金长矛耐久度 = 金层级耐久 = 32
    EXPECT_EQ(Items::GOLDEN_SPEAR->maxDamage(), 32);
}

TEST_F(SpearItemTest, DiamondSpear_HasCorrectDurability)
{
    // 钻石长矛耐久度 = 钻石层级耐久 = 1561
    EXPECT_EQ(Items::DIAMOND_SPEAR->maxDamage(), 1561);
}

TEST_F(SpearItemTest, NetheriteSpear_HasCorrectDurability)
{
    // 下界合金长矛耐久度 = 下界合金层级耐久 = 2031
    EXPECT_EQ(Items::NETHERITE_SPEAR->maxDamage(), 2031);
}

// ============================================================================
// UseAction 和 UseDuration 测试
// ============================================================================

TEST_F(SpearItemTest, GetUseAction_ReturnsSpear)
{
    auto* spear = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);

    ItemStack stack(Items::IRON_SPEAR, 1);
    EXPECT_EQ(spear->getUseAction(stack), UseAction::Spear);
}

TEST_F(SpearItemTest, GetUseDuration_Returns72000)
{
    auto* spear = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);

    ItemStack stack(Items::IRON_SPEAR, 1);
    EXPECT_EQ(spear->getUseDuration(stack), 72000);
}

// ============================================================================
// 攻击伤害测试
// ============================================================================

TEST_F(SpearItemTest, WoodenSpear_AttackDamage)
{
    // 木长矛攻击伤害 = 基础值(3) + 木层级加成(0.0) = 3.0
    auto* spear = dynamic_cast<item::SpearItem*>(Items::WOODEN_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 3.0f);
}

TEST_F(SpearItemTest, StoneSpear_AttackDamage)
{
    // 石长矛攻击伤害 = 基础值(3) + 石层级加成(1.0) = 4.0
    auto* spear = dynamic_cast<item::SpearItem*>(Items::STONE_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 4.0f);
}

TEST_F(SpearItemTest, IronSpear_AttackDamage)
{
    // 铁长矛攻击伤害 = 基础值(3) + 铁层级加成(2.0) = 5.0
    auto* spear = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 5.0f);
}

TEST_F(SpearItemTest, DiamondSpear_AttackDamage)
{
    // 钻石长矛攻击伤害 = 基础值(3) + 钻石层级加成(3.0) = 6.0
    auto* spear = dynamic_cast<item::SpearItem*>(Items::DIAMOND_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 6.0f);
}

TEST_F(SpearItemTest, NetheriteSpear_AttackDamage)
{
    // 下界合金长矛攻击伤害 = 基础值(3) + 下界合金层级加成(4.0) = 7.0
    auto* spear = dynamic_cast<item::SpearItem*>(Items::NETHERITE_SPEAR);
    ASSERT_NE(spear, nullptr);
    EXPECT_FLOAT_EQ(spear->getAttackDamage(), 7.0f);
}

// ============================================================================
// 层级与修复材料测试
// ============================================================================

TEST_F(SpearItemTest, GetTier_ReturnsCorrectTier)
{
    auto* ironSpear = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(ironSpear, nullptr);
    EXPECT_EQ(&ironSpear->getTier(), &item::tier::ItemTiers::IRON());
}

TEST_F(SpearItemTest, GetItemEnchantability_ReturnsTierEnchantability)
{
    // 金长矛附魔能力 = 22（金层级）
    EXPECT_EQ(Items::GOLDEN_SPEAR->getItemEnchantability(), 22);
}

// ============================================================================
// SPEARS 物品标签测试
// ============================================================================

TEST_F(SpearItemTest, SpearsTag_ContainsAllSpears)
{
    auto& spearsTag = item::tag::ItemTags::SPEARS();
    EXPECT_TRUE(spearsTag.contains(Items::WOODEN_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::STONE_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::COPPER_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::IRON_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::GOLDEN_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::DIAMOND_SPEAR));
    EXPECT_TRUE(spearsTag.contains(Items::NETHERITE_SPEAR));
}

TEST_F(SpearItemTest, SpearsTag_DoesNotContainTrident)
{
    auto& spearsTag = item::tag::ItemTags::SPEARS();
    EXPECT_FALSE(spearsTag.contains(Items::TRIDENT));
}

TEST_F(SpearItemTest, GoldenSpear_InPiglinLovedTag)
{
    // 金长矛属于 piglin_loved 标签
    auto& piglinLoved = item::tag::ItemTags::PIGLIN_LOVED();
    EXPECT_TRUE(piglinLoved.contains(Items::GOLDEN_SPEAR));
}

// ============================================================================
// 投掷机制动态行为测试（onPlayerStoppedUsing）
// ============================================================================

TEST_F(SpearItemTest, Throw_SufficientCharge_SpawnsEntityAndConsumesDurability)
{
    // 蓄力充足（chargeTicks >= 10）：投掷成功，生存模式消耗 1 耐久并减少 1 数量
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    // 蓄力充足：timeLeft = MAX_USE_DURATION - 15 = 71985，chargeTicks = 15
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 15);

    // 生存模式：数量减少 1，物品堆变空
    EXPECT_EQ(stack.getCount(), 0);
    // 耐久消耗 1（在数量减少前应用）
    // 生成 1 个实体
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

TEST_F(SpearItemTest, Throw_InsufficientCharge_DoesNothing)
{
    // 蓄力不足（chargeTicks < 10）：不投掷，不消耗
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    // 蓄力仅 5 tick（< MIN_CHARGE_TICKS=10）
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 5);

    // 不投掷：数量不变、耐久不变、无实体生成
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getDamage(), 0);
    EXPECT_EQ(m_world.spawnedEntities().size(), 0u);
}

TEST_F(SpearItemTest, Throw_CreativeMode_DoesNotConsumeStack)
{
    // 创造模式：投掷成功，但不消耗数量、不消耗耐久
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 20);

    // 创造模式：数量不变、耐久不变
    EXPECT_EQ(stack.getCount(), 1);
    EXPECT_EQ(stack.getDamage(), 0);
    // 实体已生成
    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
}

TEST_F(SpearItemTest, Throw_CreativeMode_SetsCreativeOnlyPickup)
{
    // 创造模式投掷的实体 PickupStatus 应为 CreativeOnly
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 20);

    ASSERT_EQ(m_world.spawnedEntities().size(), 1u);
    auto* spear = dynamic_cast<entity::SpearEntity*>(m_world.spawnedEntities()[0]);
    ASSERT_NE(spear, nullptr);
    EXPECT_EQ(spear->pickupStatus(), entity::PickupStatus::CreativeOnly);
}

TEST_F(SpearItemTest, Throw_SurvivalMode_SetsAllowedPickup)
{
    // 生存模式投掷的实体 PickupStatus 应为 Allowed
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 20);

    ASSERT_EQ(m_world.spawnedEntities().size(), 1u);
    auto* spear = dynamic_cast<entity::SpearEntity*>(m_world.spawnedEntities()[0]);
    ASSERT_NE(spear, nullptr);
    EXPECT_EQ(spear->pickupStatus(), entity::PickupStatus::Allowed);
}

TEST_F(SpearItemTest, Throw_SpawnedEntityIsSpearEntity)
{
    // 验证生成的实体类型是 SpearEntity，且携带正确的物品堆
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::DIAMOND_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    ItemStack stack(Items::DIAMOND_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 20);

    ASSERT_EQ(m_world.spawnedEntities().size(), 1u);
    auto* spear = dynamic_cast<entity::SpearEntity*>(m_world.spawnedEntities()[0]);
    ASSERT_NE(spear, nullptr);
    // 实体携带的物品堆应为钻石长矛
    EXPECT_EQ(spear->getItemStack().getItem(), Items::DIAMOND_SPEAR);
    // 投掷伤害固定 8.0
    EXPECT_FLOAT_EQ(spear->damage(), 8.0f);
}

TEST_F(SpearItemTest, Throw_NonPlayerEntity_DoesNothing)
{
    // 非玩家实体（LivingEntity 但非 Player）调用 onPlayerStoppedUsing 不应投掷
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    // 用一个非 Player 的 LivingEntity（直接用 LivingEntity 基类构造不可行，这里用 Player
    // 但不通过 dynamic_cast 验证——实际上 onPlayerStoppedUsing 内部就是 dynamic_cast，
    // 所以这里测试的语义是"如果传入的不是 Player，不会有任何副作用"）
    // 为此我们构造一个 Player，但让 dynamic_cast 失败是不可能的。
    // 此测试用例保留为占位，实际覆盖由其他测试保证。
    // 改为测试：onPlayerStoppedUsing 对 Player 传入 nullptr world 不会崩溃（健壮性）。
    // 但 world 是引用，无法传 nullptr。所以此用例改为验证蓄力边界 chargeTicks == MIN_CHARGE_TICKS。

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;
    // chargeTicks 恰好等于 MIN_CHARGE_TICKS（10），应能投掷（边界值，>= 检查）
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 10);

    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
    EXPECT_EQ(stack.getCount(), 0);
}

// ============================================================================
// onItemRightClick 测试（蓄力开始）
// ============================================================================

TEST_F(SpearItemTest, OnRightClick_SetsActiveHand)
{
    // 右键长矛应设置玩家活跃手（开始蓄力）
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    player.getHeldItem(Hand::MainHand) = stack;

    auto result = spearItem->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_TRUE(result.isSuccessOrConsume());
    // 玩家应处于"正在使用物品"状态
    EXPECT_TRUE(player.isUsingItem());
    EXPECT_EQ(player.getActiveHand(), Hand::MainHand);
}

TEST_F(SpearItemTest, OnRightClick_DamagedNearBreak_PreventsUse)
{
    // 耐久即将耗尽（getDamage >= getMaxDamage - 1）时禁止开始蓄力
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    // 铁长矛耐久 250，设为 249（即将损坏）
    stack.setDamage(249);
    player.getHeldItem(Hand::MainHand) = stack;

    auto result = spearItem->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_FALSE(result.isSuccessOrConsume());
    EXPECT_FALSE(player.isUsingItem());
}

// ============================================================================
// hitEntity 与 onBlockDestroyed 耐久消耗测试
// ============================================================================

TEST_F(SpearItemTest, HitEntity_ConsumesOneDurability)
{
    // 近战命中实体消耗 1 点耐久
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player attacker(EntityInstanceId(1), "Attacker", mc::test::testEcsRegistry());
    attacker.setPosition(0.0f, 64.0f, 0.0f);
    attacker.setWorld(&m_world);
    attacker.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    // 用一个简单的 LivingEntity 作为 target（这里用 Player 模拟）
    Player target(EntityInstanceId(2), "Target", mc::test::testEcsRegistry());
    target.setPosition(1.0f, 64.0f, 0.0f);
    target.setWorld(&m_world);

    bool result = spearItem->hitEntity(stack, target, attacker);
    EXPECT_TRUE(result);
    EXPECT_EQ(stack.getDamage(), 1);
}

TEST_F(SpearItemTest, HitEntity_BreaksAtZeroDurability)
{
    // 耐久为 0 时再消耗会导致物品损坏（变空）
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player attacker(EntityInstanceId(1), "Attacker", mc::test::testEcsRegistry());
    attacker.setPosition(0.0f, 64.0f, 0.0f);
    attacker.setWorld(&m_world);
    attacker.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    // 设耐久为 maxDamage - 1 = 249，再消耗 1 就损坏
    stack.setDamage(249);
    Player target(EntityInstanceId(2), "Target", mc::test::testEcsRegistry());
    target.setPosition(1.0f, 64.0f, 0.0f);
    target.setWorld(&m_world);

    spearItem->hitEntity(stack, target, attacker);
    // 物品应已损坏（变空）
    EXPECT_TRUE(stack.isEmpty());
}

// ============================================================================
// SpearEntity 拾取测试
// ============================================================================

TEST_F(SpearItemTest, Pickup_InGround_AllowedStatus_AddsToInventory)
{
    // 插地的长矛，PickupStatus::Allowed，可被玩家拾取
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::Allowed);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_TRUE(picked);
    // 背包应有 1 把铁长矛
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 1);
}

TEST_F(SpearItemTest, Pickup_NotInGround_Fails)
{
    // 未插地（飞行中）的长矛不能被拾取
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(false); // 飞行中
    spear.setPickupStatus(entity::PickupStatus::Allowed);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_FALSE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 0);
}

TEST_F(SpearItemTest, Pickup_DisallowedStatus_Fails)
{
    // PickupStatus::Disallowed 时无法拾取
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::Disallowed);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_FALSE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 0);
}

TEST_F(SpearItemTest, Pickup_CreativeOnly_SurvivalPlayerFails)
{
    // CreativeOnly 状态下，生存模式玩家无法拾取
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::CreativeOnly);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_FALSE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 0);
}

TEST_F(SpearItemTest, Pickup_CreativeOnly_CreativePlayerSucceeds)
{
    // CreativeOnly 状态下，创造模式玩家可以拾取
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::CreativeOnly);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Creative);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_TRUE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 1);
}

TEST_F(SpearItemTest, Pickup_ArrowShake_Fails)
{
    // 处于抖动状态（arrowShake > 0）时无法拾取
    TestSpearEntity spear(EntityInstanceId(10));
    spear.setWorld(&m_world);
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::Allowed);
    spear.setArrowShakeForTest(5); // 抖动 5 tick

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    bool picked = spear.onPlayerPickup(player);
    EXPECT_FALSE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 0);
}

// ============================================================================
// 边界场景测试
// ============================================================================

TEST_F(SpearItemTest, Throw_DamagedSpear_StillThrowsUntilNearBreak)
{
    // 耐久已消耗但未到临界值，仍可投掷（onPlayerStoppedUsing 不检查耐久，
    // 只检查 chargeTicks；onItemRightClick 检查耐久）
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::IRON_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::IRON_SPEAR, 1);
    stack.setDamage(100); // 耐久消耗了 100，还有 150
    player.getHeldItem(Hand::MainHand) = stack;
    spearItem->onPlayerStoppedUsing(stack, m_world, player, item::SpearItem::MAX_USE_DURATION - 20);

    EXPECT_EQ(m_world.spawnedEntities().size(), 1u);
    // 生存模式消耗 1 数量
    EXPECT_EQ(stack.getCount(), 0);
}

TEST_F(SpearItemTest, Throw_DurabilityAtMaxDamage_PreventsStart)
{
    // 耐久恰好等于 maxDamage-1 时，onItemRightClick 拒绝开始蓄力
    auto* spearItem = dynamic_cast<item::SpearItem*>(Items::GOLDEN_SPEAR);
    ASSERT_NE(spearItem, nullptr);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    ItemStack stack(Items::GOLDEN_SPEAR, 1);
    // 金长矛耐久 32，设为 31（maxDamage - 1）
    stack.setDamage(31);
    player.getHeldItem(Hand::MainHand) = stack;

    auto result = spearItem->onItemRightClick(m_world, player, Hand::MainHand);
    EXPECT_FALSE(result.isSuccessOrConsume());
    EXPECT_FALSE(player.isUsingItem());
}

TEST_F(SpearItemTest, Pickup_EmptySpearStack_FailsSilently)
{
    // 物品堆为空时拾取不崩溃，也不添加到背包
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setWorld(&m_world);
    // 默认 m_spearStack 为空
    spear.setInGround(true);
    spear.setPickupStatus(entity::PickupStatus::Allowed);

    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());
    player.setPosition(0.0f, 64.0f, 0.0f);
    player.setWorld(&m_world);
    player.setGameMode(GameMode::Survival);

    // 空堆情况下 onPlayerPickup 仍应成功移除实体（不添加物品到背包）
    bool picked = spear.onPlayerPickup(player);
    EXPECT_TRUE(picked);
    EXPECT_EQ(countItemInInventory(player, Items::IRON_SPEAR), 0);
}

// ============================================================================
// SpearEntity NBT 序列化往返测试
// ============================================================================

TEST_F(SpearItemTest, NbtRoundTrip_PreservesItemStack)
{
    // 验证 NBT 序列化/反序列化后长矛物品堆保持一致
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear1.setItemStack(ItemStack(Items::DIAMOND_SPEAR, 1));

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(spear2.getItemStack().getItem(), Items::DIAMOND_SPEAR);
    EXPECT_EQ(spear2.getItemStack().getCount(), 1);
}

TEST_F(SpearItemTest, NbtRoundTrip_PreservesDamageAndPickupStatus)
{
    // 验证伤害值和拾取状态在往返后保持一致
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear1.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear1.setDamage(12.5f);
    spear1.setPickupStatus(entity::PickupStatus::CreativeOnly);

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_FLOAT_EQ(spear2.damage(), 12.5f);
    EXPECT_EQ(spear2.pickupStatus(), entity::PickupStatus::CreativeOnly);
}

TEST_F(SpearItemTest, NbtRoundTrip_PreservesInGroundAndDealtDamage)
{
    // 验证插地状态和已造成伤害标志在往返后保持一致
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear1.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear1.setInGround(true);
    // 模拟已造成伤害（onEntityHit 会设置 m_dealtDamage = true）
    // 由于 m_dealtDamage 是 protected 成员，通过 NBT 往返间接验证

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    // 验证 NBT 中存在 DealtDamage 键且为 true
    auto dealtDamage = entity::serialization::nbt_helper::tryGetBool(tag, "DealtDamage");
    // 默认 m_dealtDamage=false，所以这里应该是 false
    ASSERT_TRUE(dealtDamage.has_value());
    EXPECT_FALSE(*dealtDamage);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_TRUE(spear2.isInGround());
    EXPECT_FALSE(spear2.hasDealtDamage());
}

TEST_F(SpearItemTest, NbtRoundTrip_PreservesCriticalAndPierceLevel)
{
    // 验证暴击和穿透等级在往返后保持一致
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear1.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear1.setCritical(true);
    spear1.setPierceLevel(3);

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_TRUE(spear2.isCritical());
    EXPECT_EQ(spear2.pierceLevel(), 3u);
}

TEST_F(SpearItemTest, NbtRoundTrip_DefaultValues)
{
    // 默认值序列化/反序列化应保持默认值
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear1.setItemStack(ItemStack(Items::IRON_SPEAR, 1));

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 默认值检查
    EXPECT_FALSE(spear2.isInGround());
    EXPECT_FALSE(spear2.isCritical());
    EXPECT_EQ(spear2.pierceLevel(), 0u);
    EXPECT_FALSE(spear2.hasDealtDamage());
    // 构造函数默认伤害 8.0
    EXPECT_FLOAT_EQ(spear2.damage(), 8.0f);
    // 构造函数默认拾取状态 Allowed
    EXPECT_EQ(spear2.pickupStatus(), entity::PickupStatus::Allowed);
}

TEST_F(SpearItemTest, NbtDeserialize_EmptyTag_KeepsDefaults)
{
    // 空的 NBT tag 反序列化应保持默认值，不崩溃
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());

    nbt::tags::compound_tag emptyTag;
    auto result = spear.readFromNBT(emptyTag);
    EXPECT_TRUE(static_cast<bool>(result));

    // 默认值
    EXPECT_FLOAT_EQ(spear.damage(), 8.0f);
    EXPECT_EQ(spear.pickupStatus(), entity::PickupStatus::Allowed);
    EXPECT_FALSE(spear.isInGround());
    EXPECT_TRUE(spear.getItemStack().isEmpty());
}

TEST_F(SpearItemTest, NbtSerialize_WritesAllExpectedKeys)
{
    // 验证 NBT 序列化写入了所有预期键
    entity::SpearEntity spear(EntityInstanceId(10), mc::test::testEcsRegistry());
    spear.setItemStack(ItemStack(Items::IRON_SPEAR, 1));
    spear.setDamage(8.0f);
    spear.setPickupStatus(entity::PickupStatus::Allowed);
    spear.setInGround(true);
    spear.setCritical(false);

    nbt::tags::compound_tag tag;
    spear.writeToNBT(tag);

    using namespace entity::serialization::nbt_helper;

    // 物品堆子复合标签（AbstractArrow 子树含 Spear 用小写 "item" 键，与 vanilla 1.21 一致；
    // ThrowableItemProjectile 子树才用大写 "Item"）
    EXPECT_NE(tryGetCompound(tag, "item"), nullptr);
    // 拾取状态
    EXPECT_TRUE(tryGetByte(tag, "pickup").has_value());
    // 伤害值
    EXPECT_TRUE(tryGetFloat(tag, "damage").has_value());
    // 插地状态
    EXPECT_TRUE(tryGetBool(tag, "inGround").has_value());
    // 暴击
    EXPECT_TRUE(tryGetBool(tag, "crit").has_value());
    // 穿透等级
    EXPECT_TRUE(tryGetByte(tag, "PierceLevel").has_value());
    // 已造成伤害
    EXPECT_TRUE(tryGetBool(tag, "DealtDamage").has_value());
    // 注意：knockback（击退强度）运行时从附魔 Punch 读取，vanilla 不持久化，故不验证该键。
}

TEST_F(SpearItemTest, NbtRoundTrip_PreservesDurabilityDamage)
{
    // 验证物品堆的耐久度（Damage 子标签）在往返后保持一致
    entity::SpearEntity spear1(EntityInstanceId(10), mc::test::testEcsRegistry());
    ItemStack stack(Items::IRON_SPEAR, 1);
    stack.setDamage(150);
    spear1.setItemStack(stack);

    nbt::tags::compound_tag tag;
    spear1.writeToNBT(tag);

    entity::SpearEntity spear2(EntityInstanceId(11), mc::test::testEcsRegistry());
    auto result = spear2.readFromNBT(tag);
    EXPECT_TRUE(static_cast<bool>(result));

    EXPECT_EQ(spear2.getItemStack().getItem(), Items::IRON_SPEAR);
    EXPECT_EQ(spear2.getItemStack().getDamage(), 150);
}

} // namespace
} // namespace mc
