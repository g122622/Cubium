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

// 烟花三配方（firework_star / firework_rocket / firework_star_fade）的匹配与合成测试。
//
// 覆盖：
// - FireworkStarRecipe：形状物品/荧石粉/钻石/火药的唯一性门控，染料累积为颜色列表
// - FireworkRocketRecipe：纸恰一张、火药 1~3（决定 flight_duration）、烟花之星贡献爆炸
// - FireworkStarFadeRecipe：保留星形原有字段，仅替换 fade_colors

#include <gtest/gtest.h>

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/component/Fireworks.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/special/FireworkRocketRecipe.hpp"
#include "common/item/crafting/special/FireworkStarFadeRecipe.hpp"
#include "common/item/crafting/special/FireworkStarRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"

using namespace mc;
using namespace mc::crafting;
using mc::item::component::FireworkExplosion;
using mc::item::component::FireworkShape;

namespace {

/// 3x3 合成网格
CraftingInventory _grid()
{
    return CraftingInventory(3, 3);
}

} // namespace

// ============================================================================
// FireworkStarRecipe
// ============================================================================

class FireworkStarRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        m_recipe = std::make_unique<FireworkStarRecipe>(ResourceLocation("minecraft", "firework_star"));
    }

    std::unique_ptr<FireworkStarRecipe> m_recipe;
};

TEST_F(FireworkStarRecipeTest, BasicProperties)
{
    EXPECT_EQ(m_recipe->getId().toString(), "minecraft:firework_star");
    EXPECT_TRUE(m_recipe->isSpecial());
    EXPECT_TRUE(m_recipe->isDynamic());
    EXPECT_TRUE(m_recipe->getResultItem().isEmpty());
}

TEST_F(FireworkStarRecipeTest, Matches_GunpowderAndDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, Matches_GunpowderDyeAndAllModifiers)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::FEATHER, 1));
    grid.setItemAt(0, 1, ItemStack(Items::DIAMOND, 1));
    grid.setItemAt(1, 1, ItemStack(Items::GLOWSTONE_DUST, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_MissingDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::FEATHER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_MissingGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(1, 0, ItemStack(Items::FEATHER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_SingleIngredient)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_TwoGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_TwoShapeItems)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::FEATHER, 1));
    grid.setItemAt(0, 1, ItemStack(Items::GOLD_NUGGET, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_TwoTwinkleItems)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::GLOWSTONE_DUST, 1));
    grid.setItemAt(0, 1, ItemStack(Items::GLOWSTONE_DUST, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_TwoTrailItems)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::DIAMOND, 1));
    grid.setItemAt(0, 1, ItemStack(Items::DIAMOND, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, NoMatch_UnrelatedItem)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::STONE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarRecipeTest, Assemble_DefaultShapeIsSmallBall)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);

    EXPECT_EQ(result.getItem(), Items::FIREWORK_STAR);
    EXPECT_EQ(result.getCount(), 1);
    const auto* explosion = result.getFireworkExplosion();
    ASSERT_NE(explosion, nullptr);
    EXPECT_EQ(explosion->shape, FireworkShape::SmallBall);
    EXPECT_FALSE(explosion->hasTrail);
    EXPECT_FALSE(explosion->hasTwinkle);
    ASSERT_EQ(explosion->colors.size(), 1u);
    EXPECT_EQ(explosion->colors[0], static_cast<i32>(dyeFireworkColor(DyeColor::Red)));
    EXPECT_TRUE(explosion->fadeColors.empty());
}

TEST_F(FireworkStarRecipeTest, Assemble_ShapeItemsMapToShapes)
{
    struct Case {
        const Item* item;
        FireworkShape shape;
    };
    const Case cases[] = {
        {Items::FIRE_CHARGE, FireworkShape::LargeBall},
        {Items::FEATHER, FireworkShape::Burst},
        {Items::GOLD_NUGGET, FireworkShape::Star},
        {Items::SKELETON_SKULL, FireworkShape::Creeper},
        {Items::CREEPER_HEAD, FireworkShape::Creeper},
    };

    for (const auto& c : cases) {
        auto grid = _grid();
        grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
        grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
        grid.setItemAt(2, 0, ItemStack(c.item, 1));

        const ItemStack result = m_recipe->assemble(grid);
        const auto* explosion = result.getFireworkExplosion();
        ASSERT_NE(explosion, nullptr) << "shape item: " << c.item->itemLocation().toString();
        EXPECT_EQ(explosion->shape, c.shape) << "shape item: " << c.item->itemLocation().toString();
    }
}

TEST_F(FireworkStarRecipeTest, Assemble_ModifiersAndMultipleColors)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::LIME_DYE, 1));
    grid.setItemAt(0, 1, ItemStack(Items::DIAMOND, 1));
    grid.setItemAt(1, 1, ItemStack(Items::GLOWSTONE_DUST, 1));

    const ItemStack result = m_recipe->assemble(grid);
    const auto* explosion = result.getFireworkExplosion();
    ASSERT_NE(explosion, nullptr);
    EXPECT_TRUE(explosion->hasTrail);
    EXPECT_TRUE(explosion->hasTwinkle);
    ASSERT_EQ(explosion->colors.size(), 2u);
    EXPECT_EQ(explosion->colors[0], static_cast<i32>(dyeFireworkColor(DyeColor::Red)));
    EXPECT_EQ(explosion->colors[1], static_cast<i32>(dyeFireworkColor(DyeColor::Lime)));
}

