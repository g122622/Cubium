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
 * @file PlayerCanEatTest.cpp
 * @brief Player::canEat 单元测试（对齐 MC Java 1.21.11）
 *
 * vanilla Player.canEat（Player.java:1593-1595）：
 *   return this.abilities.invulnerable || ignoreHunger || this.foodData.needsFood();
 *
 * - 创造模式 abilities.invulnerable=true → 可进食任何食物（含饱食时的 alwaysEdible、蛋糕）
 * - ignoreHunger（canAlwaysEat 食物，如金苹果/蜂蜜瓶）→ 饱食可食
 * - 否则 needsFood()（foodLevel < 20）
 *
 * 此前 Cubium 用 isCreative()||isSpectator() 早返回 false 拦截创造模式，致创造模式吃不了蛋糕
 * （CakeBlock 用 canEat(false)），且与项目内 FoodItem/GoldenAppleItem 放行创造的路径自相矛盾。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace {

class PlayerCanEatTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }

    // 创建指定游戏模式、指定饥饿值的 Player。
    std::unique_ptr<Player> makePlayer(GameMode mode, i32 foodLevel)
    {
        auto player = std::make_unique<Player>(EntityInstanceId(2), "Tester", mc::test::testEcsRegistry());
        player->setGameMode(mode);
        player->foodStats().setFoodLevel(foodLevel);
        return player;
    }
};

// ============================================================================
// 创造模式：对齐 vanilla abilities.invulnerable 放行（核心缺陷修复点）
// vanilla CakeBlock.eat（CakeBlock.java:86-89）调 canEat(false)，创造模式可吃蛋糕。
// ============================================================================

TEST_F(PlayerCanEatTest, Creative_CanEatEvenWhenFull)
{
    // 创造模式 + 饱食（foodLevel=20）+ canEat(false)（蛋糕路径）→ 应放行
    auto player = makePlayer(GameMode::Creative, 20);
    EXPECT_TRUE(player->canEat(false))
        << "Creative player must be able to eat cake even when full (vanilla abilities.invulnerable)";
}

TEST_F(PlayerCanEatTest, Creative_CanEatWhenHungry)
{
    auto player = makePlayer(GameMode::Creative, 10);
    EXPECT_TRUE(player->canEat(false));
    EXPECT_TRUE(player->canEat(true));
}

// ============================================================================
// 生存模式：对齐 vanilla needsFood / ignoreHunger 语义
// ============================================================================

TEST_F(PlayerCanEatTest, Survival_Full_CannotEatNormalFood)
{
    // 生存 + 饱食 + canEat(false)（普通食物/蛋糕）→ needsFood()=false → 不可食
    auto player = makePlayer(GameMode::Survival, 20);
    EXPECT_FALSE(player->canEat(false)) << "Survival full player cannot eat normal food/cake";
}

TEST_F(PlayerCanEatTest, Survival_Full_CanEatAlwaysEdible)
{
    // 生存 + 饱食 + canEat(true)（alwaysEdible 食物如金苹果/蜂蜜瓶）→ ignoreHunger 放行
    auto player = makePlayer(GameMode::Survival, 20);
    EXPECT_TRUE(player->canEat(true)) << "Survival full player can eat alwaysEdible food (golden apple/honey)";
}

TEST_F(PlayerCanEatTest, Survival_Hungry_CanEat)
{
    // 生存 + 饥饿（foodLevel<20）→ needsFood()=true → 可食
    auto player = makePlayer(GameMode::Survival, 15);
    EXPECT_TRUE(player->canEat(false));
    EXPECT_TRUE(player->canEat(true));
}

// ============================================================================
// 冒险模式：与生存同（abilities.invulnerable=false，走 needsFood/ignoreHunger）
// ============================================================================

TEST_F(PlayerCanEatTest, Adventure_Full_CannotEatNormalFood)
{
    auto player = makePlayer(GameMode::Adventure, 20);
    EXPECT_FALSE(player->canEat(false));
    EXPECT_TRUE(player->canEat(true));
}

TEST_F(PlayerCanEatTest, Adventure_Hungry_CanEat)
{
    auto player = makePlayer(GameMode::Adventure, 5);
    EXPECT_TRUE(player->canEat(false));
}

// ============================================================================
// 边界：饥饿值恰好 20（needsFood 严格小于 20）
// ============================================================================

TEST_F(PlayerCanEatTest, Survival_ExactlyFull_CannotEatNormalFood)
{
    // foodLevel=20 时 needsFood()=false（needsFood = foodLevel < 20，严格小于）
    auto player = makePlayer(GameMode::Survival, 20);
    EXPECT_FALSE(player->canEat(false));
}

TEST_F(PlayerCanEatTest, Survival_OneBelowFull_CanEatNormalFood)
{
    // foodLevel=19 时 needsFood()=true
    auto player = makePlayer(GameMode::Survival, 19);
    EXPECT_TRUE(player->canEat(false));
}

} // namespace
} // namespace mc
