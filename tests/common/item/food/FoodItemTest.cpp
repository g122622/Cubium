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
 * @file FoodItemTest.cpp
 * @brief FoodItem 单元测试
 *
 * 测试 FoodItem::onItemUseFinish 的核心逻辑：
 * - 非玩家 LivingEntity 食用带效果食物后获得药水效果
 * - 容器物品返回逻辑（蘑菇煲返回碗）
 * - 普通食物消耗后返回空 ItemStack
 *
 * 注意：大部分食物（如苹果、蜘蛛眼）注册为基础 Item 类（带 .food() 属性），
 * 而非 FoodItem 子类。基础 Item::onItemUseFinish 不处理食物消耗和效果。
 * 只有 FoodItem 子类（蘑菇煲、兔肉煲、甜菜汤、迷之炖菜）和 GoldenAppleItem
 * 重写了 onItemUseFinish 来处理食物效果和容器物品。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/food/Food.hpp"
#include "common/item/items/food/FoodItem.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include <memory>
#include <unordered_map>

namespace mc {
namespace {

/**
 * @brief 测试用模拟世界
 */
class FoodTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        const auto it = m_blocks.find(BlockPos(x, y, z));
        if (it != m_blocks.end()) {
            return it->second.get();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_blocks[BlockPos(x, y, z)] = std::make_unique<BlockState>(*state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        const BlockState* state = getBlockState(x, y, z);
        return state != nullptr ? state->getFluidState() : fluid::Fluid::getFluidState(0);
    }

    EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override
    {
        m_spawnedEntities.push_back(std::move(entity));
        return static_cast<EntityInstanceId>(m_spawnedEntities.size());
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::runtime_error("FoodTestWorld::tickManager not implemented");
    }
    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::runtime_error("FoodTestWorld::tickManager not implemented");
    }

private:
    std::unordered_map<BlockPos, std::unique_ptr<BlockState>> m_blocks;
    std::vector<std::unique_ptr<Entity>> m_spawnedEntities;
};

/**
 * @brief 测试专用 FoodItem 子类
 *
 * 用于独立测试 FoodItem::onItemUseFinish 的食物效果应用逻辑，
 * 不依赖全局 Items 注册表中的 FoodItem 实例。
 */
class TestFoodItem : public item::items::FoodItem {
public:
    TestFoodItem(const item::food::Food* food, ItemProperties properties)
        : FoodItem(food, std::move(properties))
    {}
};

class FoodItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    FoodTestWorld m_world;
};

// ========== 非玩家 LivingEntity 食物效果测试（通过 TestFoodItem） ==========

TEST_F(FoodItemTest, OnItemUseFinish_NonPlayerLivingEntity_GetsFoodEffect_SpiderEye)
{
    // 使用自定义 TestFoodItem 模拟蜘蛛眼（100%概率中毒）
    // 大部分食物（如蜘蛛眼）注册为基础 Item 而非 FoodItem，
    // 所以无法直接通过 Items::SPIDER_EYE 测试 FoodItem::onItemUseFinish。
    // 此处使用 TestFoodItem 直接测试 FoodItem 的效果应用逻辑。
    item::food::Food spiderEyeFood(2, 0.3f);                                   // 蜘蛛眼食物属性
    spiderEyeFood.addEffect(entity::effect::EffectType::Poison, 100, 0, 1.0f); // 100%概率中毒

    TestFoodItem testFoodItem(&spiderEyeFood, ItemProperties().maxStackSize(64));

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    // 确保实体没有中毒效果
    EXPECT_FALSE(mob.hasEffect(entity::effect::EffectType::Poison));

    // 创建物品堆叠并调用 onItemUseFinish
    ItemStack stack(&testFoodItem, 1);
    ItemStack result = testFoodItem.onItemUseFinish(stack, m_world, mob);

    // 100%概率给予中毒效果，非玩家 LivingEntity 应获得效果
    EXPECT_TRUE(mob.hasEffect(entity::effect::EffectType::Poison));

    // 验证中毒效果参数（Poison 100 ticks, amplifier 0 = level I）
    const auto* poisonEffect = mob.getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(poisonEffect, nullptr);
    EXPECT_EQ(poisonEffect->duration(), 100);
    EXPECT_EQ(poisonEffect->amplifier(), 0);
}

