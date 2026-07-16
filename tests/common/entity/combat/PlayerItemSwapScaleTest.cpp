/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 */

#include <gtest/gtest.h>

#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/entities/player/Player.hpp"
#include "world/fluid/FluidRegistry.hpp"

using namespace mc;

/**
 * @brief 物品切换缩放（getItemSwapScale / itemSwapTicker）测试
 *
 * 对齐 MC 1.21.11 Player.getItemSwapScale / Player.tick：
 * - itemSwapTicker 每 tick 递增；
 * - 仅在主手物品种类（isSameItem）切换时重置为 0，与攻击冷却 ticker 解耦；
 * - getItemSwapScale = clamp((itemSwapTicker + adjust) / currentItemAttackStrengthDelay, 0, 1)。
 */
class PlayerItemSwapScaleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();

        m_player = std::make_unique<Player>(static_cast<EntityId>(1), "TestPlayer");
    }

    void TearDown() override { m_player.reset(); }

    std::unique_ptr<Player> m_player;
};

TEST_F(PlayerItemSwapScaleTest, InitialScaleIsZeroBeforeTick)
{
    // itemSwapTicker 初始为 0，adjust 0 → 进度 0。
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 0.0f);
}

TEST_F(PlayerItemSwapScaleTest, ScaleGrowsOverTicks)
{
    // tick 一次后 itemSwapTicker=1，进度应 > 0 且 < 1（默认 attackSpeed=4 → 周期 5 ticks）。
    m_player->tick();
    const f32 s = m_player->getItemSwapScale(0.0f);
    EXPECT_GT(s, 0.0f);
    EXPECT_LT(s, 1.0f);
}

TEST_F(PlayerItemSwapScaleTest, ScaleSaturatesToOne)
{
    // 充分 tick 后进度饱和到 1。
    for (int i = 0; i < 30; ++i) {
        m_player->tick();
    }
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);
}

TEST_F(PlayerItemSwapScaleTest, AdjustTicksExtendsProgress)
{
    // tick 一次 (itemSwapTicker=1)，adjust=1 等价于 itemSwapTicker=2 的进度。
    m_player->tick();
    const f32 base = m_player->getItemSwapScale(0.0f);
    const f32 adjusted = m_player->getItemSwapScale(1.0f);
    EXPECT_GT(adjusted, base);
}

TEST_F(PlayerItemSwapScaleTest, SwapResetsTickerOnlyOnItemTypeChange)
{
    // 充分 tick 达到饱和。
    for (int i = 0; i < 30; ++i) {
        m_player->tick();
    }
    ASSERT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);

    // 切换到不同物品种类 → itemSwapTicker 归零。
    const ItemStack sword(Items::DIAMOND_SWORD);
    m_player->inventory().getSelectedStackRef() = sword;
    m_player->tick();
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 0.0f);

    // 再次切到不同种类 → 仍归零。
    const ItemStack pickaxe(Items::DIAMOND_PICKAXE);
    m_player->inventory().getSelectedStackRef() = pickaxe;
    m_player->tick();
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 0.0f);
}

TEST_F(PlayerItemSwapScaleTest, SameItemDifferentCountDoesNotReset)
{
    // 先填充一个物品并 tick 到饱和。
    m_player->inventory().getSelectedStackRef() = ItemStack(Items::DIAMOND_SWORD, 1);
    for (int i = 0; i < 30; ++i) {
        m_player->tick();
    }
    ASSERT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);

    // 改变数量但不改物品种类 → ticker 不应归零（对应 MC isSameItem 判定）。
    m_player->inventory().getSelectedStackRef() = ItemStack(Items::DIAMOND_SWORD, 2);
    m_player->tick();
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);
}

TEST_F(PlayerItemSwapScaleTest, DecoupledFromAttackCooldown)
{
    // 攻击会重置攻击冷却 ticker，但不应重置 itemSwapTicker。
    // 先 tick 到饱和。
    for (int i = 0; i < 30; ++i) {
        m_player->tick();
    }
    ASSERT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);

    // 触发攻击冷却重置（攻击后），但无物品切换 → itemSwapTicker 仍饱和。
    m_player->resetCooldown();
    m_player->tick();
    EXPECT_FLOAT_EQ(m_player->getItemSwapScale(0.0f), 1.0f);
}
