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

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/core/ItemStack.hpp"

using namespace mc;

namespace {

/**
 * @brief ����Դ·����ע���������Ʒ��
 * @param path ��Դ·����
 * @return ��ע����Ʒָ�롣
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

TEST(ItemStackJsonTest, RoundTrip_WithRegisteredItem_PreservesItemAndCount)
{
    Items::initialize();
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 13);
    nlohmann::json json = original.toJson();

    auto parsed = ItemStack::fromJson(json);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();
    EXPECT_EQ(result.getItem(), diamond);
    EXPECT_EQ(result.getCount(), 13);
}

TEST(ItemStackJsonTest, RoundTrip_PreservesNestedTagData)
{
    Items::initialize();
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    ItemStack original(diamond, 1);
    original.getOrCreateChildTag("display")["color"] = 0x123456;

    nlohmann::json json = original.toJson();
    ASSERT_TRUE(json.contains("Tag"));
    ASSERT_TRUE(json["Tag"].contains("display"));

    auto parsed = ItemStack::fromJson(json);
    ASSERT_TRUE(parsed.success()) << parsed.error().message();

    const ItemStack& result = parsed.value();
    ASSERT_TRUE(result.hasTag());
    ASSERT_NE(result.getChildTag("display"), nullptr);
    EXPECT_TRUE(result.getChildTag("display")->contains("color"));
    EXPECT_EQ((*result.getChildTag("display"))["color"].get<int>(), 0x123456);
}
