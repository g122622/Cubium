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
 * @file PlayerLuckAttributeTest.cpp
 * @brief Player LUCK 属性测试
 *
 * 测试 Player LUCK 属性注册：
 * - Player::registerAttributes() 注册 LUCK 属性
 * - 默认值为 0.0
 * - 受幸运/霉运药水效果影响
 *
 * 参考 MC 1.16.5: PlayerEntity.registerAttributes() 注册 generic.luck 属性
 */

#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::attribute;

namespace {

/**
 * @brief Player LUCK 属性测试夹具
 */
class PlayerLuckAttributeTest : public ::testing::Test {
protected:
    void SetUp() override { player = std::make_unique<Player>(static_cast<EntityInstanceId>(1), "TestPlayer"); }

    void TearDown() override { player.reset(); }

    std::unique_ptr<Player> player;
};

// ========== LUCK 属性注册测试 ==========

TEST_F(PlayerLuckAttributeTest, HasLuckAttribute)
{
    // Player 应该有 LUCK 属性
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::LUCK));
}

TEST_F(PlayerLuckAttributeTest, LuckDefaultValueIsZero)
{
    // MC 1.16.5: LUCK 属性默认值为 0.0
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, -999.0), 0.0);
}

TEST_F(PlayerLuckAttributeTest, LuckAttributeRange)
{
    // LUCK 属性范围应为 -1024.0 到 1024.0
    // 先设置一个正值
    player->attributes().setBaseValue(Attributes::LUCK, 5.0);
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), 5.0);

    // 设置一个负值
    player->attributes().setBaseValue(Attributes::LUCK, -3.0);
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), -3.0);
}

TEST_F(PlayerLuckAttributeTest, LuckPotionEffectAddsModifier)
{
    // 模拟幸运药水效果：添加 +1.0 修改器
    constexpr const char* LUCK_POTION_UUID = "03C3C89D-7037-4B42-869F-B735591E2D3E";

    player->attributes().setBaseValue(Attributes::LUCK, 0.0);

    AttributeModifier luckMod(LUCK_POTION_UUID, "luck", 1.0, Operation::Addition);
    player->attributes().addModifier(Attributes::LUCK, luckMod);

    // 0 + 1 = 1
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), 1.0);

    // 再添加一级幸运
    AttributeModifier luckMod2("luck-level-2", "luck2", 1.0, Operation::Addition);
    player->attributes().addModifier(Attributes::LUCK, luckMod2);

    // 0 + 1 + 1 = 2
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), 2.0);
}

TEST_F(PlayerLuckAttributeTest, BadLuckPotionEffectSubtractsModifier)
{
    // 模拟霉运药水效果：添加 -1.0 修改器
    constexpr const char* BAD_LUCK_POTION_UUID = "CC5AF142-2BD2-4215-B636-2605A17BEFD1";

    player->attributes().setBaseValue(Attributes::LUCK, 0.0);

    AttributeModifier badLuckMod(BAD_LUCK_POTION_UUID, "bad_luck", -1.0, Operation::Addition);
    player->attributes().addModifier(Attributes::LUCK, badLuckMod);

    // 0 - 1 = -1
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), -1.0);
}

TEST_F(PlayerLuckAttributeTest, CombinedLuckEffects)
{
    // 基础幸运值 0
    player->attributes().setBaseValue(Attributes::LUCK, 0.0);

    // 幸运药水 II (+2)
    AttributeModifier luckMod("luck-ii", "luck", 2.0, Operation::Addition);
    player->attributes().addModifier(Attributes::LUCK, luckMod);

    // 霉运药水 (-1)
    AttributeModifier badLuckMod("bad-luck", "bad_luck", -1.0, Operation::Addition);
    player->attributes().addModifier(Attributes::LUCK, badLuckMod);

    // 0 + 2 - 1 = 1
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), 1.0);
}

// ========== getAttributeValue 方法测试 ==========

TEST_F(PlayerLuckAttributeTest, GetAttributeValueReturnsRegisteredValue)
{
    // 对于已注册的属性，应返回实际值
    player->attributes().setBaseValue(Attributes::LUCK, 3.5);
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::LUCK, 0.0), 3.5);
}

TEST_F(PlayerLuckAttributeTest, GetAttributeValueReturnsDefaultForUnregistered)
{
    // 对于未注册的属性，应返回默认值
    // 先移除 LUCK 属性（模拟未注册情况）
    // 注意：Player 构造函数中已注册 LUCK，所以这个测试验证默认值参数的使用
    EXPECT_DOUBLE_EQ(player->getAttributeValue("non.existent.attribute", 42.0), 42.0);
}

// ========== 其他玩家属性测试 ==========

TEST_F(PlayerLuckAttributeTest, PlayerHasStandardAttributes)
{
    // Player 应该有标准属性
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::MAX_HEALTH));
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::MOVEMENT_SPEED));
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::ATTACK_DAMAGE));
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::ATTACK_SPEED));
    EXPECT_TRUE(player->attributes().hasAttribute(Attributes::LUCK));
}

TEST_F(PlayerLuckAttributeTest, LuckAttributeDoesNotAffectOtherAttributes)
{
    // 修改 LUCK 不应该影响其他属性
    player->attributes().setBaseValue(Attributes::LUCK, 10.0);

    // MAX_HEALTH 应该保持默认
    EXPECT_DOUBLE_EQ(player->getAttributeValue(Attributes::MAX_HEALTH, 0.0), 20.0);

    // MOVEMENT_SPEED 应该保持默认（使用 EXPECT_NEAR 因为浮点精度问题）
    EXPECT_NEAR(player->getAttributeValue(Attributes::MOVEMENT_SPEED, 0.0), 0.1, 0.0001);
}

} // namespace