TEST_F(FireworkStarRecipeTest, RemainingItems_AllConsumed)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const auto remaining = m_recipe->getRemainingItems(grid);
    EXPECT_EQ(remaining.size(), 9u);
    for (const auto& stack : remaining) {
        EXPECT_TRUE(stack.isEmpty());
    }
}

// ============================================================================
// FireworkRocketRecipe
// ============================================================================

class FireworkRocketRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        m_recipe = std::make_unique<FireworkRocketRecipe>(ResourceLocation("minecraft", "firework_rocket"));
    }

    /// 构造携带指定爆炸的烟花之星
    static ItemStack _star(FireworkExplosion explosion)
    {
        ItemStack star(Items::FIREWORK_STAR, 1);
        star.setFireworkExplosion(std::move(explosion));
        return star;
    }

    std::unique_ptr<FireworkRocketRecipe> m_recipe;
};

TEST_F(FireworkRocketRecipeTest, BasicProperties)
{
    EXPECT_EQ(m_recipe->getId().toString(), "minecraft:firework_rocket");
    EXPECT_TRUE(m_recipe->isSpecial());
    EXPECT_TRUE(m_recipe->isDynamic());
}

TEST_F(FireworkRocketRecipeTest, Matches_PaperAndGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, Matches_PaperGunpowderAndStar)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, _star(FireworkExplosion{}));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, Matches_ThreeGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(0, 1, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, NoMatch_FourGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(0, 1, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 1, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, NoMatch_MissingPaper)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, NoMatch_MissingGunpowder)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, _star(FireworkExplosion{}));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, NoMatch_TwoPaper)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::GUNPOWDER, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, NoMatch_UnrelatedItem)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::STONE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkRocketRecipeTest, Assemble_FlightDurationFromGunpowderCount)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(0, 1, ItemStack(Items::GUNPOWDER, 1));

    const ItemStack result = m_recipe->assemble(grid);

    EXPECT_EQ(result.getItem(), Items::FIREWORK_ROCKET);
    EXPECT_EQ(result.getCount(), 3); // 每次合成产出 3 个火箭
    const auto* fireworks = result.getFireworks();
    ASSERT_NE(fireworks, nullptr);
    EXPECT_EQ(fireworks->flightDuration, 3);
    EXPECT_TRUE(fireworks->explosions.empty());
}

TEST_F(FireworkRocketRecipeTest, Assemble_StarsContributeExplosions)
{
    FireworkExplosion first{};
    first.shape = FireworkShape::Star;
    first.colors = {0x123456};
    first.hasTwinkle = true;

    FireworkExplosion second{};
    second.shape = FireworkShape::Burst;

    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, _star(first));
    grid.setItemAt(0, 1, _star(second));

    const ItemStack result = m_recipe->assemble(grid);
    const auto* fireworks = result.getFireworks();
    ASSERT_NE(fireworks, nullptr);
    EXPECT_EQ(fireworks->flightDuration, 1);
    ASSERT_EQ(fireworks->explosions.size(), 2u);
    EXPECT_EQ(fireworks->explosions[0], first);
    EXPECT_EQ(fireworks->explosions[1], second);
}

TEST_F(FireworkRocketRecipeTest, Assemble_StarWithoutComponentContributesNothing)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::PAPER, 1));
    grid.setItemAt(1, 0, ItemStack(Items::GUNPOWDER, 1));
    grid.setItemAt(2, 0, ItemStack(Items::FIREWORK_STAR, 1)); // 无 firework_explosion 组件

    const ItemStack result = m_recipe->assemble(grid);
    const auto* fireworks = result.getFireworks();
    ASSERT_NE(fireworks, nullptr);
    EXPECT_TRUE(fireworks->explosions.empty());
}

// ============================================================================
// FireworkStarFadeRecipe
// ============================================================================

class FireworkStarFadeRecipeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override
    {
        m_recipe = std::make_unique<FireworkStarFadeRecipe>(ResourceLocation("minecraft", "firework_star_fade"));
    }

    static ItemStack _star(FireworkExplosion explosion)
    {
        ItemStack star(Items::FIREWORK_STAR, 1);
        star.setFireworkExplosion(std::move(explosion));
        return star;
    }

    std::unique_ptr<FireworkStarFadeRecipe> m_recipe;
};

TEST_F(FireworkStarFadeRecipeTest, BasicProperties)
{
    EXPECT_EQ(m_recipe->getId().toString(), "minecraft:firework_star_fade");
    EXPECT_TRUE(m_recipe->isSpecial());
    EXPECT_TRUE(m_recipe->isDynamic());
}

