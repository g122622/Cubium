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
 * @file PlayerInteractionRangeTest.cpp
 * @brief Player 交互距离属性测试
 *
 * 测试 Player 的方块/实体交互距离属性：
 * - registerAttributes() 注册 generic.block_interaction_range / generic.entity_interaction_range
 * - 生存/冒险模式默认值 4.5 / 3.0
 * - 创造模式通过修饰符 +0.5 / +2.0 达到 5.0 / 5.0
 * - setGameMode() 切换模式时刷新修饰符
 * - isWithinBlockInteractionRange / isWithinEntityInteractionRange 距离判定
 *
 * 参考 MC 1.21.11: ServerPlayer.updatePlayerAttributes() / Player.blockInteractionRange() /
 * Player.entityInteractionRange() / Player.isWithinBlockInteractionRange() /
 * Player.isWithinEntityInteractionRange()
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/AttributeModifierUUIDs.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/block/BlockPos.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::attribute;

namespace {

/**
 * @brief Player 交互距离测试夹具
 */
class PlayerInteractionRangeTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer", mc::test::testEcsRegistry()); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

} // namespace

// ========== 属性注册测试 ==========

TEST_F(PlayerInteractionRangeTest, RegistersBlockInteractionRangeAttribute)
{
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::BLOCK_INTERACTION_RANGE));
}

TEST_F(PlayerInteractionRangeTest, RegistersEntityInteractionRangeAttribute)
{
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::ENTITY_INTERACTION_RANGE));
}

// ========== 默认值测试（生存模式） ==========

TEST_F(PlayerInteractionRangeTest, SurvivalBlockInteractionRangeDefault)
{
    // 生存模式默认 4.5
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 4.5);
}

TEST_F(PlayerInteractionRangeTest, SurvivalEntityInteractionRangeDefault)
{
    // 生存模式默认 3.0
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 3.0);
}

// ========== 创造模式修饰符测试 ==========

TEST_F(PlayerInteractionRangeTest, CreativeModeAddsBlockInteractionRangeModifier)
{
    player->setGameMode(GameMode::Creative);
    // 创造模式 4.5 + 0.5 = 5.0
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);
    // 修饰器应存在
    EXPECT_TRUE(player->attributes().hasModifier(
        Attributes::BLOCK_INTERACTION_RANGE, uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID));
}

TEST_F(PlayerInteractionRangeTest, CreativeModeAddsEntityInteractionRangeModifier)
{
    player->setGameMode(GameMode::Creative);
    // 创造模式 3.0 + 2.0 = 5.0
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 5.0);
    // 修饰器应存在
    EXPECT_TRUE(player->attributes().hasModifier(
        Attributes::ENTITY_INTERACTION_RANGE, uuids::CREATIVE_ENTITY_INTERACTION_RANGE_UUID));
}

// ========== 模式切换测试 ==========

TEST_F(PlayerInteractionRangeTest, SwitchingFromCreativeToSurvivalRemovesModifiers)
{
    // 切到创造模式添加修饰符
    player->setGameMode(GameMode::Creative);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 5.0);

    // 切回生存模式应移除修饰符
    player->setGameMode(GameMode::Survival);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 4.5);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 3.0);
    EXPECT_FALSE(player->attributes().hasModifier(
        Attributes::BLOCK_INTERACTION_RANGE, uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID));
    EXPECT_FALSE(player->attributes().hasModifier(
        Attributes::ENTITY_INTERACTION_RANGE, uuids::CREATIVE_ENTITY_INTERACTION_RANGE_UUID));
}

TEST_F(PlayerInteractionRangeTest, SwitchingFromCreativeToAdventureRemovesModifiers)
{
    player->setGameMode(GameMode::Creative);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);

    // 冒险模式同样不应有创造修饰符
    player->setGameMode(GameMode::Adventure);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 4.5);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 3.0);
    EXPECT_FALSE(player->attributes().hasModifier(
        Attributes::BLOCK_INTERACTION_RANGE, uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID));
}

TEST_F(PlayerInteractionRangeTest, SwitchingFromCreativeToSpectatorRemovesModifiers)
{
    player->setGameMode(GameMode::Creative);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);

    // 旁观模式同样不应有创造修饰符
    player->setGameMode(GameMode::Spectator);
    EXPECT_FALSE(player->attributes().hasModifier(
        Attributes::BLOCK_INTERACTION_RANGE, uuids::CREATIVE_BLOCK_INTERACTION_RANGE_UUID));
    EXPECT_FALSE(player->attributes().hasModifier(
        Attributes::ENTITY_INTERACTION_RANGE, uuids::CREATIVE_ENTITY_INTERACTION_RANGE_UUID));
}

// ========== 幂等性测试 ==========

TEST_F(PlayerInteractionRangeTest, SetGameModeCreativeIsIdempotent)
{
    // 重复设置创造模式不应叠加修饰符
    player->setGameMode(GameMode::Creative);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 5.0);

    player->setGameMode(GameMode::Creative);
    player->setGameMode(GameMode::Creative);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 5.0);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 5.0);
}

// ========== isWithinBlockInteractionRange 测试 ==========

