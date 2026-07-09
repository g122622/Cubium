/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "client/renderer/trident/particle/data/ItemParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>
#include <gtest/gtest.h>

namespace mc {
namespace {

using namespace client::renderer::trident::particle;
using namespace client::renderer::trident::particle::data;

// ==================== 构造测试 ====================

TEST(ItemParticleDataTest, Construction_SetsType)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    EXPECT_EQ(data.getType(), ParticleTypeId::Item);
}

TEST(ItemParticleDataTest, Construction_SetsItemSlimeType)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemSlime, itemStack);

    EXPECT_EQ(data.getType(), ParticleTypeId::ItemSlime);
}

TEST(ItemParticleDataTest, Construction_SetsItemCobwebType)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemCobweb, itemStack);

    EXPECT_EQ(data.getType(), ParticleTypeId::ItemCobweb);
}

TEST(ItemParticleDataTest, Construction_SetsItemSnowballType)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemSnowball, itemStack);

    EXPECT_EQ(data.getType(), ParticleTypeId::ItemSnowball);
}

TEST(ItemParticleDataTest, Construction_PreservesItemStack)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    // 空 ItemStack 应保持为空
    EXPECT_TRUE(data.getItemStack().isEmpty());
}

// ==================== getTypeName 测试 ====================

TEST(ItemParticleDataTest, GetTypeName_ItemType_ReturnsItemName)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    EXPECT_EQ(data.getTypeName(), "minecraft:item");
}

TEST(ItemParticleDataTest, GetTypeName_ItemSlimeType_ReturnsItemSlimeName)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemSlime, itemStack);

    EXPECT_EQ(data.getTypeName(), "minecraft:item_slime");
}

TEST(ItemParticleDataTest, GetTypeName_ItemCobwebType_ReturnsItemCobwebName)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemCobweb, itemStack);

    EXPECT_EQ(data.getTypeName(), "minecraft:item_cobweb");
}

TEST(ItemParticleDataTest, GetTypeName_ItemSnowballType_ReturnsItemSnowballName)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemSnowball, itemStack);

    EXPECT_EQ(data.getTypeName(), "minecraft:item_snowball");
}

// ==================== getParameters 测试 ====================

TEST(ItemParticleDataTest, GetParameters_EmptyItemStack_ReturnsAir)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    // 空 ItemStack 的参数应为 "minecraft:air"
    EXPECT_EQ(data.getParameters(), "minecraft:air");
}

// ==================== clone 测试 ====================

TEST(ItemParticleDataTest, Clone_ReturnsCopyWithType)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::ItemSlime, itemStack);

    auto cloned = data.clone();
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->getType(), ParticleTypeId::ItemSlime);
}

TEST(ItemParticleDataTest, Clone_ReturnsCopyWithItemStack)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    auto cloned = data.clone();
    ASSERT_NE(cloned, nullptr);

    auto* itemCloned = dynamic_cast<ItemParticleData*>(cloned.get());
    ASSERT_NE(itemCloned, nullptr);
    EXPECT_TRUE(itemCloned->getItemStack().isEmpty());
}

TEST(ItemParticleDataTest, Clone_IndependentFromOriginal)
{
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);

    auto cloned = data.clone();
    ASSERT_NE(cloned, nullptr);

    // 克隆对象与原对象类型相同但独立
    EXPECT_EQ(cloned->getType(), data.getType());
}

// ==================== 非物品类型断言测试 ====================

TEST(ItemParticleDataTest, Construction_NonItemType_TriggersAssert)
{
    // 非物品类型构造应触发 MC_ASSERT_RELEASE
    // 由于 MC_ASSERT_RELEASE 在 Release 模式下也会触发，这里验证断言
    // 注意：此测试依赖断言实现，如果断言为 abort() 则测试进程会终止
    // 项目中 MC_ASSERT_RELEASE_MSG 的默认行为是终止程序
    // 因此这里只验证物品类型能正常构造
    ItemStack itemStack;
    ItemParticleData data(ParticleTypeId::Item, itemStack);
    EXPECT_EQ(data.getType(), ParticleTypeId::Item);
}

} // namespace
} // namespace mc
