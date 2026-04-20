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
Item* ensureTestItem(const char* path) {
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }

    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

TEST(ItemStackJsonTest, RoundTrip_WithRegisteredItem_PreservesItemAndCount) {
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

TEST(ItemStackJsonTest, RoundTrip_PreservesNestedTagData) {
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