TEST_F(PlayerInteractionRangeTest, IsWithinBlockInteractionRange_WithinRange)
{
    // 玩家眼睛在 (0.5, 1.62, 0.5)，方块在 (0, 0, 0)
    // 方块 AABB: [0,1]x[0,1]x[0,1]，最近点 (0.5, 1.0, 0.5)
    // 距离平方 = 0 + 0.62^2 + 0 ≈ 0.3844
    player->setPosition(0.5f, 0.0f, 0.5f);
    BlockPos pos(0, 0, 0);

    // 生存模式 4.5 格，padding=0 → 应在范围内
    EXPECT_TRUE(player->isWithinBlockInteractionRange(pos, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinBlockInteractionRange_OutOfRange)
{
    // 玩家眼睛在 (0.5, 21.62, 0.5)，方块在 (0, 0, 0)
    // 距离 ≈ 20.62，远超 4.5
    player->setPosition(0.5f, 20.0f, 0.5f);
    BlockPos pos(0, 0, 0);

    EXPECT_FALSE(player->isWithinBlockInteractionRange(pos, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinBlockInteractionRange_CreativeExtendsRange)
{
    // 玩家眼睛 (0.5, eye_y, 0.5)，方块 (0, 0, 0) 顶部 y=1.0
    // 距离 = eye_y - 1.0 = (playerY + 1.62) - 1.0 = playerY + 0.62
    BlockPos pos(0, 0, 0);

    // playerY = 3.5 → 距离 4.12 < 4.5，生存模式可达
    player->setPosition(0.5f, 3.5f, 0.5f);
    EXPECT_TRUE(player->isWithinBlockInteractionRange(pos, 0.0));

    // playerY = 4.0 → 距离 4.62 > 4.5（生存不够），但 < 5.0（创造可达）
    player->setPosition(0.5f, 4.0f, 0.5f);
    EXPECT_FALSE(player->isWithinBlockInteractionRange(pos, 0.0));
    player->setGameMode(GameMode::Creative);
    EXPECT_TRUE(player->isWithinBlockInteractionRange(pos, 0.0));

    // playerY = 4.5 → 距离 5.12 > 5.0，创造模式也不可达
    player->setPosition(0.5f, 4.5f, 0.5f);
    EXPECT_FALSE(player->isWithinBlockInteractionRange(pos, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinBlockInteractionRange_PaddingExtendsRange)
{
    // 玩家眼睛 (0.5, 4.62, 0.5)（playerY=3.0），方块 (0, 0, 0) 顶部 y=1.0
    // 距离 = 3.62 < 4.5（生存可达）
    BlockPos pos(0, 0, 0);
    player->setPosition(0.5f, 3.0f, 0.5f);
    EXPECT_TRUE(player->isWithinBlockInteractionRange(pos, 0.0));

    // 移远到 playerY=4.0，距离 4.62 > 4.5（无 padding 不够），加 padding 1.0 → 5.5 > 4.62 ✓
    player->setPosition(0.5f, 4.0f, 0.5f);
    EXPECT_FALSE(player->isWithinBlockInteractionRange(pos, 0.0));
    EXPECT_TRUE(player->isWithinBlockInteractionRange(pos, 1.0));
}

// ========== isWithinEntityInteractionRange 测试 ==========

TEST_F(PlayerInteractionRangeTest, IsWithinEntityInteractionRange_AABB_WithinRange)
{
    // 玩家眼睛 (0.5, 1.62, 0.5)，目标 AABB 在前方 2 格
    player->setPosition(0.5f, 0.0f, 0.5f);
    AxisAlignedBB aabb(0.5f, 0.0f, 2.0f, 1.5f, 1.0f, 3.0f);

    // 生存模式 3.0 格，padding=0 → 应在范围内
    EXPECT_TRUE(player->isWithinEntityInteractionRange(aabb, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinEntityInteractionRange_AABB_OutOfRange)
{
    // 玩家眼睛 (0.5, 1.62, 0.5)，目标 AABB 在前方 10 格
    player->setPosition(0.5f, 0.0f, 0.5f);
    AxisAlignedBB aabb(0.5f, 0.0f, 10.0f, 1.5f, 1.0f, 11.0f);

    EXPECT_FALSE(player->isWithinEntityInteractionRange(aabb, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinEntityInteractionRange_CreativeExtendsRange)
{
    // 玩家眼睛 (0.5, 1.62, 0.5)，目标 AABB 在前方 4.5 格
    // 生存模式 3.0 不够，创造模式 5.0 够
    player->setPosition(0.5f, 0.0f, 0.5f);
    AxisAlignedBB aabb(0.5f, 0.0f, 4.5f, 1.5f, 1.0f, 5.5f);

    EXPECT_FALSE(player->isWithinEntityInteractionRange(aabb, 0.0));
    player->setGameMode(GameMode::Creative);
    EXPECT_TRUE(player->isWithinEntityInteractionRange(aabb, 0.0));
}

TEST_F(PlayerInteractionRangeTest, IsWithinEntityInteractionRange_PaddingExtendsRange)
{
    // 玩家眼睛 (0.5, 1.62, 0.5)，目标 AABB 在前方 4.0 格
    // 生存模式 3.0 不够，加 padding 1.5 → 4.5 够
    player->setPosition(0.5f, 0.0f, 0.5f);
    AxisAlignedBB aabb(0.5f, 0.0f, 4.0f, 1.5f, 1.0f, 5.0f);

    EXPECT_FALSE(player->isWithinEntityInteractionRange(aabb, 0.0));
    EXPECT_TRUE(player->isWithinEntityInteractionRange(aabb, 1.5));
}

// ========== 属性独立性测试 ==========

TEST_F(PlayerInteractionRangeTest, ModifyingLuckDoesNotAffectInteractionRange)
{
    player->attributes().setBaseValue(Attributes::LUCK, 100.0);
    EXPECT_DOUBLE_EQ(player->blockInteractionRange(), 4.5);
    EXPECT_DOUBLE_EQ(player->entityInteractionRange(), 3.0);
}

TEST_F(PlayerInteractionRangeTest, InteractionRangeDoesNotAffectOtherAttributes)
{
    player->setGameMode(GameMode::Creative);
    // 创造修饰符不应影响 MAX_HEALTH 等其他属性
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::MAX_HEALTH, 0.0), 20.0);
}