TEST_F(FoodItemTest, OnItemUseFinish_NonPlayerLivingEntity_MultipleEffects)
{
    // 测试非玩家 LivingEntity 食用带多个效果的食物（模拟河豚效果）
    item::food::Food pufferfishFood(1, 0.1f);
    pufferfishFood.addEffect(entity::effect::EffectType::Poison, 1200, 3, 1.0f); // 中毒IV 60秒
    pufferfishFood.addEffect(entity::effect::EffectType::Hunger, 300, 2, 1.0f);  // 饥饿III 15秒
    pufferfishFood.addEffect(entity::effect::EffectType::Nausea, 300, 0, 1.0f);  // 反胃 15秒

    TestFoodItem testFoodItem(&pufferfishFood, ItemProperties().maxStackSize(64));

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stack(&testFoodItem, 1);
    testFoodItem.onItemUseFinish(stack, m_world, mob);

    // 100%概率给予三种效果
    EXPECT_TRUE(mob.hasEffect(entity::effect::EffectType::Poison));
    EXPECT_TRUE(mob.hasEffect(entity::effect::EffectType::Hunger));
    EXPECT_TRUE(mob.hasEffect(entity::effect::EffectType::Nausea));

    // 验证中毒IV（amplifier 3 = level IV）
    const auto* poisonEffect = mob.getEffect(entity::effect::EffectType::Poison);
    ASSERT_NE(poisonEffect, nullptr);
    EXPECT_EQ(poisonEffect->amplifier(), 3);
    EXPECT_EQ(poisonEffect->duration(), 1200);

    // 验证饥饿III（amplifier 2 = level III）
    const auto* hungerEffect = mob.getEffect(entity::effect::EffectType::Hunger);
    ASSERT_NE(hungerEffect, nullptr);
    EXPECT_EQ(hungerEffect->amplifier(), 2);
    EXPECT_EQ(hungerEffect->duration(), 300);

    // 验证反胃（amplifier 0 = level I）
    const auto* nauseaEffect = mob.getEffect(entity::effect::EffectType::Nausea);
    ASSERT_NE(nauseaEffect, nullptr);
    EXPECT_EQ(nauseaEffect->amplifier(), 0);
    EXPECT_EQ(nauseaEffect->duration(), 300);
}

TEST_F(FoodItemTest, OnItemUseFinish_NonPlayerLivingEntity_NoEffectFoodNoEffect)
{
    // 测试非玩家 LivingEntity 食用无效果食物后不获得任何效果
    item::food::Food appleFood(4, 0.3f); // 苹果食物属性（无效果）

    TestFoodItem testFoodItem(&appleFood, ItemProperties().maxStackSize(64));

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stack(&testFoodItem, 1);
    testFoodItem.onItemUseFinish(stack, m_world, mob);

    // 苹果没有效果，不应该有任何活跃效果
    EXPECT_EQ(mob.effectManager().getEffectCount(), 0u);
}

TEST_F(FoodItemTest, OnItemUseFinish_NonPlayerLivingEntity_ProbabilisticEffect)
{
    // 测试非玩家 LivingEntity 食用概率效果食物（模拟腐肉80%概率饥饿）
    item::food::Food rottenFleshFood(4, 0.1f);
    rottenFleshFood.addEffect(entity::effect::EffectType::Hunger, 600, 0, 0.8f); // 80%概率饥饿

    TestFoodItem testFoodItem(&rottenFleshFood, ItemProperties().maxStackSize(64));

    bool gotHungerAtLeastOnce = false;
    constexpr int maxAttempts = 30;

    for (int i = 0; i < maxAttempts; ++i) {
        MobEntity mob(static_cast<EntityInstanceId>(100 + i));
        mob.setWorld(&m_world);
        mob.setPosition(0.0f, 64.0f, 0.0f);

        ItemStack stack(&testFoodItem, 1);
        testFoodItem.onItemUseFinish(stack, m_world, mob);

        if (mob.hasEffect(entity::effect::EffectType::Hunger)) {
            gotHungerAtLeastOnce = true;

            // 验证饥饿效果参数
            const auto* hungerEffect = mob.getEffect(entity::effect::EffectType::Hunger);
            ASSERT_NE(hungerEffect, nullptr);
            EXPECT_EQ(hungerEffect->duration(), 600);
            EXPECT_EQ(hungerEffect->amplifier(), 0);
            break;
        }
    }

    EXPECT_TRUE(gotHungerAtLeastOnce) << "Rotten flesh should give Hunger effect at least once in " << maxAttempts
                                      << " attempts (80% chance each)";
}

// ========== 容器物品返回测试 ==========

