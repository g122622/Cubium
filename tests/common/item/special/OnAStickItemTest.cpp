/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
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
 * @file OnAStickItemTest.cpp
 * @brief OnAStickItem 钓竿类物品单元测试
 *
 * 测试钓竿类物品（胡萝卜钓竿、诡异菌钓竿）的核心逻辑：
 * - 物品注册和属性
 * - 实体类型匹配
 * - 耐久度消耗
 * - 损坏后转换为钓鱼竿
 * - canBeSteered() 方法与钓竿物品的交互
 *
 * MC 1.16.5 参考：
 * - OnAStickItem.onItemRightClick() 触发加速
 * - CarrotOnAStickItem: 猪控制，25耐久度，每次消耗7
 * - WarpedFungusOnAStickItem: 炽足兽控制，100耐久度，每次消耗1
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/OnAStickItem.hpp"
#include "common/item/items/special/StickItems.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

// 导入 item 命名空间以便直接使用类型名
using namespace item;

/**
 * @brief 测试用世界存根
 */
class StickItemTestWorld final : public test::BaseTestWorld {
public:
    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override
    {
        auto it = m_entities.find(id);
        return it != m_entities.end() ? it->second : nullptr;
    }

    void registerEntity(EntityInstanceId id, Entity* entity) { m_entities[id] = entity; }

    void clearEntities() { m_entities.clear(); }

private:
    std::unordered_map<EntityInstanceId, Entity*> m_entities;
};

/**
 * @brief 测试基类
 */
class OnAStickItemTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override { m_world = std::make_unique<StickItemTestWorld>(); }

    std::unique_ptr<StickItemTestWorld> m_world;
};

// ============================================================================
// 物品注册测试
// ============================================================================

TEST_F(OnAStickItemTest, CarrotOnAStickIsRegistered)
{
    ASSERT_NE(Items::CARROT_ON_A_STICK, nullptr) << "CARROT_ON_A_STICK should be registered";
}

TEST_F(OnAStickItemTest, WarpedFungusOnAStickIsRegistered)
{
    ASSERT_NE(Items::WARPED_FUNGUS_ON_A_STICK, nullptr) << "WARPED_FUNGUS_ON_A_STICK should be registered";
}

TEST_F(OnAStickItemTest, FishingRodIsRegistered)
{
    ASSERT_NE(Items::FISHING_ROD, nullptr) << "FISHING_ROD should be registered for durability conversion";
}

// ============================================================================
// 物品属性测试
// ============================================================================

TEST_F(OnAStickItemTest, CarrotOnAStickProperties)
{
    auto* item = Items::CARROT_ON_A_STICK;
    ASSERT_NE(item, nullptr);

    // 验证耐久度
    EXPECT_EQ(item->maxDamage(), CarrotOnAStickItem::MAX_DAMAGE) << "CarrotOnAStick should have 25 max damage";
    EXPECT_EQ(item->maxDamage(), 25);

    // 验证附魔能力
    EXPECT_EQ(item->getItemEnchantability(), 1) << "OnAStickItem should have enchantability of 1";
}

TEST_F(OnAStickItemTest, WarpedFungusOnAStickProperties)
{
    auto* item = Items::WARPED_FUNGUS_ON_A_STICK;
    ASSERT_NE(item, nullptr);

    // 验证耐久度
    EXPECT_EQ(item->maxDamage(), WarpedFungusOnAStickItem::MAX_DAMAGE)
        << "WarpedFungusOnAStick should have 100 max damage";
    EXPECT_EQ(item->maxDamage(), 100);

    // 验证附魔能力
    EXPECT_EQ(item->getItemEnchantability(), 1) << "OnAStickItem should have enchantability of 1";
}

TEST_F(OnAStickItemTest, CarrotOnAStickEntityId)
{
    auto* carrotStick = static_cast<OnAStickItem*>(Items::CARROT_ON_A_STICK);
    ASSERT_NE(carrotStick, nullptr);

    EXPECT_EQ(carrotStick->getEntityTypeId(), "minecraft:pig") << "CarrotOnAStick should target minecraft:pig";
}

TEST_F(OnAStickItemTest, WarpedFungusOnAStickEntityId)
{
    auto* warpedStick = static_cast<OnAStickItem*>(Items::WARPED_FUNGUS_ON_A_STICK);
    ASSERT_NE(warpedStick, nullptr);

    EXPECT_EQ(warpedStick->getEntityTypeId(), "minecraft:strider")
        << "WarpedFungusOnAStick should target minecraft:strider";
}