TEST_F(FireworkStarFadeRecipeTest, Matches_StarAndDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _star(FireworkExplosion{}));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkStarFadeRecipeTest, Matches_StarAndMultipleDyes)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _star(FireworkExplosion{}));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::LIME_DYE, 1));

    EXPECT_TRUE(m_recipe->matches(grid));
}

TEST_F(FireworkStarFadeRecipeTest, NoMatch_TwoStars)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _star(FireworkExplosion{}));
    grid.setItemAt(1, 0, _star(FireworkExplosion{}));
    grid.setItemAt(2, 0, ItemStack(Items::RED_DYE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarFadeRecipeTest, NoMatch_MissingDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _star(FireworkExplosion{}));
    grid.setItemAt(1, 0, ItemStack(Items::FIREWORK_ROCKET, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarFadeRecipeTest, NoMatch_NonStarNonDye)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, _star(FireworkExplosion{}));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::STONE, 1));

    EXPECT_FALSE(m_recipe->matches(grid));
}

TEST_F(FireworkStarFadeRecipeTest, Assemble_SetsFadeColorsAndKeepsShape)
{
    FireworkExplosion original{};
    original.shape = FireworkShape::Creeper;
    original.colors = {0xAABBCC};
    original.hasTrail = true;
    original.hasTwinkle = true;

    auto grid = _grid();
    grid.setItemAt(0, 0, _star(original));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));
    grid.setItemAt(2, 0, ItemStack(Items::LIME_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);

    EXPECT_EQ(result.getItem(), Items::FIREWORK_STAR);
    EXPECT_EQ(result.getCount(), 1);
    const auto* explosion = result.getFireworkExplosion();
    ASSERT_NE(explosion, nullptr);
    // 形状/主色/拖尾/闪烁原样保留
    EXPECT_EQ(explosion->shape, FireworkShape::Creeper);
    ASSERT_EQ(explosion->colors.size(), 1u);
    EXPECT_EQ(explosion->colors[0], 0xAABBCC);
    EXPECT_TRUE(explosion->hasTrail);
    EXPECT_TRUE(explosion->hasTwinkle);
    // 渐变色为染料色
    ASSERT_EQ(explosion->fadeColors.size(), 2u);
    EXPECT_EQ(explosion->fadeColors[0], static_cast<i32>(dyeFireworkColor(DyeColor::Red)));
    EXPECT_EQ(explosion->fadeColors[1], static_cast<i32>(dyeFireworkColor(DyeColor::Lime)));
}

TEST_F(FireworkStarFadeRecipeTest, Assemble_StarWithoutComponentUsesDefaultExplosion)
{
    auto grid = _grid();
    grid.setItemAt(0, 0, ItemStack(Items::FIREWORK_STAR, 1)); // 无 firework_explosion 组件
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);
    const auto* explosion = result.getFireworkExplosion();
    ASSERT_NE(explosion, nullptr);
    // 默认爆炸为小圆球，仅渐变色被染料填充
    EXPECT_EQ(explosion->shape, FireworkShape::SmallBall);
    EXPECT_TRUE(explosion->colors.empty());
    ASSERT_EQ(explosion->fadeColors.size(), 1u);
    EXPECT_EQ(explosion->fadeColors[0], static_cast<i32>(dyeFireworkColor(DyeColor::Red)));
}

TEST_F(FireworkStarFadeRecipeTest, Assemble_ReplacesPreviousFadeColors)
{
    FireworkExplosion original{};
    original.fadeColors = {0x111111, 0x222222};

    auto grid = _grid();
    grid.setItemAt(0, 0, _star(original));
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    const ItemStack result = m_recipe->assemble(grid);
    const auto* explosion = result.getFireworkExplosion();
    ASSERT_NE(explosion, nullptr);
    ASSERT_EQ(explosion->fadeColors.size(), 1u);
    EXPECT_EQ(explosion->fadeColors[0], static_cast<i32>(dyeFireworkColor(DyeColor::Red)));
}

TEST_F(FireworkStarFadeRecipeTest, Assemble_DoesNotMutateSourceStar)
{
    FireworkExplosion original{};
    original.fadeColors = {0x111111};
    const ItemStack sourceStar = _star(original);

    auto grid = _grid();
    grid.setItemAt(0, 0, sourceStar);
    grid.setItemAt(1, 0, ItemStack(Items::RED_DYE, 1));

    m_recipe->assemble(grid);

    // 源物品（网格内的）不应被就地修改。注：须先取出副本再取组件指针——
    // getFireworkExplosion() 返回指向该 ItemStack 内部 extras 的指针，直接对
    // getItemAt() 的临时量取指针会立即悬垂。
    const ItemStack sourceAfter = grid.getItemAt(0, 0);
    const auto* sourceExplosion = sourceAfter.getFireworkExplosion();
    ASSERT_NE(sourceExplosion, nullptr);
    ASSERT_EQ(sourceExplosion->fadeColors.size(), 1u);
    EXPECT_EQ(sourceExplosion->fadeColors[0], 0x111111);
}