TEST_F(FoodItemTest, OnItemUseFinish_MushroomStew_ReturnsBowl)
{
    // 测试蘑菇煲消耗后返回碗（容器物品）
    // Items::MUSHROOM_STEW 是注册为 FoodItem 的食物
    if (Items::MUSHROOM_STEW == nullptr || Items::BOWL == nullptr) {
        GTEST_SKIP() << "MUSHROOM_STEW or BOWL item not registered";
    }

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stew(Items::MUSHROOM_STEW, 1);

    // 验证蘑菇煲有容器物品
    ASSERT_TRUE(stew.getItem()->hasContainerItem());
    EXPECT_EQ(stew.getItem()->containerItem(), Items::BOWL);

    // 调用 onItemUseFinish
    ItemStack result = const_cast<Item*>(stew.getItem())->onItemUseFinish(stew, m_world, mob);

    // 结果应该是碗
    EXPECT_FALSE(result.isEmpty());
    EXPECT_EQ(result.getItem(), Items::BOWL);
    EXPECT_EQ(result.getCount(), 1);
}

TEST_F(FoodItemTest, OnItemUseFinish_FoodItemWithNoContainer_ShrinksAndReturnsEmpty)
{
    // 测试 FoodItem 无容器物品时消耗后返回空 ItemStack
    item::food::Food appleFood(4, 0.3f); // 无效果、无容器物品
    TestFoodItem testFoodItem(&appleFood, ItemProperties().maxStackSize(64));

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stack(&testFoodItem, 1);
    ASSERT_FALSE(stack.getItem()->hasContainerItem());

    ItemStack result = testFoodItem.onItemUseFinish(stack, m_world, mob);

    // 无容器物品，shrink(1) 后 count=0，返回空 ItemStack
    EXPECT_TRUE(result.isEmpty());
}

// ========== 玩家与非玩家对比测试 ==========

TEST_F(FoodItemTest, OnItemUseFinish_PlayerGetsHungerAndEffects_NonPlayerGetsOnlyEffects)
{
    // 对比测试：玩家和非玩家食用带效果的食物
    item::food::Food spiderEyeFood(2, 0.3f);
    spiderEyeFood.addEffect(entity::effect::EffectType::Poison, 100, 0, 1.0f);

    TestFoodItem testFoodItem(&spiderEyeFood, ItemProperties().maxStackSize(64));

    // 玩家食用
    Player player(EntityInstanceId(1), "TestPlayer");
    player.setWorld(&m_world);
    player.setPosition(0.0f, 64.0f, 0.0f);

    // 设置较低的饥饿值以便测试恢复
    player.foodStats().setFoodLevel(10);
    i32 prevFoodLevel = player.foodStats().foodLevel();

    ItemStack playerStack(&testFoodItem, 1);
    testFoodItem.onItemUseFinish(playerStack, m_world, player);

    // 玩家应获得饥饿恢复（蜘蛛眼恢复2点饥饿值）
    EXPECT_GT(player.foodStats().foodLevel(), prevFoodLevel);
    // 玩家应获得中毒效果（100%概率）
    EXPECT_TRUE(player.hasEffect(entity::effect::EffectType::Poison));

    // 非玩家实体食用
    MobEntity mob(EntityInstanceId(2));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack mobStack(&testFoodItem, 1);
    testFoodItem.onItemUseFinish(mobStack, m_world, mob);

    // 非玩家应获得中毒效果（100%概率）
    EXPECT_TRUE(mob.hasEffect(entity::effect::EffectType::Poison));
    // 非玩家不应有 FoodStats（MobEntity 没有 foodStats 方法）
    // 这个测试验证效果对非玩家正确应用
}

TEST_F(FoodItemTest, OnItemUseFinish_ContainerItem_NonPlayer_ReturnsBowl)
{
    // 测试非玩家实体食用带容器物品的食物后返回容器
    if (Items::BOWL == nullptr) {
        GTEST_SKIP() << "BOWL item not registered";
    }

    item::food::Food stewFood(6, 0.6f); // 蘑菇煲食物属性
    TestFoodItem testStewItem(&stewFood, ItemProperties().maxStackSize(1).containerItem(Items::BOWL));

    MobEntity mob(EntityInstanceId(1));
    mob.setWorld(&m_world);
    mob.setPosition(0.0f, 64.0f, 0.0f);

    ItemStack stack(&testStewItem, 1);
    ASSERT_TRUE(stack.getItem()->hasContainerItem());

    ItemStack result = testStewItem.onItemUseFinish(stack, m_world, mob);

    // 结果应该是碗
    EXPECT_FALSE(result.isEmpty());
    EXPECT_EQ(result.getItem(), Items::BOWL);
    EXPECT_EQ(result.getCount(), 1);
}

} // namespace
} // namespace mc