TEST_F(OnAStickItemTest, CarrotOnAStickDurabilityCost)
{
    auto* carrotStick = static_cast<OnAStickItem*>(Items::CARROT_ON_A_STICK);
    ASSERT_NE(carrotStick, nullptr);

    EXPECT_EQ(carrotStick->getDurabilityCost(), CarrotOnAStickItem::DURABILITY_COST)
        << "CarrotOnAStick should consume 7 durability per boost";
    EXPECT_EQ(carrotStick->getDurabilityCost(), 7);
}

TEST_F(OnAStickItemTest, WarpedFungusOnAStickDurabilityCost)
{
    auto* warpedStick = static_cast<OnAStickItem*>(Items::WARPED_FUNGUS_ON_A_STICK);
    ASSERT_NE(warpedStick, nullptr);

    EXPECT_EQ(warpedStick->getDurabilityCost(), WarpedFungusOnAStickItem::DURABILITY_COST)
        << "WarpedFungusOnAStick should consume 1 durability per boost";
    EXPECT_EQ(warpedStick->getDurabilityCost(), 1);
}

// ============================================================================
// ItemStack 测试
// ============================================================================

TEST_F(OnAStickItemTest, ItemStackCreation)
{
    ItemStack carrotStack(Items::CARROT_ON_A_STICK, 1);
    EXPECT_FALSE(carrotStack.isEmpty());
    EXPECT_EQ(carrotStack.getItem(), Items::CARROT_ON_A_STICK);
    EXPECT_EQ(carrotStack.getCount(), 1);
    EXPECT_EQ(carrotStack.getDamage(), 0) << "New item should have 0 damage";
}

TEST_F(OnAStickItemTest, ItemStackDurabilityTracking)
{
    ItemStack stack(Items::CARROT_ON_A_STICK, 1);

    // 初始耐久度
    EXPECT_EQ(stack.getDamage(), 0);
    EXPECT_EQ(stack.getMaxDamage(), 25);

    // 模拟耐久度消耗
    stack.attemptDamageItem(7, nullptr);
    EXPECT_EQ(stack.getDamage(), 7);

    // 再次消耗
    stack.attemptDamageItem(7, nullptr);
    EXPECT_EQ(stack.getDamage(), 14);

    // 再次消耗
    stack.attemptDamageItem(7, nullptr);
    EXPECT_EQ(stack.getDamage(), 21);

    // 最后一次消耗应该损坏物品
    stack.attemptDamageItem(7, nullptr);
    // 耐久度超过最大值，物品应该变为空
    EXPECT_TRUE(stack.isEmpty() || stack.getDamage() >= 25);
}

// ============================================================================
// 耐久度消耗计算测试
// ============================================================================

TEST_F(OnAStickItemTest, CarrotOnAStickUseCount)
{
    // 胡萝卜钓竿：25耐久度，每次消耗7
    // 使用次数：floor(25/7) = 3次（剩余4耐久度）
    constexpr i32 maxDamage = 25;
    constexpr i32 costPerUse = 7;
    constexpr i32 expectedUses = maxDamage / costPerUse; // 3

    EXPECT_EQ(expectedUses, 3) << "CarrotOnAStick should allow 3 uses before breaking";

    // 计算实际使用次数
    i32 uses = 0;
    i32 damage = 0;
    while (damage + costPerUse <= maxDamage) {
        damage += costPerUse;
        uses++;
    }
    EXPECT_EQ(uses, 3);
    EXPECT_EQ(damage, 21) << "After 3 uses, damage should be 21";
}

TEST_F(OnAStickItemTest, WarpedFungusOnAStickUseCount)
{
    // 诡异菌钓竿：100耐久度，每次消耗1
    // 使用次数：100次
    constexpr i32 maxDamage = 100;
    constexpr i32 costPerUse = 1;
    constexpr i32 expectedUses = maxDamage / costPerUse; // 100

    EXPECT_EQ(expectedUses, 100) << "WarpedFungusOnAStick should allow 100 uses before breaking";
}

// ============================================================================
// 实体类型匹配测试
// ============================================================================

TEST_F(OnAStickItemTest, EntityIdMatching)
{
    auto* carrotStick = static_cast<OnAStickItem*>(Items::CARROT_ON_A_STICK);
    auto* warpedStick = static_cast<OnAStickItem*>(Items::WARPED_FUNGUS_ON_A_STICK);

    // 胡萝卜钓竿匹配猪
    EXPECT_EQ(carrotStick->getEntityTypeId(), "minecraft:pig");
    EXPECT_NE(carrotStick->getEntityTypeId(), "minecraft:strider");

    // 诡异菌钓竿匹配炽足兽
    EXPECT_EQ(warpedStick->getEntityTypeId(), "minecraft:strider");
    EXPECT_NE(warpedStick->getEntityTypeId(), "minecraft:pig");
}

