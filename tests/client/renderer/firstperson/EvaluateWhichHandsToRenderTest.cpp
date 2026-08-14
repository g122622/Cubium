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
 */

#include "client/renderer/trident/firstperson/FirstPersonTransforms.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/fluid/FluidRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::trident::firstperson;

// ============================================================================
// evaluateWhichHandsToRender 测试（ItemInHandRenderer.evaluateWhichHandsToRender）
// ============================================================================

class EvaluateWhichHandsToRenderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();

        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer", mc::test::testEcsRegistry());
    }

    void TearDown() override { m_player.reset(); }

    /// 主手装备并触发使用（模拟拉弓/装填弩）。
    void useMainHand(const ItemStack& stack)
    {
        m_player->setMainHandItem(stack);
        m_player->setActiveHand(Hand::MainHand);
    }

    /// 副手装备并触发使用。
    void useOffHand(const ItemStack& stack)
    {
        m_player->setOffHandItem(stack);
        m_player->setActiveHand(Hand::OffHand);
    }

    std::unique_ptr<Player> m_player;
};

// ---------- 无弓/弩 ----------

TEST_F(EvaluateWhichHandsToRenderTest, NoBowOrCrossbowRendersBothHands)
{
    m_player->setMainHandItem(ItemStack(Items::DIAMOND_SWORD));
    m_player->setOffHandItem(ItemStack(Items::SHIELD));
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderBothHands);
}

TEST_F(EvaluateWhichHandsToRenderTest, EmptyHandsRendersBothHands)
{
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderBothHands);
}

// ---------- 持弓使用中 ----------

TEST_F(EvaluateWhichHandsToRenderTest, UsingBowMainHandRendersMainHandOnly)
{
    useMainHand(ItemStack(Items::BOW));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderMainHandOnly);
}

TEST_F(EvaluateWhichHandsToRenderTest, UsingBowOffHandRendersOffHandOnly)
{
    useOffHand(ItemStack(Items::BOW));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderOffHandOnly);
}

// ---------- 持弩使用中（装填） ----------

TEST_F(EvaluateWhichHandsToRenderTest, UsingCrossbowMainHandRendersMainHandOnly)
{
    useMainHand(ItemStack(Items::CROSSBOW));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderMainHandOnly);
}

TEST_F(EvaluateWhichHandsToRenderTest, UsingCrossbowOffHandRendersOffHandOnly)
{
    useOffHand(ItemStack(Items::CROSSBOW));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderOffHandOnly);
}

// ---------- 持弓/弩但未使用 ----------

TEST_F(EvaluateWhichHandsToRenderTest, HoldingBowNotUsingRendersBothHands)
{
    m_player->setMainHandItem(ItemStack(Items::BOW));
    EXPECT_FALSE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderBothHands);
}

TEST_F(EvaluateWhichHandsToRenderTest, HoldingChargedCrossbowMainHandRendersMainHandOnly)
{
    // 已装填弩在主手、未使用 → 仅主手（isChargedCrossbow(mainHand) 分支）。
    ItemStack chargedCrossbow(Items::CROSSBOW);
    item::CrossbowItem::setCharged(chargedCrossbow, true);
    ASSERT_TRUE(item::CrossbowItem::isCharged(chargedCrossbow));
    m_player->setMainHandItem(chargedCrossbow);
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderMainHandOnly);
}

TEST_F(EvaluateWhichHandsToRenderTest, HoldingUnchargedCrossbowMainHandRendersBothHands)
{
    m_player->setMainHandItem(ItemStack(Items::CROSSBOW));
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderBothHands);
}

// ---------- 主手用非弓弩 + 副手已装填弩 ----------

TEST_F(EvaluateWhichHandsToRenderTest, UsingNonBowMainHandWithChargedCrossbowOffHandRendersMainHandOnly)
{
    // 主手使用苹果（非弓弩）+ 副手已装填弩 → 持有弩(holdsBow) + 使用中 + 使用物品非弓弩
    // + 使用手是主手 + 副手已装填弩 → 仅主手（selectionUsingItemWhileHoldingBowLike
    // 中 interactionhand==MAIN_HAND && isChargedCrossbow(offhand) 分支）。
    ItemStack chargedCrossbow(Items::CROSSBOW);
    item::CrossbowItem::setCharged(chargedCrossbow, true);
    m_player->setOffHandItem(chargedCrossbow);
    useMainHand(ItemStack(Items::APPLE));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderMainHandOnly);
}

