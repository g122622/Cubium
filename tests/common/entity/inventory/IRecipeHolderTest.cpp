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
 * @file IRecipeHolderTest.cpp
 * @brief IRecipeHolder 接口单元测试
 *
 * 测试 IRecipeHolder::canUseRecipe() 方法的有限合成规则检查逻辑。
 * 参考 MC 1.16.5: net.minecraft.inventory.IRecipeHolder.canUseRecipe()
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/IRecipeHolder.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/item/crafting/RecipeBook.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "item/Items.hpp"
#include "server/player/ServerPlayer.hpp"

using namespace mc;
using namespace mc::world::gamerule;

// ============================================================================
// Mock 类定义
// ============================================================================

/**
 * @brief 测试用的 Mock IWorld 实现
 *
 * 继承 BaseTestWorld 并添加 GameRules 支持。
 */
class MockWorld : public mc::test::BaseTestWorld {
public:
    MockWorld()
        : BaseTestWorld()
        , m_gameRules()
    {}

    // ========== GameRules 访问 ==========

    [[nodiscard]] const GameRules& getGameRules() const override { return m_gameRules; }
    [[nodiscard]] GameRules& getGameRules() override { return m_gameRules; }

    // ========== 测试辅助方法 ==========

    /**
     * @brief 设置有限合成规则
     * @param enabled 是否启用有限合成
     */
    void setLimitedCrafting(bool enabled)
    {
        m_gameRules.setBoolean(GameRuleKeys::DO_LIMITED_CRAFTING, enabled, nullptr);
    }

private:
    GameRules m_gameRules;
};

/**
 * @brief 测试用的 Mock IRecipe 实现
 */
class MockRecipe : public crafting::IRecipe<IInventory> {
public:
    explicit MockRecipe(const ResourceLocation& id, bool isDynamic = false)
        : m_id(id)
        , m_isDynamic(isDynamic)
        , m_ingredients()
    {}

    [[nodiscard]] ResourceLocation getId() const override { return m_id; }

    [[nodiscard]] bool matches(const IInventory&) const override { return true; }

    [[nodiscard]] ItemStack assemble(const IInventory&) const override { return ItemStack(); }

    [[nodiscard]] ItemStack getResultItem() const override { return ItemStack(); }

    [[nodiscard]] bool isDynamic() const override { return m_isDynamic; }

    [[nodiscard]] crafting::RecipeType getType() const override { return crafting::RecipeType::Crafting; }

    [[nodiscard]] const std::vector<crafting::Ingredient>& getIngredients() const override { return m_ingredients; }

    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory&) const override { return {}; }

private:
    ResourceLocation m_id;
    bool m_isDynamic;
    std::vector<crafting::Ingredient> m_ingredients;
};

/**
 * @brief 测试用的 IRecipeHolder 实现类
 */
class TestRecipeHolder : public IRecipeHolder {
public:
    TestRecipeHolder()
        : m_recipeUsed(nullptr)
    {}

    void setRecipeUsed(const crafting::IRecipe<IInventory>* recipe) override { m_recipeUsed = recipe; }

    [[nodiscard]] const crafting::IRecipe<IInventory>* getRecipeUsed() const override { return m_recipeUsed; }

private:
    const crafting::IRecipe<IInventory>* m_recipeUsed;
};

// ============================================================================
// 测试夹具
// ============================================================================

class IRecipeHolderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化物品系统
        Items::initialize();

        // 创建测试对象
        world_ = std::make_unique<MockWorld>();
        holder_ = std::make_unique<TestRecipeHolder>();

        // 创建测试配方
        normalRecipe_ = std::make_unique<MockRecipe>(ResourceLocation("minecraft", "test_recipe"), false);
        dynamicRecipe_ = std::make_unique<MockRecipe>(ResourceLocation("minecraft", "dynamic_recipe"), true);
        anotherRecipe_ = std::make_unique<MockRecipe>(ResourceLocation("minecraft", "another_recipe"), false);
    }

    void TearDown() override
    {
        holder_.reset();
        world_.reset();
        normalRecipe_.reset();
        dynamicRecipe_.reset();
        anotherRecipe_.reset();
    }

    // 测试对象
    std::unique_ptr<MockWorld> world_;
    std::unique_ptr<TestRecipeHolder> holder_;

    // 测试配方
    std::unique_ptr<MockRecipe> normalRecipe_;
    std::unique_ptr<MockRecipe> dynamicRecipe_;
    std::unique_ptr<MockRecipe> anotherRecipe_;
};

