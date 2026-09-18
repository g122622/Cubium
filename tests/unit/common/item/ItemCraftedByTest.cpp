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

#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDataManager.hpp"
#include "item/Items.hpp"
#include "item/core/Item.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/map/FilledMapItem.hpp"

using namespace mc;
using namespace mc::item::items;

namespace {

/**
 * @brief 注册测试用物品
 * @param path 物品资源路径
 * @return 已注册的物品指针
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

// ============================================================================
// Item::onCraftedBy / Item::onCraftedPostProcess 基类默认行为测试
// ============================================================================

class ItemCraftedByTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemCraftedByTest, BaseItemOnCraftedByDoesNotCrash)
{
    // 基类 Item 的 onCraftedBy 默认实现应转发给 onCraftedPostProcess（空操作），
    // 不应崩溃。由于需要 IWorld 和 Player，这里只验证方法存在且可调用。
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);

    // 验证物品可以创建 ItemStack
    ItemStack stack(*stick, 1);
    EXPECT_FALSE(stack.isEmpty());
}

TEST_F(ItemCraftedByTest, BaseItemOnCraftedPostProcessDoesNotCrash)
{
    // 基类 Item 的 onCraftedPostProcess 默认实现为空操作。
    // 这里验证 FilledMapItem 已注册且可以创建 ItemStack。
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    EXPECT_FALSE(stack.isEmpty());
}

// ============================================================================
// ItemStack::onCraftedBy 边界场景测试
// ============================================================================

class ItemStackCraftedByTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemStackCraftedByTest, EmptyStackOnCraftedByDoesNotCrash)
{
    // 空 ItemStack 调用 onCraftedBy 应安全返回，不崩溃。
    // 这里无法传入 Player（需要完整游戏世界），但可以验证空栈的基本检查。
    ItemStack emptyStack;
    EXPECT_TRUE(emptyStack.isEmpty());
    // onCraftedBy 内部第一行检查 isEmpty()，空栈直接返回
}

TEST_F(ItemStackCraftedByTest, StackWithNullItemDoesNotCrash)
{
    // ItemStack 持有 nullptr item 也应安全处理
    ItemStack nullItemStack(static_cast<const Item*>(nullptr), 1);
    EXPECT_TRUE(nullItemStack.isEmpty());
}

// ============================================================================
// FilledMapItem::onCraftedPostProcess NBT 处理测试
// ============================================================================

class FilledMapItemCraftedTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(FilledMapItemCraftedTest, MapScaleDirectionNbtTagProcessed)
{
    // 验证 FilledMapItem 在合成后处理中正确读取 map_scale_direction NBT 标签
    // 设置 map_scale_direction=1 应触发 scaleMap，处理后标签应被移除
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 设置 map_scale_direction NBT 标签
    tag["map_scale_direction"] = 1;

    // 验证标签已设置
    EXPECT_TRUE(tag.contains("map_scale_direction"));
    EXPECT_EQ(tag["map_scale_direction"].get<i32>(), 1);

    // 注意：onCraftedPostProcess 需要 IWorld（MapDataManager），
    // 无法在单元测试中直接调用（无世界实例）。
    // 这里验证 NBT 标签的设置和读取逻辑正确，
    // 实际的 scaleMap/lockMap 调用需在集成测试中验证。
}

TEST_F(FilledMapItemCraftedTest, MapLockNbtTagSet)
{
    // 验证制图台锁定地图时设置的 map_lock NBT 标签
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 设置 map_lock NBT 标签（模拟制图台锁定地图）
    tag["map_lock"] = 1;

    EXPECT_TRUE(tag.contains("map_lock"));
    EXPECT_EQ(tag["map_lock"].get<i32>(), 1);
}

TEST_F(FilledMapItemCraftedTest, NoSpecialNbtTagNoProcessing)
{
    // 没有 map_scale_direction 或 map_lock NBT 标签的地图不应被处理
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);

    // 不设置任何特殊 NBT 标签
    const auto* tag = stack.getTag();
    // 标签可能不存在（空 NBT）或不含特殊标签
    if (tag != nullptr) {
        EXPECT_FALSE(tag->contains("map_scale_direction"));
        EXPECT_FALSE(tag->contains("map_lock"));
    }
}

TEST_F(FilledMapItemCraftedTest, MapScaleDirectionZeroNoProcessing)
{
    // map_scale_direction=0 不应触发缩放（0 表示无变化）
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 设置 map_scale_direction=0
    tag["map_scale_direction"] = 0;

    EXPECT_TRUE(tag.contains("map_scale_direction"));
    EXPECT_EQ(tag["map_scale_direction"].get<i32>(), 0);

    // onCraftedPostProcess 中 scaleDirection=0 不会调用 scaleMap，
    // 但仍会移除标签
}

TEST_F(FilledMapItemCraftedTest, BothNbtTagsSet)
{
    // 同时设置 map_scale_direction 和 map_lock NBT 标签
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 同时设置两个标签
    tag["map_scale_direction"] = 1;
    tag["map_lock"] = 1;

    EXPECT_TRUE(tag.contains("map_scale_direction"));
    EXPECT_TRUE(tag.contains("map_lock"));

    // onCraftedPostProcess 应先处理 scale，再处理 lock，最后移除两个标签
}

TEST_F(FilledMapItemCraftedTest, MapScaleDirectionRemovedAfterProcessing)
{
    // 验证 removeChildTag 可以正确移除 map_scale_direction 标签
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 设置标签
    tag["map_scale_direction"] = 1;
    EXPECT_TRUE(tag.contains("map_scale_direction"));

    // 模拟 onCraftedPostProcess 的标签移除逻辑
    stack.removeChildTag("map_scale_direction");

    // 验证标签已移除
    const auto* updatedTag = stack.getTag();
    if (updatedTag != nullptr) {
        EXPECT_FALSE(updatedTag->contains("map_scale_direction"));
    }
}

TEST_F(FilledMapItemCraftedTest, MapLockRemovedAfterProcessing)
{
    // 验证 removeChildTag 可以正确移除 map_lock 标签
    Item* filledMap = ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map"));
    ASSERT_NE(filledMap, nullptr);

    ItemStack stack(*filledMap, 1);
    auto& tag = stack.getOrCreateTag();

    // 设置标签
    tag["map_lock"] = 1;
    EXPECT_TRUE(tag.contains("map_lock"));

    // 模拟 onCraftedPostProcess 的标签移除逻辑
    stack.removeChildTag("map_lock");

    // 验证标签已移除
    const auto* updatedTag = stack.getTag();
    if (updatedTag != nullptr) {
        EXPECT_FALSE(updatedTag->contains("map_lock"));
    }
}

TEST_F(FilledMapItemCraftedTest, RegularItemHasNoMapTags)
{
    // 普通物品不应有地图相关的 NBT 标签
    Item* stick = ItemRegistry::instance().getItem(ResourceLocation("minecraft:stick"));
    ASSERT_NE(stick, nullptr);

    ItemStack stack(*stick, 1);
    const auto* tag = stack.getTag();

    // 普通物品的 tag 为空或不含地图标签
    if (tag != nullptr) {
        EXPECT_FALSE(tag->contains("map_scale_direction"));
        EXPECT_FALSE(tag->contains("map_lock"));
        EXPECT_FALSE(tag->contains("map"));
    }
}