TEST_F(EvaluateWhichHandsToRenderTest, UsingNonBowOffHandWithChargedCrossbowOffHandRendersBothHands)
{
    // 使用手是副手（非弓弩）→ 不满足"主手使用+副手已装填弩" → 双手。
    // 主手装填弩使 holdsBow=true，副手用苹果。
    ItemStack chargedCrossbow(Items::CROSSBOW);
    item::CrossbowItem::setCharged(chargedCrossbow, true);
    m_player->setMainHandItem(chargedCrossbow);
    useOffHand(ItemStack(Items::APPLE));
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderBothHands);
}

TEST_F(EvaluateWhichHandsToRenderTest, ChargedCrossbowMainHandWithBowOffHandRendersMainHandOnly)
{
    // 主手已装填弩 + 副手弓 + 未使用 → 主手是已装填弩 → 仅主手。
    ItemStack chargedCrossbow(Items::CROSSBOW);
    item::CrossbowItem::setCharged(chargedCrossbow, true);
    m_player->setMainHandItem(chargedCrossbow);
    m_player->setOffHandItem(ItemStack(Items::BOW));
    EXPECT_EQ(evaluateWhichHandsToRender(*m_player), HandRenderSelection::RenderMainHandOnly);
}

// ---------- shouldRenderHand 辅助 ----------

TEST(HandRenderSelectionHelperTest, ShouldRenderHandBothHands)
{
    EXPECT_TRUE(shouldRenderHand(HandRenderSelection::RenderBothHands, Hand::MainHand));
    EXPECT_TRUE(shouldRenderHand(HandRenderSelection::RenderBothHands, Hand::OffHand));
}

TEST(HandRenderSelectionHelperTest, ShouldRenderHandMainOnly)
{
    EXPECT_TRUE(shouldRenderHand(HandRenderSelection::RenderMainHandOnly, Hand::MainHand));
    EXPECT_FALSE(shouldRenderHand(HandRenderSelection::RenderMainHandOnly, Hand::OffHand));
}

TEST(HandRenderSelectionHelperTest, ShouldRenderHandOffOnly)
{
    EXPECT_FALSE(shouldRenderHand(HandRenderSelection::RenderOffHandOnly, Hand::MainHand));
    EXPECT_TRUE(shouldRenderHand(HandRenderSelection::RenderOffHandOnly, Hand::OffHand));
}

// ============================================================================
// isScoping 测试（LivingEntity.isScoping）
// 项目无 Spyglass 物品，UseAction::Spyglass 通过自定义测试物品注入。
// ============================================================================

namespace {

/// 测试用望远镜物品：getUseAction 返回 Spyglass，getUseDuration 返回正值以允许 setActiveHand。
class TestSpyglassItem : public Item {
public:
    TestSpyglassItem()
        : Item(ItemProperties().maxStackSize(1))
    {}

    [[nodiscard]] UseAction getUseAction(const ItemStack&) const override { return UseAction::Spyglass; }
    [[nodiscard]] i32 getUseDuration(const ItemStack&) const override { return 1200; }
};

} // namespace

class IsScopingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();

        m_player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer", mc::test::testEcsRegistry());
        m_spyglass = std::make_unique<TestSpyglassItem>();
    }

    void TearDown() override
    {
        m_player.reset();
        m_spyglass.reset();
    }

    std::unique_ptr<Player> m_player;
    std::unique_ptr<TestSpyglassItem> m_spyglass;
};

TEST_F(IsScopingTest, NotUsingItemIsNotScoping)
{
    EXPECT_FALSE(m_player->isScoping());
}

TEST_F(IsScopingTest, UsingNonSpyglassIsNotScoping)
{
    m_player->setMainHandItem(ItemStack(Items::BOW));
    m_player->setActiveHand(Hand::MainHand);
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_FALSE(m_player->isScoping());
}

TEST_F(IsScopingTest, UsingSpyglassIsScoping)
{
    m_player->setMainHandItem(ItemStack(m_spyglass.get()));
    m_player->setActiveHand(Hand::MainHand);
    ASSERT_TRUE(m_player->isUsingItem());
    EXPECT_TRUE(m_player->isScoping());
}

TEST_F(IsScopingTest, StopUsingSpyglassIsNotScoping)
{
    m_player->setMainHandItem(ItemStack(m_spyglass.get()));
    m_player->setActiveHand(Hand::MainHand);
    ASSERT_TRUE(m_player->isScoping());
    m_player->stopActiveHand();
    EXPECT_FALSE(m_player->isScoping());
}