// ============================================================================
// 基础测试：空配方和动态配方
// ============================================================================

TEST_F(IRecipeHolderTest, NullRecipe_ReturnsTrue)
{
    // 空配方应该直接返回 true
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    bool result = holder_->canUseRecipe(*world_, player, nullptr);
    EXPECT_TRUE(result);
    EXPECT_EQ(holder_->getRecipeUsed(), nullptr);
}

TEST_F(IRecipeHolderTest, DynamicRecipe_ReturnsTrue)
{
    // 动态配方应该直接返回 true，不受有限合成规则影响
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    bool result = holder_->canUseRecipe(*world_, player, dynamicRecipe_.get());
    EXPECT_TRUE(result);
    EXPECT_EQ(holder_->getRecipeUsed(), dynamicRecipe_.get());
}

TEST_F(IRecipeHolderTest, DynamicRecipe_WithLimitedCraftingOff_ReturnsTrue)
{
    // 有限合成关闭时，动态配方也应该返回 true
    world_->setLimitedCrafting(false);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    bool result = holder_->canUseRecipe(*world_, player, dynamicRecipe_.get());
    EXPECT_TRUE(result);
    EXPECT_EQ(holder_->getRecipeUsed(), dynamicRecipe_.get());
}

// ============================================================================
// 有限合成关闭测试
// ============================================================================

TEST_F(IRecipeHolderTest, LimitedCraftingOff_UnlockedRecipe_ReturnsTrue)
{
    // 有限合成关闭时，无论配方是否解锁都应该返回 true
    world_->setLimitedCrafting(false);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 配方未解锁，但有限合成关闭
    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_TRUE(result);
    EXPECT_EQ(holder_->getRecipeUsed(), normalRecipe_.get());
}

TEST_F(IRecipeHolderTest, LimitedCraftingOff_LockedRecipe_ReturnsTrue)
{
    // 有限合成关闭时，锁定配方也应该返回 true
    world_->setLimitedCrafting(false);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 配方未解锁，但有限合成关闭，应该允许使用
    bool result = holder_->canUseRecipe(*world_, player, anotherRecipe_.get());
    EXPECT_TRUE(result);
}

// ============================================================================
// 有限合成开启测试
// ============================================================================

TEST_F(IRecipeHolderTest, LimitedCraftingOn_UnlockedRecipe_ReturnsTrue)
{
    // 有限合成开启，但配方已解锁，应该返回 true
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 解锁配方
    player.getRecipeBook().unlock(normalRecipe_->getId());

    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_TRUE(result);
    EXPECT_EQ(holder_->getRecipeUsed(), normalRecipe_.get());
}

TEST_F(IRecipeHolderTest, LimitedCraftingOn_LockedRecipe_ReturnsFalse)
{
    // 有限合成开启，配方未解锁，应该返回 false
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 配方未解锁
    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_FALSE(result);
    // 配方不应该被设置
    EXPECT_EQ(holder_->getRecipeUsed(), nullptr);
}