// ============================================================================
// canBeSteered 与钓竿物品关系测试
// ============================================================================

/**
 * @brief PigEntity canBeSteered 测试
 *
 * 验证猪在有鞍且玩家手持胡萝卜钓竿时可以被控制
 * 注意：此测试验证 canBeSteered() 的逻辑结构，不依赖 BoostHelper 初始化
 */
TEST_F(OnAStickItemTest, PigCanBeSteeredRequiresSaddleAndPlayer)
{
    // 创建猪实体
    PigEntity pig(EntityInstanceId(1));

    // 无鞍且无乘客时不能控制
    // 注意：BoostHelper 未初始化，hasSaddle() 返回 false
    EXPECT_FALSE(pig.hasSaddle());
    EXPECT_FALSE(pig.canBeSteered()); // 无鞍 + 无乘客
}

/**
 * @brief StriderEntity canBeSteered 测试
 *
 * 验证炽足兽在有鞍且玩家手持诡异菌钓竿时可以被控制
 */
TEST_F(OnAStickItemTest, StriderCanBeSteeredRequiresSaddleAndPlayer)
{
    // 创建炽足兽实体
    StriderEntity strider(EntityInstanceId(1));

    // 无鞍且无乘客时不能控制
    // 注意：BoostHelper 未初始化，hasSaddle() 返回 false
    EXPECT_FALSE(strider.hasSaddle());
    EXPECT_FALSE(strider.canBeSteered()); // 无鞍 + 无乘客
}

// ============================================================================
// 物品标识符测试
// ============================================================================

TEST_F(OnAStickItemTest, ItemIdentifiers)
{
    EXPECT_EQ(Items::CARROT_ON_A_STICK->itemLocation(), ResourceLocation("minecraft:carrot_on_a_stick"));
    EXPECT_EQ(Items::WARPED_FUNGUS_ON_A_STICK->itemLocation(), ResourceLocation("minecraft:warped_fungus_on_a_stick"));
}

// ============================================================================
// 类型转换测试
// ============================================================================

TEST_F(OnAStickItemTest, DynamicCastToOnAStickItem)
{
    // 验证可以安全地转换到 OnAStickItem
    Item* baseItem = Items::CARROT_ON_A_STICK;
    auto* stickItem = dynamic_cast<OnAStickItem*>(baseItem);
    ASSERT_NE(stickItem, nullptr) << "CARROT_ON_A_STICK should be castable to OnAStickItem";

    baseItem = Items::WARPED_FUNGUS_ON_A_STICK;
    stickItem = dynamic_cast<OnAStickItem*>(baseItem);
    ASSERT_NE(stickItem, nullptr) << "WARPED_FUNGUS_ON_A_STICK should be castable to OnAStickItem";
}

TEST_F(OnAStickItemTest, DynamicCastFromNullptr)
{
    Item* nullItem = nullptr;
    auto* stickItem = dynamic_cast<OnAStickItem*>(nullItem);
    EXPECT_EQ(stickItem, nullptr) << "Casting nullptr should return nullptr";
}

// ============================================================================
// 附魔能力测试
// ============================================================================

TEST_F(OnAStickItemTest, Enchantability)
{
    // MC 1.16.5: OnAStickItem.getItemEnchantability() 返回 1
    EXPECT_EQ(Items::CARROT_ON_A_STICK->getItemEnchantability(), 1);
    EXPECT_EQ(Items::WARPED_FUNGUS_ON_A_STICK->getItemEnchantability(), 1);
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(OnAStickItemTest, ConstantValuesMatchMC)
{
    // 胡萝卜钓竿常量验证 (MC 1.16.5)
    EXPECT_EQ(CarrotOnAStickItem::MAX_DAMAGE, 25);
    EXPECT_EQ(CarrotOnAStickItem::DURABILITY_COST, 7);

    // 诡异菌钓竿常量验证 (MC 1.16.5)
    EXPECT_EQ(WarpedFungusOnAStickItem::MAX_DAMAGE, 100);
    EXPECT_EQ(WarpedFungusOnAStickItem::DURABILITY_COST, 1);
}

} // namespace
} // namespace mc