TEST_F(IRecipeHolderTest, LimitedCraftingOn_PartiallyUnlockedRecipes)
{
    // 有限合成开启，部分配方解锁
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 只解锁 normalRecipe_，anotherRecipe_ 未解锁
    player.getRecipeBook().unlock(normalRecipe_->getId());

    // 已解锁的配方应该可用
    bool result1 = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_TRUE(result1);

    // 未解锁的配方应该不可用
    bool result2 = holder_->canUseRecipe(*world_, player, anotherRecipe_.get());
    EXPECT_FALSE(result2);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_F(IRecipeHolderTest, MultipleUnlocks_StillWorks)
{
    // 多次解锁同一个配方不应该有问题
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 多次解锁
    player.getRecipeBook().unlock(normalRecipe_->getId());
    player.getRecipeBook().unlock(normalRecipe_->getId());
    player.getRecipeBook().unlock(normalRecipe_->getId());

    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_TRUE(result);
}

TEST_F(IRecipeHolderTest, LockAfterUnlock_BlocksRecipe)
{
    // 解锁后再锁定，应该不可用
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 解锁然后锁定
    player.getRecipeBook().unlock(normalRecipe_->getId());
    EXPECT_TRUE(player.getRecipeBook().isUnlocked(normalRecipe_->getId()));

    player.getRecipeBook().lock(normalRecipe_->getId());
    EXPECT_FALSE(player.getRecipeBook().isUnlocked(normalRecipe_->getId()));

    // 现在应该不可用
    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_FALSE(result);
}

TEST_F(IRecipeHolderTest, RecipeUsedSetAfterSuccessfulUse)
{
    // 成功使用配方后，getRecipeUsed() 应该返回该配方
    world_->setLimitedCrafting(false);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_EQ(holder_->getRecipeUsed(), normalRecipe_.get());

    // 再次使用另一个配方
    holder_->canUseRecipe(*world_, player, anotherRecipe_.get());
    EXPECT_EQ(holder_->getRecipeUsed(), anotherRecipe_.get());
}

TEST_F(IRecipeHolderTest, RecipeUsedNotSetAfterFailedUse)
{
    // 失败时不应设置 recipeUsed
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 配方未解锁，应该失败
    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_FALSE(result);

    // recipeUsed 不应该被设置
    EXPECT_EQ(holder_->getRecipeUsed(), nullptr);
}

// ============================================================================
// GameRules 默认值测试
// ============================================================================

TEST_F(IRecipeHolderTest, DefaultLimitedCrafting_IsFalse)
{
    // 默认情况下，doLimitedCrafting 应该是 false
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 不设置任何规则，使用默认值
    bool result = holder_->canUseRecipe(*world_, player, normalRecipe_.get());
    EXPECT_TRUE(result); // 默认关闭有限合成，应该允许
}

// ============================================================================
// 组合场景测试
// ============================================================================

TEST_F(IRecipeHolderTest, MultipleRecipes_MixedUnlockState)
{
    // 多个配方，部分解锁部分未解锁
    world_->setLimitedCrafting(true);
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 只解锁 normalRecipe_
    player.getRecipeBook().unlock(normalRecipe_->getId());

    // 测试所有配方
    EXPECT_TRUE(holder_->canUseRecipe(*world_, player, normalRecipe_.get()));
    EXPECT_FALSE(holder_->canUseRecipe(*world_, player, anotherRecipe_.get()));
    EXPECT_TRUE(holder_->canUseRecipe(*world_, player, dynamicRecipe_.get())); // 动态配方始终可用
}

TEST_F(IRecipeHolderTest, ToggleLimitedCrafting_AffectsAvailability)
{
    // 切换有限合成规则影响配方可用性
    ServerPlayer player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 配方未解锁

    // 有限合成关闭 -> 可用
    world_->setLimitedCrafting(false);
    EXPECT_TRUE(holder_->canUseRecipe(*world_, player, normalRecipe_.get()));

    // 开启有限合成 -> 不可用
    world_->setLimitedCrafting(true);
    EXPECT_FALSE(holder_->canUseRecipe(*world_, player, normalRecipe_.get()));

    // 再次关闭 -> 可用
    world_->setLimitedCrafting(false);
    EXPECT_TRUE(holder_->canUseRecipe(*world_, player, normalRecipe_.get()));
}

// ============================================================================
// onCrafting 测试
// ============================================================================

TEST_F(IRecipeHolderTest, OnCrafting_WithNormalRecipe_UnlocksRecipe)
{
    // 使用普通配方合成后，应该触发配方解锁
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 设置使用的配方
    holder_->setRecipeUsed(normalRecipe_.get());
    EXPECT_EQ(holder_->getRecipeUsed(), normalRecipe_.get());

    // 调用 onCrafting
    holder_->onCrafting(player);

    // 配方应该被清除
    EXPECT_EQ(holder_->getRecipeUsed(), nullptr);
}

TEST_F(IRecipeHolderTest, OnCrafting_WithDynamicRecipe_NotCleared)
{
    // 使用动态配方合成后，recipeUsed 不应被清除
    // 参考 MC 1.16.5: onCrafting() 只对非动态配方清除 recipeUsed
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    // 设置使用的动态配方
    holder_->setRecipeUsed(dynamicRecipe_.get());

    // 调用 onCrafting
    holder_->onCrafting(player);

    // 动态配方的 recipeUsed 不应该被清除
    EXPECT_EQ(holder_->getRecipeUsed(), dynamicRecipe_.get());
}

TEST_F(IRecipeHolderTest, OnCrafting_WithNullRecipe_DoesNothing)
{
    // 空配方调用 onCrafting 不应该崩溃
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    holder_->setRecipeUsed(nullptr);
    holder_->onCrafting(player);

    EXPECT_EQ(holder_->getRecipeUsed(), nullptr);
}
